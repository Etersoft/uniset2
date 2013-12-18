// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMultiMaster::MBTCPMultiMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, 
                            SharedMemory* ic, const std::string prefix ):
MBExchange(objId,shmId,ic,prefix),
force_disconnect(true),
pollThread(0),
checkThread(0)
{
    tcpMutex.setName(myname+"_tcpMutex");

    if( objId == DefaultObjectId )
        throw UniSetTypes::SystemError("(MBTCPMultiMaster): objId=-1?!! Use --" + prefix + "-name" );

    // префикс для "свойств" - по умолчанию
    prop_prefix = "tcp_";
    // если задано поле для "фильтрации"
    // то в качестве префикса используем его
    if( !s_field.empty() )
        prop_prefix = s_field + "_";
    // если "принудительно" задан префикс
    // используем его.
    {
        string p("--" + prefix + "-set-prop-prefix");
        string v = conf->getArgParam(p,"");
        if( !v.empty() && v[0] != '-' )
            prop_prefix = v;
        // если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
        else if( findArgParam(p,conf->getArgc(),conf->getArgv()) != -1 )
            prop_prefix = "";
    }

    if( dlog.is_info() )
        dlog.info() << myname << "(init): prop_prefix=" << prop_prefix << endl;

    UniXML_iterator it(cnode);

    checktime = conf->getArgPInt("--" + prefix + "-checktime",it.getProp("checktime"), 5000);
    force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection",it.getProp("persistent_connection")) ? false : true;

    UniXML_iterator it1(it);
    if( !it1.find("GateList") )
    {
        ostringstream err;
        err << myname << "(init): not found <GateList>";
        if( dlog.is_crit() )
            dlog.crit() << err.str() << endl;
        throw UniSetTypes::SystemError(err.str());
    }

    if( !it1.goChildren() )
    {
        ostringstream err;
        err << myname << "(init): empty <GateList> ?!";
        if( dlog.is_crit() )
            dlog.crit() << err.str() << endl;
        throw UniSetTypes::SystemError(err.str());
    }

    for( ;it1.getCurrent(); it1++ )
    {
        MBSlaveInfo sinf;
        sinf.ip = it1.getProp("ip");
        if( sinf.ip.empty() )
        {
            ostringstream err;
            err << myname << "(init): ip='' in <GateList>";
            if( dlog.is_crit() )
                dlog.crit() << err.str() << endl;
            throw UniSetTypes::SystemError(err.str());
        }

        sinf.port = it1.getIntProp("port");
        if( sinf.port <=0 )
        {
            ostringstream err;
            err << myname << "(init): ERROR: port=''" << sinf.port << " for ip='" << sinf.ip << "' in <GateList>";
            if( dlog.is_crit() )
                dlog.crit() << err.str() << endl;
            throw UniSetTypes::SystemError(err.str());
        }

        if( !it1.getProp("respond").empty() )
        {
            sinf.respond_id = conf->getSensorID( it1.getProp("respond") );
            if( sinf.respond_id == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): ERROR: Unknown SensorID for '" << it1.getProp("respond") << "' in <GateList>";
                if( dlog.is_crit() )
                    dlog.crit() << err.str() << endl;
                throw UniSetTypes::SystemError(err.str());
            }
        }

        sinf.priority = it1.getIntProp("priority");
        sinf.mbtcp = new ModbusTCPMaster();

        sinf.recv_timeout = it1.getPIntProp("recv_timeout",recv_timeout);
        sinf.aftersend_pause = it1.getPIntProp("aftersend_pause",aftersend_pause);
        sinf.sleepPause_usec = it1.getPIntProp("sleepPause_usec",sleepPause_usec);
        sinf.respond_invert = it1.getPIntProp("respond_invert",0);

        sinf.force_disconnect = it.getPIntProp("persistent_connection",!force_disconnect) ? false : true;

        ostringstream n;
        n << sinf.ip << ":" << sinf.port;
        sinf.myname = n.str();

        if( dlog.debugging(Debug::LEVEL9) )
            sinf.mbtcp->setLog(dlog);

        mblist.push_back(sinf);

        if( dlog.is_info() )
            dlog.info() << myname << "(init): add slave channel " << sinf.myname << endl;
    }

    if( mblist.empty() )
    {
        ostringstream err;
        err << myname << "(init): empty <GateList>!";
        if( dlog.is_crit() )
            dlog.crit() << err.str() << endl;
        throw UniSetTypes::SystemError(err.str());
    }
    
    mblist.sort();
    mbi = mblist.rbegin();

    if( shm->isLocalwork() )
    {
        readConfiguration();
        rtuQueryOptimization(rmap);
        initDeviceList();
    }
    else
        ic->addReadItem( sigc::mem_fun(this,&MBTCPMultiMaster::readItem) );

    pollThread = new ThreadCreator<MBTCPMultiMaster>(this, &MBTCPMultiMaster::poll_thread);
    checkThread = new ThreadCreator<MBTCPMultiMaster>(this, &MBTCPMultiMaster::check_thread);


    // Т.к. при "многоканальном" доступе к slave, смена канала должна происходит сразу после
    // неудачной попытки запросов по одному из каналов, то ПЕРЕОПРЕДЕЛЯЕМ reopen, на timeout..
    ptReopen.setTiming(ptTimeout.getInterval());

    if( dlog.is_info() )
        printMap(rmap);
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster::~MBTCPMultiMaster()
{
    delete pollThread;
    delete checkThread;
    for( MBGateList::iterator it=mblist.begin(); it!=mblist.end(); ++it )
    {
        delete it->mbtcp;
        it->mbtcp = 0;
        mbi = mblist.rend();
    }
}
// -----------------------------------------------------------------------------
ModbusClient* MBTCPMultiMaster::initMB( bool reopen )
{
    // просто движемся по кругу (т.к. связь не проверяется)
    // движемся в обратном порядке, т.к. сортировка по возрастанию приоритета
    if( checktime <=0 )
    {
        mbi++;
        if( mbi == mblist.rend() )    
            mbi = mblist.rbegin();

        mbi->init();
        mb = mbi->mbtcp;
        return mbi->mbtcp;
    }

    {
        uniset_rwmutex_wrlock l(tcpMutex);
        // Если по текущему каналу связь есть, то возвращаем его
        if( mbi!=mblist.rend() && mbi->respond )
        {
            if( mbi->mbtcp->isConnection() || ( !mbi->mbtcp->isConnection() && mbi->init()) )
            {
                mb = mbi->mbtcp;
                return mbi->mbtcp;
            }
        }

        if( mbi != mblist.rend() )
            mbi->mbtcp->disconnect();
    }

    // проходим по списку (в обратном порядке, т.к. самый приоритетный в конце)
    for( MBGateList::reverse_iterator it=mblist.rbegin(); it!=mblist.rend(); ++it )
    {
        uniset_rwmutex_wrlock l(tcpMutex);
        if( it->respond && it->init() )
        {
            mbi = it;
            mb = mbi->mbtcp;
            return it->mbtcp;
        }
    }

    {
        uniset_rwmutex_wrlock l(tcpMutex);
        mbi = mblist.rend();
        mb = 0;
    }

    return 0;
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::check()
{
     return mbtcp->checkConnection(ip,port);
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::init()
{
    try
    {
        // ost::Thread::setException(ost::Thread::throwException);

        if( dlog.is_info() )
            dlog.info() << myname << "(init): connect..." << endl;

        mbtcp->connect(ip,port);
        mbtcp->setForceDisconnect(force_disconnect);

        if( recv_timeout > 0 )
            mbtcp->setTimeout(recv_timeout);

//        if( !initOK )
        {
            mbtcp->setSleepPause(sleepPause_usec);
            mbtcp->setAfterSendPause(aftersend_pause);

            if( mbtcp->isConnection() && dlog.is_info() )
                dlog.info() << "(init): " << myname << " connect OK" << endl;
        
            initOK = true;
        }
        return mbtcp->isConnection();
    }
    catch( ModbusRTU::mbException& ex )
    {
        if( dlog.is_warn() )
            dlog.warn() << "(init): " << ex << endl;
    }
    catch(...)
    {
        if( dlog.is_warn() )
            dlog.warn() << "(init): " << myname << " catch ..." << endl;
    }

    initOK = false;
    return false;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sysCommand( UniSetTypes::SystemMessage *sm )
{
    MBExchange::sysCommand(sm);
    if( sm->command == SystemMessage::StartUp )
    {
        pollThread->start();
        if( checktime > 0 )
            checkThread->start();
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::poll_thread()
{
    {
        uniset_mutex_lock l(pollMutex,300);
        ptTimeout.reset();
    }

    while( checkProcActive() )
    {
        try
        {
            if( sidExchangeMode != DefaultObjectId && force )
                exchangeMode = shm->localGetValue(itExchangeMode,sidExchangeMode);
        }
        catch(...){}
        try
        {
            poll();
        }
        catch(...){}

        if( !checkProcActive() )
            break;

        msleep(polltime);
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::check_thread()
{
    while( checkProcActive() )
    {
        for( MBGateList::iterator it=mblist.begin(); it!=mblist.end(); ++it )
        {
            try
            {
                bool r = it->check();
                if( dlog.is_info() )
                    dlog.info() << myname << "(check): " << it->myname << " " << ( r ? "OK" : "FAIL" ) << endl;

                try
                {
                    if( it->respond_id != DefaultObjectId && (force_out || r != it->respond) )
                    {
                        bool set = it->respond_invert ? !it->respond : it->respond;
                        shm->localSetValue(it->respond_it,it->respond_id,(set ? 1:0),getId());
                    }
                }
                catch( Exception& ex )
                {
                    if( dlog.debugging(Debug::CRIT) )
                        dlog.crit() << myname << "(check): (respond) " << ex << std::endl;
                }
                catch(...){}


                {    
                    uniset_rwmutex_wrlock l(tcpMutex);
                    it->respond = r;
                }
            }
            catch(...){}

            if( !checkProcActive() )
                break;
        }

        if( !checkProcActive() )
            break;

        msleep(checktime);
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::initIterators()
{
    MBExchange::initIterators();
    for( MBGateList::iterator it=mblist.begin(); it!=mblist.end(); ++it )
        shm->initIterator(it->respond_it);
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::help_print( int argc, const char* const* argv )
{
    cout << "Default: prefix='mbtcp'" << endl;
    MBExchange::help_print(argc,argv);
    cout << endl;
    cout << " Настройки протокола TCP(MultiMaster): " << endl;
    cout << "--prefix-persistent-connection 0,1     - Не закрывать соединение на каждом цикле опроса" << endl;
    cout << "--prefix-checktime                     - период проверки связи по каналам (<GateList>)" << endl;
    cout << endl;
    cout << " ВНИМАНИЕ! '--prefix-reopen-timeout' для MBTCPMultiMaster НЕ ДЕЙСТВУЕТ! " << endl;
    cout << " Переключение на следующий канал зависит от '--prefix-timeout'" << endl;
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster* MBTCPMultiMaster::init_mbmaster( int argc, const char* const* argv, 
                                            UniSetTypes::ObjectId icID, SharedMemory* ic, 
                                            const std::string prefix )
{
    string name = conf->getArgParam("--" + prefix + "-name","MBTCPMultiMaster1");
    if( name.empty() )
    {
        dlog.crit() << "(MBTCPMultiMaster): Не задан name'" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);
    if( ID == UniSetTypes::DefaultObjectId )
    {
        dlog.crit() << "(MBTCPMultiMaster): идентификатор '" << name 
            << "' не найден в конф. файле!"
            << " в секции " << conf->getObjectsSection() << endl;
        return 0;
    }

    dlog.info() << "(MBTCPMultiMaster): name = " << name << "(" << ID << ")" << endl;
    return new MBTCPMultiMaster(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
