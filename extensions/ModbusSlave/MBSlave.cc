#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "MBSlave.h"
#include "modbus/ModbusRTUSlaveSlot.h"
#include "modbus/ModbusTCPServerSlot.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
using namespace ModbusRTU;
// -----------------------------------------------------------------------------
MBSlave::MBSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, const std::shared_ptr<SharedMemory> ic, const string& prefix ):
UniSetObject_LT(objId),
initPause(0),
test_id(DefaultObjectId),
askcount_id(DefaultObjectId),
respond_id(DefaultObjectId),
respond_invert(false),
askCount(0),
activated(false),
cancelled(false),
activateTimeout(500),
pingOK(true),
force(false),
mbregFromID(false),
prefix(prefix),
prop_prefix("")
{
    if( objId == DefaultObjectId )
        throw UniSetTypes::SystemError("(MBSlave): objId=-1?!! Use --mbs-name" );

    auto conf = uniset_conf();
    mutex_start.setName(myname + "_mutex_start");

//    xmlNode* cnode = conf->getNode(myname);

	string conf_name(conf->getArgParam("--" + prefix + "-confnode",myname));
	cnode = conf->getNode(conf_name);
    if( cnode == NULL )
        throw UniSetTypes::SystemError("(MBSlave): Not found conf-node for " + myname );

    shm = make_shared<SMInterface>(shmId,ui,objId,ic);

    UniXML::iterator it(cnode);

    // определяем фильтр
    s_field = conf->getArgParam("--" + prefix + "-filter-field");
    s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
    dinfo << myname << "(init): read s_field='" << s_field
                        << "' s_fvalue='" << s_fvalue << "'" << endl;

    // префикс для "свойств" - по умолчанию
    prop_prefix = "";
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

    force = conf->getArgInt("--" + prefix + "-force",it.getProp("force"));

    // int recv_timeout = conf->getArgParam("--" + prefix + "-recv-timeout",it.getProp("recv_timeout")));

    string saddr = conf->getArgParam("--" + prefix + "-my-addr",it.getProp("addr"));

    if( saddr.empty() )
        addr = 0x01;
    else
        addr = ModbusRTU::str2mbAddr(saddr);

    mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id",it.getProp("reg_from_id"));
    dinfo << myname << "(init): mbregFromID=" << mbregFromID << endl;

    respond_id = conf->getSensorID(conf->getArgParam("--" + prefix + "-respond-id",it.getProp("respond_id")));
    respond_invert = conf->getArgInt("--" + prefix + "-respond-invert",it.getProp("respond_invert"));

    timeout_t reply_tout = conf->getArgInt("--" + prefix + "-reply-timeout",it.getProp("replyTimeout"));
    if( reply_tout == 0 )
        reply_tout = 3000;

    timeout_t aftersend_pause = conf->getArgInt("--" + prefix + "-aftersend-pause",it.getProp("afterSendPause"));

    string stype = conf->getArgParam("--" + prefix + "-type",it.getProp("type"));

    if( stype == "RTU" )
    {
        // ---------- init RS ----------
        string dev    = conf->getArgParam("--" + prefix + "-dev",it.getProp("device"));
        if( dev.empty() )
            throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown device...");

        string speed     = conf->getArgParam("--" + prefix + "-speed",it.getProp("speed"));
        if( speed.empty() )
            speed = "38400";

        bool use485F = conf->getArgInt("--" + prefix + "-use485F",it.getProp("use485F"));
        bool transmitCtl = conf->getArgInt("--" + prefix + "-transmit-ctl",it.getProp("transmitCtl"));

        auto rs = make_shared<ModbusRTUSlaveSlot>(dev,use485F,transmitCtl);
        rs->setSpeed(speed);
        rs->setRecvTimeout(2000);
        rs->setAfterSendPause(aftersend_pause);
        rs->setReplyTimeout(reply_tout);
        rs->setLog(dlog());

        mbslot = std::static_pointer_cast<ModbusServerSlot>(rs);
        thr = make_shared< ThreadCreator<MBSlave> >(this,&MBSlave::execute_rtu);
        thr->setFinalAction(this,&MBSlave::finalThread);
        dinfo << myname << "(init): type=RTU myaddr=" << ModbusRTU::addr2str(addr)
            << " dev=" << dev << " speed=" << speed << endl;
    }
    else if( stype == "TCP" )
    {
        string iaddr = conf->getArgParam("--" + prefix + "-inet-addr",it.getProp("iaddr"));
        if( iaddr.empty() )
            throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown TCP server address. Use: --prefix-inet-addr [ XXX.XXX.XXX.XXX| hostname ]");

        int port = conf->getArgPInt("--" + prefix + "-inet-port",it.getProp("iport"), 502);

        dinfo << myname << "(init): type=TCP myaddr=" << ModbusRTU::addr2str(addr)
                << " inet=" << iaddr << " port=" << port << endl;

        ost::InetAddress ia(iaddr.c_str());
        auto mbtcp = make_shared<ModbusTCPServerSlot>(ia,port);

        mbtcp->setAfterSendPause(aftersend_pause);
        mbtcp->setReplyTimeout(reply_tout);

        mbslot = mbslot = std::static_pointer_cast<ModbusServerSlot>(mbtcp);
        thr = make_shared< ThreadCreator<MBSlave> >(this,&MBSlave::execute_tcp);
        thr->setFinalAction(this,&MBSlave::finalThread);
        dinfo << myname << "(init): init TCP connection ok. " << " inet=" << iaddr << " port=" << port << endl;

        if( dlog()->is_level9() )
            mbtcp->setLog(dlog());
    }
    else
        throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown slave type. Use: --mbs-type [RTU|TCP]");

    mbslot->connectReadCoil( sigc::mem_fun(this, &MBSlave::readCoilStatus) );
    mbslot->connectReadInputStatus( sigc::mem_fun(this, &MBSlave::readInputStatus) );
    mbslot->connectReadOutput( sigc::mem_fun(this, &MBSlave::readOutputRegisters) );
    mbslot->connectReadInput( sigc::mem_fun(this, &MBSlave::readInputRegisters) );
    mbslot->connectForceSingleCoil( sigc::mem_fun(this, &MBSlave::forceSingleCoil) );
    mbslot->connectForceCoils( sigc::mem_fun(this, &MBSlave::forceMultipleCoils) );
    mbslot->connectWriteOutput( sigc::mem_fun(this, &MBSlave::writeOutputRegisters) );
    mbslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBSlave::writeOutputSingleRegister) );
    mbslot->connectMEIRDI( sigc::mem_fun(this, &MBSlave::read4314) );

    if( findArgParam("--" + prefix + "-allow-setdatetime",conf->getArgc(),conf->getArgv())!=-1 )
        mbslot->connectSetDateTime( sigc::mem_fun(this, &MBSlave::setDateTime) );

    mbslot->connectDiagnostics( sigc::mem_fun(this, &MBSlave::diagnostics) );
    mbslot->connectFileTransfer( sigc::mem_fun(this, &MBSlave::fileTransfer) );

//    mbslot->connectJournalCommand( sigc::mem_fun(this, &MBSlave::journalCommand) );
//    mbslot->connectRemoteService( sigc::mem_fun(this, &MBSlave::remoteService) );
    // -------------------------------

    initPause = conf->getArgPInt("--" + prefix + "-initPause",it.getProp("initPause"), 3000);

    if( shm->isLocalwork() )
    {
        readConfiguration();
        dinfo << myname << "(init): iomap size = " << iomap.size() << endl;
    }
    else
    {
        ic->addReadItem( sigc::mem_fun(this,&MBSlave::readItem) );
        // при работе с SM через указатель принудительно включаем force
        force = true;
    }

    // ********** HEARTBEAT *************
    string heart = conf->getArgParam("--" + prefix + "-heartbeat-id",it.getProp("heartbeat_id"));
    if( !heart.empty() )
    {
        sidHeartBeat = conf->getSensorID(heart);
        if( sidHeartBeat == DefaultObjectId )
        {
            ostringstream err;
            err << myname << ": не найден идентификатор для датчика 'HeartBeat' " << heart;
            dcrit << myname << "(init): " << err.str() << endl;
            throw SystemError(err.str());
        }

        int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time",it.getProp("heartbeatTime"),conf->getHeartBeatTime());
        if( heartbeatTime )
            ptHeartBeat.setTiming(heartbeatTime);
        else
            ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

        maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max",it.getProp("heartbeat_max"), 10);
        test_id = sidHeartBeat;
    }
    else
    {
        test_id = conf->getSensorID("TestMode_S");
        if( test_id == DefaultObjectId )
        {
            ostringstream err;
            err << myname << ": test_id unknown. 'TestMode_S' not found...";
            dcrit << myname << "(init): " << err.str() << endl;
            throw SystemError(err.str());
        }
    }

    askcount_id = conf->getSensorID(conf->getArgParam("--" + prefix + "-askcount-id",it.getProp("askcount_id")));
    dinfo << myname << ": init askcount_id=" << askcount_id << endl;
    dinfo << myname << ": init test_id=" << test_id << endl;

    wait_msec = conf->getHeartBeatTime() - 100;
    if( wait_msec < 500 )
        wait_msec = 500;

    activateTimeout    = conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

    timeout_t msec = conf->getArgPInt("--" + prefix + "-timeout",it.getProp("timeout"), 3000);
    ptTimeout.setTiming(msec);

    dinfo << myname << "(init): rs-timeout=" << msec << " msec" << endl;

    // build file list...
    xmlNode* fnode = 0;
    const std::shared_ptr<UniXML> xml = conf->getConfXML();
    if( xml )
        fnode = xml->extFindNode(cnode,1,1,"filelist");

    if( fnode )
    {
        UniXML::iterator fit(fnode);
        if( fit.goChildren() )
        {
            for( ;fit.getCurrent(); fit.goNext() )
            {
                std::string nm = fit.getProp("name");
                if( nm.empty() )
                {
                    dwarn << myname << "(build file list): ignore empty name... " << endl;
                    continue;
                }
                int id = fit.getIntProp("id");
                if( id == 0 )
                {
                    dwarn << myname << "(build file list): FAILED ID for " << nm << "... ignore..." << endl;
                    continue;
                }

                std::string dir(fit.getProp("directory"));
                if( !dir.empty() )
                {
                    if( dir == "ConfDir" )
                        nm = conf->getConfDir() + nm;
                    else if( dir == "DataDir" )
                        nm = conf->getDataDir() + nm;
                    else
                        nm = dir + nm;
                }

                dinfo << myname << "(init):       add to filelist: "
                        << "id=" << id
                        << " file=" << nm
                        << endl;

                flist[id] = nm;
            }
        }
        else
            dinfo << myname << "(init): <filelist> empty..." << endl;
    }
    else
        dinfo << myname << "(init): <filelist> not found..." << endl;


    // Формирование "карты" ответов на запрос 0x2B(43)/0x0E(14)
    xmlNode* mnode = 0;
    if( xml )
        mnode = xml->extFindNode(cnode,1,1,"MEI");

    if( mnode )
    {
        // Считывается структура для формирования ответов на запрос 0x2B(43)/0x0E(14)
//        <MEI>
//               <device id="">
//                  <objects id="">
//                      <string id="" value=""/>
//                      <string id="" value=""/>
//                      <string id="" value=""/>
//                        ...
//                  </objects>
//               </device>
//               <device devID="">
//                 ...
//               </device>
//      </MEI>
        UniXML::iterator dit(mnode);
        if( dit.goChildren() )
        {
            // Device ID list..
            for( ;dit.getCurrent(); dit.goNext() )
            {
                if( dit.getProp("id").empty() )
                {
                    dwarn << myname << "(init): read <MEI>. Unknown <device id=''>. Ignore.." << endl;
                    continue;
                }

                int devID = dit.getIntProp("id");

                UniXML::iterator oit(dit);
                if( oit.goChildren() )
                {
                    dwarn << myname << "(init): MEI: read dev='" << devID << "'" << endl;
                    MEIObjIDMap meiomap;
                    // Object ID list..
                    for( ;oit.getCurrent(); oit.goNext() )
                    {
                        if( dit.getProp("id").empty() )
                        {
                            dwarn << myname
                                    << "(init): read <MEI>. Unknown <object id='' (for device id='"
                                    << devID << "'). Ignore.."
                                    << endl;

                            continue;
                        }

                        int objID = oit.getIntProp("id");
                        UniXML::iterator sit(oit);
                        if( sit.goChildren() )
                        {
                            dinfo << myname << "(init): MEI: read obj='" << objID << "'" << endl;
                            MEIValMap meivmap;
                            // request (string) list..
                            for( ;sit.getCurrent(); sit.goNext() )
                            {
                                int vid = objID;
                                if( sit.getProp("id").empty() )
                                {
                                    dwarn << myname << "(init): MEI: dev='" << devID
                                            << "' obj='" << objID << "'"
                                            << ". Unknown id='' for value='" << sit.getProp("value") << "'"
                                            << ". Set objID='" << objID << "'"
                                            << endl;
                                }
                                else
                                    vid = sit.getIntProp("id");

                                meivmap[vid] = sit.getProp("value");
                            }

                            if( !meivmap.empty() )
                                meiomap[objID] = meivmap;
                        }
                    }

                    if( !meiomap.empty() )
                        meidev[devID] = meiomap;
                }
            }
        }

        if( !meidev.empty() )
            dinfo << myname << "(init): <MEI> init ok." << endl;
    }
    else
        dinfo << myname << "(init): <MEI> empty..." << endl;

}
// -----------------------------------------------------------------------------
MBSlave::~MBSlave()
{
    cancelled = true;
    if( thr && thr->isRunning() )
    {
        thr->stop();
//        thr->join();
    }
}
// -----------------------------------------------------------------------------
void MBSlave::finalThread()
{
    activated = false;
    cancelled = true;
}
// -----------------------------------------------------------------------------
void MBSlave::waitSMReady()
{
    // waiting for SM is ready...
    int ready_timeout = uniset_conf()->getArgInt("--" + prefix + "-sm-ready-timeout","15000");
    if( ready_timeout == 0 )
        ready_timeout = 15000;
    else if( ready_timeout < 0 )
        ready_timeout = UniSetTimer::WaitUpTime;

    if( !shm->waitSMready(ready_timeout,50) )
    {
        ostringstream err;
        err << myname << "(waitSMReady): Не дождались готовности SharedMemory к работе в течение " << ready_timeout << " мсек";
        dcrit << err.str() << endl;
//        throw SystemError(err.str());
        raise(SIGTERM);
        terminate();
//        abort();
    }
}
// -----------------------------------------------------------------------------
void MBSlave::execute_rtu()
{
    auto rscomm = dynamic_pointer_cast<ModbusRTUSlaveSlot>(mbslot);

    ModbusRTU::mbErrCode prev = erNoError;

    while( !cancelled )
    {
        try
        {
            ModbusRTU::mbErrCode res = rscomm->receive( addr, wait_msec );

            if( res!=ModbusRTU::erTimeOut )
                ptTimeout.reset();

            // собираем статистику обмена
            if( prev!=ModbusRTU::erTimeOut )
            {
                //  с проверкой на переполнение
                askCount = askCount>=numeric_limits<long>::max() ? 0 : askCount+1;
                if( res!=ModbusRTU::erNoError )
                    ++errmap[res];
            }

            prev = res;

            if( res!=ModbusRTU::erNoError && res!=ModbusRTU::erTimeOut )
                dwarn << myname << "(execute_rtu): " << ModbusRTU::mbErr2Str(res) << endl;

            if( !activated )
                continue;

            if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
            {
                try
                {
                    shm->localSetValue(itHeartBeat,sidHeartBeat,maxHeartBeat,getId());
                    ptHeartBeat.reset();
                }
                catch( const Exception& ex )
                {
                    dcrit << myname
                          << "(execute_rtu): (hb) " << ex << std::endl;
                }
            }

            if( respond_id != DefaultObjectId )
            {
                bool state = ptTimeout.checkTime() ? false : true;
                if( respond_invert )
                    state ^= true;

                try
                {
                    shm->localSetValue(itRespond,respond_id,state,getId());
                }
                catch( const Exception& ex )
                {
                    dcrit << myname << "(execute_rtu): (respond) " << ex << std::endl;
                }
            }

            if( askcount_id!=DefaultObjectId )
            {
                try
                {
                    shm->localSetValue(itAskCount,askcount_id,askCount,getId());
                }
                catch( const Exception& ex )
                {
                    dcrit << myname << "(execute_rtu): (askCount) " << ex << std::endl;
                }
            }

            for( auto &it: iomap )
                IOBase::processingThreshold(&it.second,shm,force);
        }
        catch(...){}
    }
}
// -------------------------------------------------------------------------
void MBSlave::execute_tcp()
{
    auto sslot = dynamic_pointer_cast<ModbusTCPServerSlot>(mbslot);

    ModbusRTU::mbErrCode prev = erNoError;

    dinfo << myname << "(execute_tcp): thread running.." << endl;

    while( !cancelled )
    {
        try
        {
            ModbusRTU::mbErrCode res = sslot->receive( addr, wait_msec );

            if( res!=ModbusRTU::erTimeOut )
                ptTimeout.reset();

            // собираем статистику обмена
            if( prev!=ModbusRTU::erTimeOut )
            {
                //  с проверкой на переполнение
                askCount = askCount>=numeric_limits<long>::max() ? 0 : askCount+1;
                if( res!=ModbusRTU::erNoError )
                    ++errmap[res];
            }

            prev = res;

            if( res!=ModbusRTU::erNoError && res!=ModbusRTU::erTimeOut )
                dwarn << myname << "(execute_tcp): " << ModbusRTU::mbErr2Str(res) << endl;

            if( !activated )
                continue;

            if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
            {
                try
                {
                    shm->localSetValue(itHeartBeat,sidHeartBeat,maxHeartBeat,getId());
                    ptHeartBeat.reset();
                }
                catch( const Exception& ex )
                {
                    dcrit << myname << "(execute_tcp): (hb) " << ex << std::endl;
                }
            }

            if( respond_id != DefaultObjectId )
            {
                bool state = ptTimeout.checkTime() ? false : true;
                if( respond_invert )
                    state ^= true;
                try
                {
                    shm->localSetValue(itRespond,respond_id,state,getId());
                }
                catch( const Exception& ex )
                {
                    dcrit << myname
                          << "(execute_rtu): (respond) " << ex << std::endl;
                }
            }

            if( askcount_id!=DefaultObjectId )
            {
                try
                {
                    shm->localSetValue(itAskCount,askcount_id,askCount,getId());
                }
                catch( const Exception& ex )
                {
                    dcrit << myname
                          << "(execute_tcp): (askCount) " << ex << std::endl;
                }
            }

            for( auto &it: iomap )
                IOBase::processingThreshold(&it.second,shm,force);
        }
        catch( const std::exception& ex )
        {
            dcrit << myname << "(execute_tcp): " << ex.what() << endl;
        }
    }

    dinfo << myname << "(execute_tcp): thread stopped.." << endl;
}
// -------------------------------------------------------------------------
void MBSlave::sysCommand( const UniSetTypes::SystemMessage *sm )
{
    switch( sm->command )
    {
        case SystemMessage::StartUp:
        {
            if( iomap.empty() )
            {
                dcrit << myname << "(sysCommand): iomap EMPTY! terminated..." << endl;
                raise(SIGTERM);
                return;
            }

            waitSMReady();

            // подождать пока пройдёт инициализация датчиков
            // см. activateObject()
            msleep(initPause);
            PassiveTimer ptAct(activateTimeout);
            while( !activated && !ptAct.checkTime() )
            {
                cout << myname << "(sysCommand): wait activate..." << endl;
                msleep(300);
                if( activated )
                    break;
            }

            if( !activated )
            {
                dcrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
            }
            else
            {
                UniSetTypes::uniset_rwmutex_rlock l(mutex_start);
                askSensors(UniversalIO::UIONotify);
                thr->start();
            }
            break;
        }

        case SystemMessage::FoldUp:
        case SystemMessage::Finish:
            askSensors(UniversalIO::UIODontNotify);
            break;

        case SystemMessage::WatchDog:
        {
            // ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
            // Если идёт локальная работа
            // (т.е. MBSlave  запущен в одном процессе с SharedMemory2)
            // то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
            // при заказе датчиков, а если SM вылетит, то вместе с этим процессом(MBSlave)
            if( shm->isLocalwork() )
                break;

        }
        break;

        case SystemMessage::LogRotate:
        {
            // переоткрываем логи
            ulogany << myname << "(sysCommand): logRotate" << std::endl;
            string fname(ulog()->getLogFile());
            if( !fname.empty() )
            {
                ulog()->logFile(fname,true);
                ulogany << myname << "(sysCommand): ***************** ulog LOG ROTATE *****************" << std::endl;
            }

            dlogany << myname << "(sysCommand): logRotate" << std::endl;
            fname = dlog()->getLogFile();
            if( !fname.empty() )
            {
                dlog()->logFile(fname,true);
                dlogany << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
            }
        }
        break;

        default:
            break;
    }
}
// ------------------------------------------------------------------------------------------
void MBSlave::askSensors( UniversalIO::UIOCommand cmd )
{
    if( !shm->waitSMworking(test_id,activateTimeout,50) )
    {
        ostringstream err;
        err << myname
            << "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение "
            << activateTimeout << " мсек";

        dcrit << err.str() << endl;
        kill(SIGTERM,getpid());    // прерываем (перезапускаем) процесс...
        throw SystemError(err.str());
    }

    if( force )
        return;


    for( auto &it: iomap )
    {
        IOProperty* p(&it.second);
        try
        {
            shm->askSensor(p->si.id,cmd);
        }
        catch( UniSetTypes::Exception& ex )
        {
            dwarn << myname << "(askSensors): " << ex << std::endl;
        }
        catch(...){}
    }
}
// ------------------------------------------------------------------------------------------
void MBSlave::sensorInfo( const UniSetTypes::SensorMessage* sm )
{
    for( auto it=iomap.begin(); it!=iomap.end(); ++it )
    {
        if( it->second.si.id == sm->id )
        {
            IOProperty* p(&it->second);
            if( p->stype == UniversalIO::DO ||
                p->stype == UniversalIO::DI )
            {
                uniset_rwmutex_wrlock lock(p->val_lock);
                p->value = sm->value ? 1 : 0;
            }
            else if( p->stype == UniversalIO::AO ||
                    p->stype == UniversalIO::AI )
            {
                uniset_rwmutex_wrlock lock(p->val_lock);
                p->value = sm->value;
            }

            int sz = VTypes::wsize(p->vtype);
            if( sz < 1 )
                return;

            // если размер больше одного слова
            // то надо обновить значение "везде"
            // они если "всё верно инициализировано" идут подряд
            int i=0;
            for( ;i<sz && it!=iomap.end(); i++,it++ )
            {
                p  = &it->second;
                if( p->si.id == sm->id )
                    p->value = sm->value;
            }

            // вообще этого не может случиться
            // потому-что корректность проверяется при загрузке
            if( i != sz )
                dcrit << myname << "(sensorInfo): update failed for sid=" << sm->id
                        << " (i=" << i << " sz=" << sz << ")" << endl;

            return;
        }
    }
}
// ------------------------------------------------------------------------------------------
bool MBSlave::activateObject()
{
    // блокирование обработки Starsp
    // пока не пройдёт инициализация датчиков
    // см. sysCommand()
    {
        activated = false;
        UniSetTypes::uniset_rwmutex_wrlock l(mutex_start);
        UniSetObject_LT::activateObject();
        initIterators();
        activated = true;
    }

    return true;
}
// ------------------------------------------------------------------------------------------
bool MBSlave::deactivateObject()
{
    dinfo << myname << "(deactivateObject): ..." << endl;

    activated = false;
    cancelled = true;
    try
    {
        if( mbslot )
            mbslot->sigterm(SIGTERM);
    }
    catch(...){}

    return UniSetObject_LT::deactivateObject();
}
// ------------------------------------------------------------------------------------------
void MBSlave::sigterm( int signo )
{
    dinfo << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
    activated = false;
    cancelled = true;
    try
    {
        if( mbslot )
            mbslot->sigterm(signo);
    }
    catch(...){}

    UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void MBSlave::readConfiguration()
{
//    readconf_ok = false;
    xmlNode* root = uniset_conf()->getXMLSensorsSection();
    if(!root)
    {
        ostringstream err;
        err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
        throw SystemError(err.str());
    }

    UniXML::iterator it(root);
    if( !it.goChildren() )
    {
        dcrit << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
        return;
    }

    for( ;it.getCurrent(); it.goNext() )
    {
        if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
            initItem(it);
    }

//    readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool MBSlave::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
    if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
        initItem(it);
    return true;
}

// ------------------------------------------------------------------------------------------
bool MBSlave::initItem( UniXML::iterator& it )
{
    IOProperty p;
    
    if( !IOBase::initItem( static_cast<IOBase*>(&p),it,shm,prop_prefix,false,dlog(),myname) )
        return false;

    if( mbregFromID )
        p.mbreg = p.si.id;
    else
    {
        string r = IOBase::initProp(it,"mbreg",prop_prefix,false);
        if( r.empty() )
        {
            dcrit << myname << "(initItem): Unknown '" << prop_prefix << "mbreg' for " << it.getProp("name") << endl;
            return false;
        }

        p.mbreg = ModbusRTU::str2mbData(r);
    }

    p.amode = MBSlave::amRW;
    string am(IOBase::initProp(it,"accessmode",prop_prefix,false));
    if( am == "ro" )
        p.amode = MBSlave::amRO;
    else if( am == "wo" )
        p.amode = MBSlave::amWO;

    int nbit = IOBase::initIntProp(it,"nbit",prop_prefix,false,-1);
    if( nbit != -1 )
    {
        if( nbit<0 || nbit >= ModbusRTU::BitsPerData )
        {
            dcrit << myname << "(initItem): BAD " << prop_prefix << "nbit=" << nbit << ". Must be  0 <= nbit < " << ModbusRTU::BitsPerData
                  << " for '"
                  << it.getProp("name")
                  << "'" << endl;
            return false;
        }

        auto i = iomap.find(p.mbreg);
        if( i != iomap.end() )
        {
            if( !i->second.bitreg )
            {
                dcrit << myname << "(initItem): BAD USE " << prop_prefix << "nbit=" << nbit
                      << " (for '"
                      << it.getProp("name")
                      << "') SENSOR ALREADY ADDED sid='" << i->second.si.id << "'"
                      << "(" << uniset_conf()->oind->getMapName(i->second.si.id) << ")"
                      << endl;
                return false;
            }

            if( i->second.bitreg->check(p.si) )
            {
                dcrit << myname << "(initItem): BIT " << prop_prefix << "nbit=" << nbit
                      << " (for "
                      << it.getProp("name")
                      << ") ALREADY IN USE for sid='" << i->second.bitreg->bvec[nbit].si.id << "'"
                      << "(" << uniset_conf()->oind->getMapName(i->second.bitreg->bvec[nbit].si.id) << ")"
                      << endl;
                return false;
            }

            i->second.bitreg->bvec[nbit] = std::move(p);
            return true;
        }
        else
        {
            ModbusData mbreg = p.mbreg;
            IOProperty p_dummy;
            p_dummy.bitreg = make_shared<BitRegProperty>();
            p_dummy.bitreg->mbreg = mbreg;

            p.vtype = VTypes::vtUnknown;
            p.wnum = 0;

            p_dummy.bitreg->bvec[nbit] = std::move(p);
            dinfo << myname << "(initItem): add bit register: " << p_dummy.bitreg.get() << endl;
            iomap[mbreg] = std::move(p_dummy);
        }
        return true;
    }

    string vt(IOBase::initProp(it,"vtype",prop_prefix,false));
    if( vt.empty() )
    {
        p.vtype = VTypes::vtUnknown;
        p.wnum = 0;
        dinfo << myname << "(initItem): add " << p << endl;
        iomap[p.mbreg] = std::move(p);
    }
    else
    {
        VTypes::VType v(VTypes::str2type(vt));
        if( v == VTypes::vtUnknown )
        {
            dcrit << myname << "(initItem): Unknown rtuVType=" << vt << " for "
                  << it.getProp("name")
                  << endl;

            return false;
        }
        else if( v == VTypes::vtByte )
        {
            p.nbyte = IOBase::initIntProp(it,"nbyte",prop_prefix,false);
            if( p.nbyte <=0 )
            {
                dcrit << myname << "(initItem): Unknown " << prop_prefix << "nbyte='' for "
                      << it.getProp("name")
                      << endl;
                return false;
            }
            else if( p.nbyte > 2 )
            {
                dcrit << myname << "(initItem): BAD " << prop_prefix << "nbyte='" << p.nbyte << "' for "
                      << it.getProp("name")
                      << ". Must be [1,2]."
                      << endl;
                return false;
            }
        }

        p.vtype = v;
        p.wnum = 0;
        for( auto i=0; i<VTypes::wsize(p.vtype); i++ )
        {
            p.mbreg += i;
            p.wnum+= i;
            dinfo << myname << "(initItem): add " << p << endl;
            iomap[p.mbreg] = std::move(p);
        }
    }

    return true;
}
// ------------------------------------------------------------------------------------------
bool MBSlave::BitRegProperty::check( const IOController_i::SensorInfo& si )
{
    for( auto& i: bvec )
    {
        if( i.si.id == si.id && i.si.node == si.node )
            return true;
    }

    return false;
}
// ------------------------------------------------------------------------------------------
void MBSlave::initIterators()
{
    auto it=iomap.begin();
    for( ; it!=iomap.end(); ++it )
    {
        shm->initIterator(it->second.ioit);
        if( it->second.bitreg )
        {
            auto b = it->second.bitreg;
            for( auto i=b->bvec.begin(); i!= b->bvec.end(); ++ i )
                shm->initIterator(i->ioit);
        }
    }


    shm->initIterator(itHeartBeat);
    shm->initIterator(itAskCount);
    shm->initIterator(itRespond);
}
// -----------------------------------------------------------------------------
void MBSlave::help_print( int argc, const char* const* argv )
{
    cout << "Default: prefix='mbs'" << endl;
    cout << "--prefix-name  name        - ObjectID. По умолчанию: MBSlave1" << endl;
    cout << "--prefix-confnode cnode    - Возможность задать настроечный узел в configure.xml. По умолчанию: name" << endl;
    cout << endl;
    cout << "--prefix-reg-from-id 0,1   - Использовать в качестве регистра sensor ID" << endl;
    cout << "--prefix-filter-field name - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
    cout << "--prefix-filter-value val  - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
    cout << "--prefix-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
    cout << "--prefix-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
    cout << "--prefix-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
    cout << "--prefix-initPause         - Задержка перед инициализацией (время на активизация процесса)" << endl;
    cout << "--prefix-force 1           - Читать данные из SM каждый раз, а не по изменению." << endl;
    cout << "--prefix-respond-id - respond sensor id" << endl;
    cout << "--prefix-respond-invert [0|1]    - invert respond logic" << endl;
    cout << "--prefix-sm-ready-timeout        - время на ожидание старта SM" << endl;
    cout << "--prefix-timeout msec            - timeout for check link" << endl;
    cout << "--prefix-after-send-pause msec   - принудительная пауза после посылки ответа. По умолчанию: 0" << endl;
    cout << "--prefix-reply-timeout msec      - Контрольное время для формирования ответа. " << endl
         << "                                   Если обработка запроса превысит это время, ответ не будет послан (timeout)." << endl
         << "                                   По умолчанию: 3 сек" << endl;
    cout << "--prefix-set-prop-prefix [val]   - Использовать для свойств указанный или пустой префикс." << endl;

    cout << "--prefix-allow-setdatetime - On set date and time (0x50) modbus function" << endl;
    cout << "--prefix-my-addr      - адрес текущего узла" << endl;
    cout << "--prefix-type [RTU|TCP] - modbus server type." << endl;

    cout << " Настройки протокола RTU: " << endl;
    cout << "--prefix-dev devname  - файл устройства" << endl;
    cout << "--prefix-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;

    cout << " Настройки протокола TCP: " << endl;
    cout << "--prefix-inet-addr [xxx.xxx.xxx.xxx | hostname ]  - this modbus server address" << endl;
    cout << "--prefix-inet-port num - this modbus server port. Default: 502" << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<MBSlave> MBSlave::init_mbslave( int argc, const char* const* argv, UniSetTypes::ObjectId icID,
                                                const std::shared_ptr<SharedMemory> ic, const string& prefix )
{
    auto conf = uniset_conf();
    string name = conf->getArgParam("--" + prefix + "-name","MBSlave1");
    if( name.empty() )
    {
        cerr << "(mbslave): Не задан name'" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);
    if( ID == UniSetTypes::DefaultObjectId )
    {
        cerr << "(mbslave): идентификатор '" << name
            << "' не найден в конф. файле!"
            << " в секции " << conf->getObjectsSection() << endl;
        return 0;
    }

    dinfo << "(mbslave): name = " << name << "(" << ID << ")" << endl;
    return make_shared<MBSlave>(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBSlave::BitRegProperty* p )
{
    return os << (*p);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBSlave::BitRegProperty& p )
{
    os  << " reg=" << ModbusRTU::dat2str(p.mbreg) << "(" << (int)p.mbreg << ")[ ";
    for( auto& i: p.bvec )
        os << "'" << i.si.id << "' ";
    os << "]";

    return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBSlave::IOProperty& p )
{
    os  << " reg=" << ModbusRTU::dat2str(p.mbreg) << "(" << (int)p.mbreg << ")"
        << " sid=" << p.si.id
        << " stype=" << p.stype
        << " wnum=" << p.wnum
        << " nbyte=" << p.nbyte
        << " safety=" << p.safety
        << " invert=" << p.invert;

    if( p.stype == UniversalIO::AI || p.stype == UniversalIO::AO )
    {
        os     << p.cal
            << " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
    }

    return os;
}
// -----------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readOutputRegisters( ModbusRTU::ReadOutputMessage& query, 
                                                    ModbusRTU::ReadOutputRetMessage& reply )
{
    dinfo << myname << "(readOutputRegisters): " << query << endl;

    if( query.count <= 1 )
    {
        ModbusRTU::ModbusData d = 0;
        ModbusRTU::mbErrCode ret = real_read(query.start,d);
        if( ret == ModbusRTU::erNoError )
            reply.addData(d);
        else
            reply.addData(0);
        return ret;
    }

    // Фомирование ответа:
    ModbusRTU::mbErrCode ret = much_real_read(query.start,buf,query.count);
    if( ret == ModbusRTU::erNoError )
    {
        for( unsigned int i=0; i<query.count; i++ )
            reply.addData( buf[i] );
    }

    return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
                                            ModbusRTU::WriteOutputRetMessage& reply )
{
    dinfo << myname << "(writeOutputRegisters): " << query << endl;

    // Формирование ответа:
    ModbusRTU::mbErrCode ret = much_real_write(query.start,query.data,query.quant);
    if( ret == ModbusRTU::erNoError )
            reply.set(query.start,query.quant);
    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
                                            ModbusRTU::WriteSingleOutputRetMessage& reply )
{
    dinfo << myname << "(writeOutputSingleRegisters): " << query << endl;

    ModbusRTU::mbErrCode ret = real_write(query.start,query.data);
    if( ret == ModbusRTU::erNoError )
        reply.set(query.start,query.data);

    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::much_real_write( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, 
                        int count )
{
    dinfo << myname << "(much_real_write): read mbID="
            << ModbusRTU::dat2str(reg) << "(" << (int)reg << ")" << " count=" << count << endl;


    int i=0;
    auto it = iomap.end();
    for( ; i<count; i++ )
    {
        it = iomap.find(reg+i);
        if( it != iomap.end() )
        {
            reg += i;
            break;
        }
    }

    if( it == iomap.end() )
        return ModbusRTU::erBadDataAddress;

    for( ; (it!=iomap.end()) && (i<count); i++,reg++ )
    {
        if( it->first == reg )
        {
            real_write_it(it,dat,i,count);
            --i; // т.к. внутри real_write_it будет сделан ++
            ++it;
        }
    }

    return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_write(  ModbusRTU::ModbusData reg, ModbusRTU::ModbusData val )
{
    ModbusRTU::ModbusData dat[1] = {val};
    int i=0;
    return real_write(reg,dat,i,1);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_write( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, int &i, int count )
{
    ModbusRTU::ModbusData mbval = dat[i];

    dinfo << myname << "(write): save mbID="
            << ModbusRTU::dat2str(reg)
            << " data=" << ModbusRTU::dat2str(mbval)
            << "(" << (int)mbval << ")" << endl;

    auto it = iomap.find(reg);
    return real_write_it(it,dat,i,count);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_write_it( IOMap::iterator& it, ModbusRTU::ModbusData* dat, int &i, int count )
{
    if( it == iomap.end() )
        return ModbusRTU::erBadDataAddress;

    ModbusRTU::ModbusData mbval = dat[i++];
    try
    {
        IOProperty* p(&it->second);

        if( p->amode == MBSlave::amRO )
            return ModbusRTU::erBadDataAddress;

        if( p->vtype == VTypes::vtUnknown )
        {
            if( p->stype == UniversalIO::DI ||
                p->stype == UniversalIO::DO )
            {
                IOBase::processingAsDI( p, mbval, shm, force );
            }
            else
            {
                long val = (signed short)(mbval);
                IOBase::processingAsAI( p, val, shm, force );
            }
            return erNoError;
        }
        else if( p->vtype == VTypes::vtUnsigned )
        {
            long val = (unsigned short)(mbval);
            IOBase::processingAsAI( p, val, shm, force );
        }
        else if( p->vtype == VTypes::vtSigned )
        {
            long val = (signed short)(mbval);
            IOBase::processingAsAI( p, val, shm, force );
        }
        else if( p->vtype == VTypes::vtI2 )
        {
            if( (i + VTypes::I2::wsize() - 1) > count )
            {
                i += VTypes::I2::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::I2::wsize()];
            for( int k=0; k<VTypes::I2::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::I2 i2(d,VTypes::I2::wsize());
            delete[] d;
            IOBase::processingAsAI( p, (long)i2, shm, force );
        }
        else if( p->vtype == VTypes::vtI2r )
        {
            if( (i + VTypes::I2r::wsize() - 1) > count )
            {
                i += VTypes::I2r::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::I2r::wsize()];
            for( int k=0; k<VTypes::I2r::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::I2r i2r(d,VTypes::I2r::wsize());
            delete[] d;
            IOBase::processingAsAI( p, (long)i2r, shm, force );
        }
        else if( p->vtype == VTypes::vtU2 )
        {
            if( (i + VTypes::U2::wsize() - 1) > count )
            {
                i += VTypes::U2::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::U2::wsize()];
            for( int k=0; k<VTypes::U2::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::U2 u2(d,VTypes::U2::wsize());
            delete[] d;
            IOBase::processingAsAI( p, (unsigned long)u2, shm, force );
        }
        else if( p->vtype == VTypes::vtU2r )
        {
            if( (i + VTypes::U2r::wsize() - 1) > count )
            {
                i += VTypes::U2r::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::U2r::wsize()];
            for( int k=0; k<VTypes::U2r::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::U2r u2r(d,VTypes::U2r::wsize());
            delete[] d;
            IOBase::processingAsAI( p, (unsigned long)u2r, shm, force );
        }
        else if( p->vtype == VTypes::vtF2 )
        {
            if( (i + VTypes::F2::wsize() - 1) > count )
            {
                i += VTypes::F2::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::F2::wsize()];
            for( int k=0; k<VTypes::F2::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::F2 f2(d,VTypes::F2::wsize());
            delete[] d;
            IOBase::processingFasAI( p, (float)f2, shm, force );
        }
        else if( p->vtype == VTypes::vtF2r )
        {
            if( (i + VTypes::F2r::wsize() - 1) > count )
            {
                i += VTypes::F2r::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::F2r::wsize()];
            for( int k=0; k<VTypes::F2r::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::F2r f2r(d,VTypes::F2r::wsize());
            delete[] d;
            IOBase::processingFasAI( p, (float)f2r, shm, force );
        }
        else if( p->vtype == VTypes::vtF4 )
        {
            if( (i + VTypes::F4::wsize() - 1) > count )
            {
                i += VTypes::F4::wsize();
                return ModbusRTU::erInvalidFormat;
            }

            ModbusRTU::ModbusData* d = new ModbusRTU::ModbusData[VTypes::F4::wsize()];
            for( int k=0; k<VTypes::F4::wsize(); k++, i++ )
                 d[k] = dat[i-1];

            VTypes::F4 f4(d,VTypes::F4::wsize());
            delete[] d;
            IOBase::processingFasAI( p, (float)f4, shm, force );
        }
        else if( p->vtype == VTypes::vtByte )
        {
            VTypes::Byte b(mbval);
            IOBase::processingAsAI( p, b.raw.b[p->nbyte-1], shm, force );
        }
/*
        if( p->stype == UniversalIO::DI ||
            p->stype == UniversalIO::DO )
        {
            bool set = val ? true : false;
            IOBase::processingAsDI(p,set,shm,force);
        }
        else if( p->stype == UniversalIO::AI ||
                 p->stype == UniversalIO::AO )
        {
            IOBase::processingAsAI( p, val, shm, force );
        }
*/
        pingOK = true;
        return ModbusRTU::erNoError;
    }
    catch( UniSetTypes::NameNotFound& ex )
    {
        dwarn << myname << "(write): " << ex << endl;
        return ModbusRTU::erBadDataAddress;
    }
    catch( UniSetTypes::OutOfRange& ex )
    {
        dwarn << myname << "(write): " << ex << endl;
        return ModbusRTU::erBadDataValue;
    }
    catch( const Exception& ex )
    {
        if( pingOK )
            dcrit << myname << "(write): " << ex << endl;
    }
    catch( const CORBA::SystemException& ex )
    {
        if( pingOK )
            dcrit << myname << "(write): СORBA::SystemException: "
                << ex.NP_minorString() << endl;
    }
    catch(...)
    {
        if( pingOK )
            dcrit << myname << "(write) catch ..." << endl;
    }

    pingOK = false;
    return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::much_real_read( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, 
                        int count )
{
    dinfo << myname << "(much_real_read): read mbID="
            << ModbusRTU::dat2str(reg) << "(" << (int)reg << ") " << " count=" << count << endl;

    auto it = iomap.end();
    int i=0;
    for( ; i<count; i++ )
    {
        it = iomap.find(reg+i);
        if( it != iomap.end() )
        {
            reg += i;
            break;
        }

        dat[i] = 0;
    }

    if( it == iomap.end() )
        return ModbusRTU::erBadDataAddress;

    ModbusRTU::ModbusData val=0;
    for( ; (it!=iomap.end()) && (i<count); i++,reg++ )
    {
        val=0;
        // если регистры идут не подряд, то просто вернём ноль
        if( it->first == reg )
        {
            real_read_it(it,val);
            ++it;
        }
        dat[i] = val;
    }

    // добиваем нулями "ответ"
    // чтобы он был такой длинны, которую запрашивали
    if( i<count )
    {
        for( ; i<count; i++ )
            dat[i] = 0;
    }

    return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_read( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData& val )
{
    dinfo << myname << "(real_read): read mbID="
            << ModbusRTU::dat2str(reg) << "(" << (int)reg << ")" << endl;

    auto it = iomap.find(reg);
    return real_read_it(it,val);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_read_it( IOMap::iterator& it, ModbusRTU::ModbusData& val )
{
    if( it == iomap.end() )
        return ModbusRTU::erBadDataAddress;

    dinfo << myname << "(real_read_it): read mbID="
            << ModbusRTU::dat2str(it->first) << "(" << (int)it->first << ")" << endl;

    IOProperty* p(&it->second);

    if( p->bitreg )
        return real_bitreg_read_it(p->bitreg,val);

    return real_read_prop(p,val);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_bitreg_read_it( std::shared_ptr<BitRegProperty>& bp, ModbusRTU::ModbusData& val )
{
    dinfo << myname << "(real_bitreg_read_it): read mbID="
            << ModbusRTU::dat2str(bp->mbreg) << "(" << (int)bp->mbreg << ")" << endl;

    ModbusRTU::DataBits16 d;

    for( int i=0; i<ModbusRTU::BitsPerData; i++ )
    {
        IOProperty* p(&(bp->bvec[i]));

        if( p->si.id == DefaultObjectId )
        {
            d.set(i,0);
            continue;
        }

        ModbusRTU::ModbusData v = 0;
        ModbusRTU::mbErrCode err = real_read_prop(p,v);
        if( err == ModbusRTU::erNoError )
            d.set(i,(bool)v);
        else
            d.set(i,0);
    }

    val = d.mdata();
    return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_read_prop( IOProperty* p, ModbusRTU::ModbusData& val )
{
    try
    {
        val = 0;

        if( p->amode == MBSlave::amWO )
            return ModbusRTU::erBadDataAddress;

        if( p->stype == UniversalIO::DI ||
            p->stype == UniversalIO::DO )
        {
            val = IOBase::processingAsDO(p,shm,force) ? 1 : 0;
        }
        else if( p->stype == UniversalIO::AI ||
                p->stype == UniversalIO::AO )
        {
            if( p->vtype == VTypes::vtUnknown )
            {
                val = IOBase::processingAsAO(p,shm,force);
            }
            else if( p->vtype == VTypes::vtF2 )
            {
                float f = IOBase::processingFasAO(p,shm,force);
                VTypes::F2 f2(f);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < f4.wsize()
                val = f2.raw.v[p->wnum];
            }
            else if( p->vtype == VTypes::vtF2r )
            {
                float f = IOBase::processingFasAO(p,shm,force);
                VTypes::F2r f2(f);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < f4.wsize()
                val = f2.raw_backorder.v[p->wnum];
            }
            else if( p->vtype == VTypes::vtF4 )
            {
                float f = IOBase::processingFasAO(p,shm,force);
                VTypes::F4 f4(f);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < f4.wsize()
                val = f4.raw.v[p->wnum];
            }
            else if( p->vtype == VTypes::vtI2 )
            {
                long v = IOBase::processingAsAO(p,shm,force);
                VTypes::I2 i2(v);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < i2.wsize()
                val = i2.raw.v[p->wnum];
            }
            else if( p->vtype == VTypes::vtI2r )
            {
                long v = IOBase::processingAsAO(p,shm,force);
                VTypes::I2r i2(v);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < i2.wsize()
                val = i2.raw_backorder.v[p->wnum];
            }
            else if( p->vtype == VTypes::vtU2 )
            {
                long v = IOBase::processingAsAO(p,shm,force);
                VTypes::U2 u2(v);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < u2.wsize()
                val = u2.raw.v[p->wnum];
            }
            else if( p->vtype == VTypes::vtU2r )
            {
                long v = IOBase::processingAsAO(p,shm,force);
                VTypes::U2r u2(v);
                // оптимизируем и проверку не делаем
                // считая, что при "загрузке" всё было правильно
                // инициализировано
                // if( p->wnum >=0 && p->wnum < u2.wsize()
                val = u2.raw_backorder.v[p->wnum];
            }
            else
                val = IOBase::processingAsAO(p,shm,force);
        }
        else
            return ModbusRTU::erBadDataAddress;

        pingOK = true;
        return ModbusRTU::erNoError;
    }
    catch( UniSetTypes::NameNotFound& ex )
    {
        dwarn << myname << "(real_read_it): " << ex << endl;
        return ModbusRTU::erBadDataAddress;
    }
    catch( UniSetTypes::OutOfRange& ex )
    {
        dwarn << myname << "(real_read_it): " << ex << endl;
        return ModbusRTU::erBadDataValue;
    }
    catch( const Exception& ex )
    {
        if( pingOK )
            dcrit << myname << "(real_read_it): " << ex << endl;
    }
    catch( const CORBA::SystemException& ex )
    {
        if( pingOK )
            dcrit << myname << "(real_read_it): CORBA::SystemException: "
                << ex.NP_minorString() << endl;
    }
    catch(...)
    {
        if( pingOK )
            dcrit << myname << "(real_read_it) catch ..." << endl;
    }

    pingOK = false;
    return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------

mbErrCode MBSlave::readInputRegisters( ReadInputMessage& query, ReadInputRetMessage& reply )
{
    dinfo << myname << "(readInputRegisters): " << query << endl;

    if( query.count <= 1 )
    {
        ModbusRTU::ModbusData d = 0;
        ModbusRTU::mbErrCode ret = real_read(query.start,d);
        if( ret == ModbusRTU::erNoError )
            reply.addData(d);
        else
            reply.addData(0);

        return ret;
    }

    // Фомирование ответа:
    ModbusRTU::mbErrCode ret = much_real_read(query.start,buf,query.count);
    if( ret == ModbusRTU::erNoError )
    {
        for( unsigned int i=0; i<query.count; i++ )
            reply.addData( buf[i] );
    }

    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::setDateTime( ModbusRTU::SetDateTimeMessage& query, 
                                    ModbusRTU::SetDateTimeRetMessage& reply )
{
    return ModbusServer::replySetDateTime(query,reply,dlog());
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::remoteService( ModbusRTU::RemoteServiceMessage& query, 
                                    ModbusRTU::RemoteServiceRetMessage& reply )
{
//    cerr << "(remoteService): " << query << endl;
    return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::fileTransfer( ModbusRTU::FileTransferMessage& query, 
                                    ModbusRTU::FileTransferRetMessage& reply )
{
    dinfo << myname << "(fileTransfer): " << query << endl;

    auto it = flist.find(query.numfile);
    if( it == flist.end() )
        return ModbusRTU::erBadDataValue;

    std::string fname(it->second);
    return ModbusServer::replyFileTransfer( fname,query,reply,dlog() );
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readCoilStatus( ReadCoilMessage& query,
                                                ReadCoilRetMessage& reply )
{
    dinfo << myname << "(readCoilStatus): " << query << endl;

    try
    {
        if( query.count <= 1 )
        {
            ModbusRTU::ModbusData d = 0;
            ModbusRTU::mbErrCode ret = real_read(query.start,d);
            reply.addData(0);
            if( ret == ModbusRTU::erNoError )
                reply.setBit(0,0,d);
            else
                reply.setBit(0,0,0);

            pingOK = true;
            return ret;
        }

        much_real_read(query.start,buf,query.count);
        int bnum = 0;
        unsigned int i=0;
        while( i<query.count )
        {
            reply.addData(0);
            for( auto nbit=0; nbit<BitsPerByte && i<query.count; nbit++,i++ )
                reply.setBit(bnum,nbit,(bool)(buf[i]));
            bnum++;
        }

        pingOK = true;
        return ModbusRTU::erNoError;
    }
    catch( UniSetTypes::NameNotFound& ex )
    {
        dwarn << myname << "(readCoilStatus): " << ex << endl;
        return ModbusRTU::erBadDataAddress;
    }
    catch( const Exception& ex )
    {
        if( pingOK )
            dcrit << myname << "(readCoilStatus): " << ex << endl;
    }
    catch( const CORBA::SystemException& ex )
    {
        if( pingOK )
            dcrit << myname << "(readCoilStatus): СORBA::SystemException: "
                << ex.NP_minorString() << endl;
    }
    catch(...)
    {
        if( pingOK )
            dcrit << myname << "(readCoilStatus): catch ..." << endl;
    }

    pingOK = false;
    return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readInputStatus( ReadInputStatusMessage& query, 
                                                ReadInputStatusRetMessage& reply )
{
    dinfo << myname << "(readInputStatus): " << query << endl;

    try
    {
        if( query.count <= 1 )
        {
            ModbusRTU::ModbusData d = 0;
            ModbusRTU::mbErrCode ret = real_read(query.start,d);
            reply.addData(0);
            if( ret == ModbusRTU::erNoError )
                reply.setBit(0,0,d);
            else
                reply.setBit(0,0,0);

            pingOK = true;
            return ret;
        }

        much_real_read(query.start,buf,query.count);
        int bnum = 0;
        unsigned int i=0;
        while( i<query.count )
        {
            reply.addData(0);
            for( auto nbit=0; nbit<BitsPerByte && i<query.count; nbit++,i++ )
                reply.setBit(bnum,nbit,(bool)(buf[i]));
            bnum++;
        }

        pingOK = true;
        return ModbusRTU::erNoError;
    }
    catch( UniSetTypes::NameNotFound& ex )
    {
        dwarn << myname << "(readInputStatus): " << ex << endl;
        return ModbusRTU::erBadDataAddress;
    }
    catch( const Exception& ex )
    {
        if( pingOK )
            dcrit << myname << "(readInputStatus): " << ex << endl;
    }
    catch( const CORBA::SystemException& ex )
    {
        if( pingOK )
            dcrit << myname << "(readInputStatus): СORBA::SystemException: "
                << ex.NP_minorString() << endl;
    }
    catch(...)
    {
        if( pingOK )
            dcrit << myname << "(readInputStatus): catch ..." << endl;
    }

    pingOK = false;
    return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
                                                    ModbusRTU::ForceCoilsRetMessage& reply )
{
    dinfo << myname << "(forceMultipleCoils): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    int nbit = 0;
    for( unsigned int i = 0; i<query.bcnt; i++ )
    {
        ModbusRTU::DataBits b(query.data[i]);
        for( auto k=0; k<ModbusRTU::BitsPerByte && nbit<query.quant; k++, nbit++ )
        {
            // ModbusRTU::mbErrCode ret =
            real_write(query.start+nbit, (b[k] ? 1 : 0) );
            //if( ret == ModbusRTU::erNoError )
        }
    }

    //if( ret == ModbusRTU::erNoError )
    if( nbit == query.quant )
        reply.set(query.start,query.quant);

    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
                                            ModbusRTU::ForceSingleCoilRetMessage& reply )
{
    dinfo << myname << "(forceSingleCoil): " << query << endl;

    ModbusRTU::mbErrCode ret = real_write(query.start, (query.cmd() ? 1 : 0) );
    if( ret == ModbusRTU::erNoError )
        reply.set(query.start,query.data);

    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::diagnostics( ModbusRTU::DiagnosticMessage& query, 
                                            ModbusRTU::DiagnosticRetMessage& reply )
{
    if( query.subf == ModbusRTU::subEcho )
    {
        reply = query;
        return ModbusRTU::erNoError;
    }

    if( query.subf == ModbusRTU::dgBusErrCount )
    {
        reply = query;
        reply.data[0] = errmap[ModbusRTU::erBadCheckSum];
        return ModbusRTU::erNoError;
    }

    if( query.subf == ModbusRTU::dgMsgSlaveCount || query.subf == ModbusRTU::dgBusMsgCount )
    {
        reply = query;
        reply.data[0] = askCount;
        return ModbusRTU::erNoError;
    }

    if( query.subf == ModbusRTU::dgSlaveNAKCount )
    {
        reply = query;
        reply.data[0] = errmap[erOperationFailed];
        return ModbusRTU::erNoError;
    }

    if( query.subf == ModbusRTU::dgClearCounters )
    {
        askCount = 0;
        errmap[erOperationFailed] = 0;
        errmap[ModbusRTU::erBadCheckSum] = 0;
        // другие счётчики пока не сбрасываем..
        reply = query;
        return ModbusRTU::erNoError;
    }

    return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::read4314( ModbusRTU::MEIMessageRDI& query, 
                                ModbusRTU::MEIMessageRetRDI& reply )
{
    dinfo << "(read4314): " << query << endl;

//    if( query.devID <= rdevMinNum || query.devID >= rdevMaxNum )
//        return erOperationFailed;

    auto dit = meidev.find(query.devID);
    if( dit == meidev.end() )
        return ModbusRTU::erBadDataAddress;

    auto oit = dit->second.find(query.objID);
    if( oit == dit->second.end() )
        return ModbusRTU::erBadDataAddress;

    reply.mf = 0xFF;
    reply.conformity = query.devID;
    for( const auto &i: oit->second )
        reply.addData( i.first, i.second );

    return erNoError;
}
// -------------------------------------------------------------------------
