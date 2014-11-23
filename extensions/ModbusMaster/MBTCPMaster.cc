// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMaster::MBTCPMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, 
                            SharedMemory* ic, const std::string& prefix ):
MBExchange(objId,shmId,ic,prefix),
force_disconnect(true),
mbtcp(nullptr),
pollThread(0)
{
    if( objId == DefaultObjectId )
        throw UniSetTypes::SystemError("(MBTCPMaster): objId=-1?!! Use --" + prefix + "-name" );

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

    dinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

    UniXML::iterator it(cnode);

    // ---------- init MBTCP ----------
    string pname("--" + prefix + "-gateway-iaddr");
    iaddr    = conf->getArgParam(pname,it.getProp("gateway_iaddr"));
    if( iaddr.empty() )
        throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet addr...(Use: " + pname +")" );

    string tmp("--" + prefix + "-gateway-port");
    port = conf->getArgInt(tmp,it.getProp("gateway_port"));
    if( port <= 0 )
        throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet port...(Use: " + tmp +")" );

    dinfo << myname << "(init): gateway " << iaddr << ":" << port << endl;

    force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection",it.getProp("persistent_connection")) ? false : true;
    dinfo << myname << "(init): persisten-connection=" << (!force_disconnect) << endl;

    if( shm->isLocalwork() )
    {
        readConfiguration();
        rtuQueryOptimization(rmap);
        initDeviceList();
    }
    else
        ic->addReadItem( sigc::mem_fun(this,&MBTCPMaster::readItem) );

    pollThread = new ThreadCreator<MBTCPMaster>(this, &MBTCPMaster::poll_thread);

    if( dlog.is_info() )
        printMap(rmap);
}
// -----------------------------------------------------------------------------
MBTCPMaster::~MBTCPMaster()
{
    if( pollThread )
    {
        pollThread->stop();
        if( pollThread->isRunning() )
            pollThread->join();
    }
    delete pollThread;
    //delete mbtcp;
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> MBTCPMaster::initMB( bool reopen )
{
    if( mbtcp )
    {
        if( !reopen )
            return mbtcp;

        mbtcp = nullptr;
        mb = nullptr;
    }

    try
    {
        ost::Thread::setException(ost::Thread::throwException);
        mbtcp = std::make_shared<ModbusTCPMaster>();

        ost::InetAddress ia(iaddr.c_str());
        mbtcp->connect(ia,port);
        mbtcp->setForceDisconnect(force_disconnect);

        if( recv_timeout > 0 )
            mbtcp->setTimeout(recv_timeout);

        mbtcp->setSleepPause(sleepPause_usec);

        mbtcp->setAfterSendPause(aftersend_pause);

         dinfo << myname << "(init): ipaddr=" << iaddr << " port=" << port << endl;

        if( dlog.is_level9() )
            mbtcp->setLog(dlog);
    }
    catch( ModbusRTU::mbException& ex )
    {
        dwarn << "(init): " << ex << endl;
    }
    catch(...)
    {
        mb = nullptr;
        mbtcp = nullptr;
    }

    mb = mbtcp;
    return mbtcp;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sysCommand( const UniSetTypes::SystemMessage *sm )
{
    MBExchange::sysCommand(sm);
    if( sm->command == SystemMessage::StartUp )
        pollThread->start();
}
// -----------------------------------------------------------------------------
void MBTCPMaster::poll_thread()
{
    {
        uniset_rwmutex_wrlock l(pollMutex);
        ptTimeout.reset();
    }

    while( checkProcActive() )
    {
        try
        {
            if( sidExchangeMode != DefaultObjectId && force )
                exchangeMode = shm->localGetValue(itExchangeMode,sidExchangeMode);
        }
        catch(...)
        {
            throw;
        }

        try
        {
            poll();
        }
        catch(...)
        {
//            if( !checkProcActive() )
                throw;
        }

        if( !checkProcActive() )
            break;

        msleep(polltime);
    }
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sigterm( int signo )
{
    setProcActive(false);

    if( pollThread )
    {
        pollThread->stop();
        if( pollThread->isRunning() )
            pollThread->join();
        
        delete pollThread;
        pollThread = 0;
    }

    try
    {
        MBExchange::sigterm(signo);
    }
    catch( const std::exception& ex )
    {
        cerr << "catch: " << ex.what() << endl;
    }
    catch( ... )
    {
        std::exception_ptr p = std::current_exception();
        std::clog <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
    }
}

// -----------------------------------------------------------------------------
void MBTCPMaster::help_print( int argc, const char* const* argv )
{
    cout << "Default: prefix='mbtcp'" << endl;
    MBExchange::help_print(argc,argv);
    cout << endl;
    cout << " Настройки протокола TCP: " << endl;
    cout << "--prefix-gateway-iaddr hostname,IP     - IP опрашиваемого узла" << endl;
    cout << "--prefix-gateway-port num              - port на опрашиваемом узле" << endl;
    cout << "--prefix-persistent-connection 0,1     - Не закрывать соединение на каждом цикле опроса" << endl;
}
// -----------------------------------------------------------------------------
MBTCPMaster* MBTCPMaster::init_mbmaster( int argc, const char* const* argv, 
                                            UniSetTypes::ObjectId icID, SharedMemory* ic,
                                            const std::string& prefix )
{
    string name = conf->getArgParam("--" + prefix + "-name","MBTCPMaster1");
    if( name.empty() )
    {
        dcrit << "(MBTCPMaster): Не задан name'" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);
    if( ID == UniSetTypes::DefaultObjectId )
    {
        dcrit << "(MBTCPMaster): идентификатор '" << name
                << "' не найден в конф. файле!"
                << " в секции " << conf->getObjectsSection() << endl;
        return 0;
    }

    dinfo << "(MBTCPMaster): name = " << name << "(" << ID << ")" << endl;
    return new MBTCPMaster(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
