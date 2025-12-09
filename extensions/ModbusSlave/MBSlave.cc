/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include <Poco/Net/NetException.h>
#include "unisetstd.h"
#include "Exceptions.h"
#include "Extensions.h"
#include "MBSlave.h"
#include "modbus/ModbusRTUSlaveSlot.h"
#include "modbus/ModbusTCPServerSlot.h"
#include "modbus/MBLogSugar.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    using namespace std;
    using namespace uniset::extensions;
    using namespace ModbusRTU;
    // -----------------------------------------------------------------------------
    MBSlave::MBSlave(uniset::ObjectId objId, uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic, const string& prefix ):
        USingleProcess(cnode, uniset_conf()->getArgc(), uniset_conf()->getArgv(), ""),
        UniSetObject(objId),
        initPause(3000),
        sidTestSMReady(DefaultObjectId),
        askcount_id(DefaultObjectId),
        respond_id(DefaultObjectId),
        respond_invert(false),
        connCount(0),
        activated(false),
        canceled(false),
        activateTimeout(500),
        smPingOK(true),
        force(false),
        mbregFromID(false),
        prefix(prefix),
        prop_prefix("")
    {
        if( objId == DefaultObjectId )
            throw uniset::SystemError("(MBSlave): objId=-1?!! Use --" + prefix + "-name" );

        auto conf = uniset_conf();
        mutex_start.setName(myname + "_mutex_start");

        loga = make_shared<LogAgregator>(myname + "-loga");
        loga->add(ulog());
        loga->add(dlog());

        mblog = loga->create(myname);
        conf->initLogStream(mblog, prefix + "-log");

        //    xmlNode* cnode = conf->getNode(myname);

        string conf_name(conf->getArgParam("--" + prefix + "-confnode", myname));
        cnode = conf->getNode(conf_name);

        if( cnode == NULL )
            throw uniset::SystemError("(MBSlave): Not found conf-node for " + myname );

        shm = make_shared<SMInterface>(shmId, ui, objId, ic);

        UniXML::iterator it(cnode);

        logserv = make_shared<LogServer>(loga);
        logserv->init( prefix + "-logserver", cnode );

        if( findArgParam("--" + prefix + "-run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
        {
            logserv_host = conf->getArg2Param("--" + prefix + "-logserver-host", it.getProp("logserverHost"), "localhost");
            logserv_port = conf->getArgPInt("--" + prefix + "-logserver-port", it.getProp("logserverPort"), getId());
        }

        if( ic )
            ic->logAgregator()->add(loga);

        // определяем фильтр
        s_field = conf->getArg2Param("--" + prefix + "-filter-field", it.getProp("filterField"));
        s_fvalue = conf->getArg2Param("--" + prefix + "-filter-value", it.getProp("filterValue"));
        mbinfo << myname << "(init): read s_field='" << s_field
               << "' s_fvalue='" << s_fvalue << "'" << endl;

        vmonit(s_field);
        vmonit(s_fvalue);

        // префикс для "свойств" - по умолчанию
        prop_prefix = "";

        // если "принудительно" задан префикс
        // используем его.
        {
            string p("--" + prefix + "-set-prop-prefix");
            string v = conf->getArgParam(p, "");

            if( !v.empty() && v[0] != '-' )
                prop_prefix = v;
            // если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
            else if( findArgParam(p, conf->getArgc(), conf->getArgv()) != -1 )
                prop_prefix = "";
        }

        vmonit(prop_prefix);

        mbinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

        force = conf->getArgInt("--" + prefix + "-force", it.getProp("force"));
        httpEnabledSetParams =  conf->getArgPInt("--" + prefix + "-http-enabled-setparams", it.getProp("httpEnabledSetParams"), 0);

        // int recv_timeout = conf->getArgParam("--" + prefix + "-recv-timeout",it.getProp("recv_timeout")));

        vmonit(force);

        default_mbaddr = conf->getArg2Param("--" + prefix + "-default-mbaddr", it.getProp("default_mbaddr"), "");
        default_mbfunc = conf->getArgPInt("--" + prefix + "-default-mbfunc", it.getProp("default_mbfunc"), 0);

        mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id", it.getProp("regFromId"));
        checkMBFunc = conf->getArgInt("--" + prefix + "-check-mbfunc", it.getProp("checkMBFunc"));
        noMBFuncOptimize = conf->getArgInt("--" + prefix + "-no-mbfunc-optimization", it.getProp("noMBFuncOptimization"));
        mbinfo << myname << "(init): mbregFromID=" << mbregFromID
               << " checkMBFunc=" << checkMBFunc
               << " default_mbfunc=" << default_mbfunc
               << " default_mbaddr=" << default_mbaddr
               << " noMBFuncOptimize=" << noMBFuncOptimize
               << endl;

        vmonit(default_mbaddr);
        vmonit(default_mbfunc);
        vmonit(checkMBFunc);
        vmonit(noMBFuncOptimize);

        respond_id = conf->getSensorID(conf->getArgParam("--" + prefix + "-respond-id", it.getProp("respond_id")));
        respond_invert = conf->getArgInt("--" + prefix + "-respond-invert", it.getProp("respond_invert"));

        timeout_t reply_tout = conf->getArgPInt("--" + prefix + "-reply-timeout", it.getProp("replyTimeout"), 3000);

        timeout_t aftersend_pause = conf->getArgInt("--" + prefix + "-aftersend-pause", it.getProp("afterSendPause"));

        checkExchangeTime = conf->getArgPInt("--" + prefix + "-check-exchange-time", it.getProp("checkExchangeTime"), 10000);
        vmonit(checkExchangeTime);
        vmonit(restartTCPServerCount);

        mbtype = conf->getArgParam("--" + prefix + "-type", it.getProp("type"));

        if( mbtype == "RTU" )
        {
            // ---------- init RS ----------
            string dev    = conf->getArgParam("--" + prefix + "-dev", it.getProp("device"));

            if( dev.empty() )
                throw uniset::SystemError(myname + "(MBSlave): Unknown device...");

            string speed  = conf->getArgParam("--" + prefix + "-speed", it.getProp("speed"));

            if( speed.empty() )
                speed = "38400";

            string parity = conf->getArgParam("--" + prefix + "-parity", it.getProp("parity"));
            int stopbits  = conf->getArgInt("--" + prefix + "-stopbits", it.getProp("stopBits"));
            bool use485F = conf->getArgInt("--" + prefix + "-use485F", it.getProp("use485F"));
            bool transmitCtl = conf->getArgInt("--" + prefix + "-transmit-ctl", it.getProp("transmitCtl"));
            string charsize = conf->getArgParam("--" + prefix + "-char-size", it.getProp("charSize"));

            auto rs = make_shared<ModbusRTUSlaveSlot>(dev, use485F, transmitCtl);
            rs->setSpeed(speed);

            if( !parity.empty() )
                rs->setParity(parity);

            rs->setRecvTimeout(2000);
            rs->setAfterSendPause(aftersend_pause);
            rs->setReplyTimeout(reply_tout);

            if( stopbits > 0 )
                rs->setStopBits((ComPort::StopBits)stopbits);

            if( !charsize.empty() )
                rs->setCharSize(ComPort::getCharacterSize(charsize));

            // rs->setLog(mblog);

            mbslot = std::static_pointer_cast<ModbusServerSlot>(rs);
            thr = unisetstd::make_unique< ThreadCreator<MBSlave> >(this, &MBSlave::execute_rtu);
            thr->setFinalAction(this, &MBSlave::finalThread);
            mbinfo << myname << "(init): type=RTU dev=" << dev << " speed=" << speed << endl;

            ostringstream n;
            n << prefix << "-exchangelog";
            auto l = loga->create(n.str());

            if( mblog->is_crit() )
                l->addLevel(Debug::CRIT);

            if( mblog->is_warn() )
                l->addLevel(Debug::WARN);

            rs->setLog(l);
            conf->initLogStream(l, prefix + "-exchangelog");
        }
        else if( mbtype == "TCP" )
        {
            string iaddr = conf->getArgParam("--" + prefix + "-inet-addr", it.getProp("iaddr"));

            if( iaddr.empty() )
                throw uniset::SystemError(myname + "(MBSlave): Unknown TCP server address. Use: --prefix-inet-addr [ XXX.XXX.XXX.XXX| hostname ]");

            int port = conf->getArgPInt("--" + prefix + "-inet-port", it.getProp("iport"), 502);

            tcpBreakIfFailRun = conf->getArgPInt("--" + prefix + "-break-if-fail-run", it.getProp("tcpBreakIfFailRun"), 0);
            tcpRepeatCreateSocketPause = conf->getArgPInt("--" + prefix + "-repeat-create-socket", it.getProp("tcpRepeatCreateSocket"), 30000);

            mbinfo << myname << "(init): type=TCP inet=" << iaddr << " port=" << port << endl;

            tcpserver = make_shared<ModbusTCPServerSlot>(iaddr, port);
            tcpserver->setAfterSendPause(aftersend_pause);
            tcpserver->setReplyTimeout(reply_tout);

            mbslot = std::static_pointer_cast<ModbusServerSlot>(tcpserver);
            //thr = make_shared< ThreadCreator<MBSlave> >(this, &MBSlave::execute_tcp);
            //thr->setFinalAction(this, &MBSlave::finalThread);
            mbinfo << myname << "(init): init TCP connection ok. " << " inet=" << iaddr << " port=" << port << endl;

            ostringstream n;
            n << prefix << "-exchangelog";
            auto l = loga->create(n.str());

            if( mblog->is_crit() )
                l->addLevel(Debug::CRIT);

            if( mblog->is_warn() )
                l->addLevel(Debug::WARN);

            tcpserver->setLog(l);
            conf->initLogStream(l, prefix + "-exchangelog");

            int tmpval = conf->getArgInt("--" + prefix + "-update-stat-time", it.getProp("updateStatTime"));

            if( tmpval > 0 )
                updateStatTime = tmpval;

            vmonit(updateStatTime);

            tmpval = conf->getArgInt("--" + prefix + "-session-timeout", it.getProp("sessTimeout"));

            if( tmpval > 0 )
                sessTimeout = tmpval;

            vmonit(sessTimeout);

            tmpval = conf->getArgInt("--" + prefix + "-socket-timeout", it.getProp("socketTimeout"));

            if( tmpval > 0 )
                sockTimeout = tmpval;

            vmonit(sockTimeout);

            tmpval = conf->getArgInt("--" + prefix + "-session-maxnum", it.getProp("sessMaxNum"));

            if( tmpval > 0 )
                sessMaxNum = tmpval;

            vmonit(sessMaxNum);

            sesscount_id = conf->getSensorID( conf->getArgParam("--" + prefix + "-session-count-id", it.getProp("sesscount")) );
        }
        else
            throw uniset::SystemError(myname + "(MBSlave): Unknown slave type. Use: --" + prefix + "-type [RTU|TCP]");

        mbslot->connectReadCoil( sigc::mem_fun(this, &MBSlave::readCoilStatus) );
        mbslot->connectReadInputStatus( sigc::mem_fun(this, &MBSlave::readInputStatus) );
        mbslot->connectReadOutput( sigc::mem_fun(this, &MBSlave::readOutputRegisters) );
        mbslot->connectReadInput( sigc::mem_fun(this, &MBSlave::readInputRegisters) );
        mbslot->connectForceSingleCoil( sigc::mem_fun(this, &MBSlave::forceSingleCoil) );
        mbslot->connectForceCoils( sigc::mem_fun(this, &MBSlave::forceMultipleCoils) );
        mbslot->connectWriteOutput( sigc::mem_fun(this, &MBSlave::writeOutputRegisters) );
        mbslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBSlave::writeOutputSingleRegister) );
        mbslot->connectMEIRDI( sigc::mem_fun(this, &MBSlave::read4314) );

        if( findArgParam("--" + prefix + "-allow-setdatetime", conf->getArgc(), conf->getArgv()) != -1 )
            mbslot->connectSetDateTime( sigc::mem_fun(this, &MBSlave::setDateTime) );

        mbslot->connectDiagnostics( sigc::mem_fun(this, &MBSlave::diagnostics) );
        mbslot->connectFileTransfer( sigc::mem_fun(this, &MBSlave::fileTransfer) );

        //    mbslot->connectJournalCommand( sigc::mem_fun(this, &MBSlave::journalCommand) );
        //    mbslot->connectRemoteService( sigc::mem_fun(this, &MBSlave::remoteService) );
        // -------------------------------

        initPause = conf->getArgPInt("--" + prefix + "-initPause", it.getProp("initPause"), 3000);

        if( shm->isLocalwork() )
        {
            readConfiguration();
            mbinfo << myname << "(init): iomap size = " << iomap.size()
                   << " myaddr=" << ModbusServer::vaddr2str(vaddr) << endl;
        }
        else
        {
            ic->addReadItem( sigc::mem_fun(this, &MBSlave::readItem) );
            // при работе с SM через указатель принудительно включаем force
            force = true;
        }

        // ********** HEARTBEAT *************
        string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", it.getProp("heartbeat_id"));

        if( !heart.empty() )
        {
            sidHeartBeat = conf->getSensorID(heart);

            if( sidHeartBeat == DefaultObjectId )
            {
                ostringstream err;
                err << myname << ": не найден идентификатор для датчика 'HeartBeat' " << heart;
                mbcrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }

            int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time", it.getProp("heartbeatTime"), conf->getHeartBeatTime());

            if( heartbeatTime > 0 )
                ptHeartBeat.setTiming(heartbeatTime);
            else
                ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

            maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", it.getProp("heartbeat_max"), 10);
        }

        string sm_ready_sid = conf->getArgParam("--" + prefix + "-sm-test-sid", it.getProp2("smTestSID", "TestMode_S"));
        sidTestSMReady = conf->getSensorID(sm_ready_sid);

        mbinfo << myname << ": init sidTestSMReady=" << sidTestSMReady << endl;

        askcount_id = conf->getSensorID(conf->getArgParam("--" + prefix + "-askcount-id", it.getProp("askcount_id")));
        mbinfo << myname << ": init askcount_id=" << askcount_id << endl;

        wait_msec = conf->getHeartBeatTime() - 100;

        if( wait_msec < 500 )
            wait_msec = 500;

        activateTimeout    = conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

        timeout_t msec = conf->getArgPInt("--" + prefix + "-timeout", it.getProp("timeout"), 3000);
        ptTimeout.setTiming(msec);

        mbinfo << myname << "(init): rs-timeout=" << msec << " msec" << endl;

        // build file list...
        xmlNode* fnode = 0;
        const std::shared_ptr<UniXML> xml = conf->getConfXML();

        if( xml )
            fnode = xml->extFindNode(cnode, 1, 1, "filelist");

        if( fnode )
        {
            UniXML::iterator fit(fnode);

            if( fit.goChildren() )
            {
                for( ; fit.getCurrent(); fit.goNext() )
                {
                    std::string nm = fit.getProp("name");

                    if( nm.empty() )
                    {
                        mbwarn << myname << "(build file list): ignore empty name... " << endl;
                        continue;
                    }

                    int id = fit.getIntProp("id");

                    if( id == 0 )
                    {
                        mbwarn << myname << "(build file list): FAILED ID for " << nm << "... ignore..." << endl;
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

                    mbinfo << myname << "(init):       add to filelist: "
                           << "id=" << id
                           << " file=" << nm
                           << endl;

                    flist[id] = nm;
                }
            }
            else
                mbinfo << myname << "(init): <filelist> empty..." << endl;
        }
        else
            mbinfo << myname << "(init): <filelist> not found..." << endl;


        // Формирование "карты" ответов на запрос 0x2B(43)/0x0E(14)
        xmlNode* mnode = 0;

        if( xml )
            mnode = xml->extFindNode(cnode, 1, 1, "MEI");

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
                for( ; dit.getCurrent(); dit.goNext() )
                {
                    if( dit.getProp("id").empty() )
                    {
                        mbwarn << myname << "(init): read <MEI>. Unknown <device id=''>. Ignore.." << endl;
                        continue;
                    }

                    int devID = dit.getIntProp("id");

                    UniXML::iterator oit(dit);

                    if( oit.goChildren() )
                    {
                        mbwarn << myname << "(init): MEI: read dev='" << devID << "'" << endl;
                        MEIObjIDMap meiomap;

                        // Object ID list..
                        for( ; oit.getCurrent(); oit.goNext() )
                        {
                            if( dit.getProp("id").empty() )
                            {
                                mbwarn << myname
                                       << "(init): read <MEI>. Unknown <object id='' (for device id='"
                                       << devID << "'). Ignore.."
                                       << endl;

                                continue;
                            }

                            int objID = oit.getIntProp("id");
                            UniXML::iterator sit(oit);

                            if( sit.goChildren() )
                            {
                                mbinfo << myname << "(init): MEI: read obj='" << objID << "'" << endl;
                                MEIValMap meivmap;

                                // request (string) list..
                                for( ; sit.getCurrent(); sit.goNext() )
                                {
                                    int vid = objID;

                                    if( sit.getProp("id").empty() )
                                    {
                                        mbwarn << myname << "(init): MEI: dev='" << devID
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
                mbinfo << myname << "(init): <MEI> init ok." << endl;
        }
        else
            mbinfo << myname << "(init): <MEI> empty..." << endl;

    }
    // -----------------------------------------------------------------------------
    MBSlave::~MBSlave()
    {
        canceled = true;

        if( tcpserver && tcpserver->isActive() )
            tcpserver->terminate();

        try
        {
            if( thr && thr->isRunning() )
            {
                thr->stop();
                //        thr->join();
            }
        }
        catch( Poco::NullPointerException& ex )
        {

        }
    }
    // -----------------------------------------------------------------------------
    void MBSlave::finalThread()
    {
        activated = false;
        canceled = true;
    }
    // -----------------------------------------------------------------------------
    bool MBSlave::waitSMReady()
    {
        // waiting for SM is ready...
        int tout = uniset_conf()->getArgInt("--" + prefix + "-sm-ready-timeout", "");
        timeout_t ready_timeout = uniset_conf()->getNCReadyTimeout();

        if( tout > 0 )
            ready_timeout = tout;
        else if( tout < 0 )
            ready_timeout = UniSetTimer::WaitUpTime;

        if( !shm->waitSMreadyWithCancellation(ready_timeout, canceled, 50) )
        {
            if( !canceled )
            {
                ostringstream err;
                err << myname << "(waitSMReady): failed waiting SharedMemory " << ready_timeout << " msec. ==> TERMINATE!";
                mbcrit << err.str() << endl;
            }

            return false;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    void MBSlave::execute_rtu()
    {
        auto rscomm = dynamic_pointer_cast<ModbusRTUSlaveSlot>(mbslot);

        // ждём чтобы прошла инициализация
        // потому-что нужно чтобы наполнилась таблица адресов (vaddr)
        if( !shm->isLocalwork() )
        {
            std::unique_lock<std::mutex> locker(mutexStartNotify);

            while( !activated && !canceled )
                startNotifyEvent.wait(locker);

            if( canceled )
                return;
        }

        if( vaddr.empty() )
        {
            mbcrit << "(execute_rtu): Unknown my modbus addresses!" << endl << flush;
            uterminate();
            return;
        }

        mbinfo << myname << "(execute_rtu): thread running.."
               << " iomap size = " << iomap.size()
               << " myaddr=" << ModbusServer::vaddr2str(vaddr)
               << endl;

        while( !canceled )
        {
            try
            {
                ModbusRTU::mbErrCode res = rscomm->receive( vaddr, wait_msec );

                if( res != ModbusRTU::erTimeOut )
                    ptTimeout.reset();

                if( res != ModbusRTU::erNoError && res != ModbusRTU::erTimeOut )
                    mbwarn << myname << "(execute_rtu): " << ModbusRTU::mbErr2Str(res) << endl;

                if( !activated )
                    continue;

                updateStatistics();
                updateThresholds();
            }
            catch( std::exception& ex )
            {
                dcrit << myname << "(execute_rtu): " << ex.what() << endl;
            }
        }
    }
    // -------------------------------------------------------------------------
    void MBSlave::execute_tcp()
    {
        if( !tcpserver )
        {
            mbcrit << myname << "(execute_tcp): DYNAMIC CAST ERROR (mbslot --> ModbusTCPServerSlot)" << std::endl << flush;
            uterminate();
            return;
        }

        // ждём чтобы прошла инициализация
        // потому-что нужно чтобы наполнилась таблица адресов (vaddr)
        if( !shm->isLocalwork() )
        {
            std::unique_lock<std::mutex> locker(mutexStartNotify);

            while( !activated )
                startNotifyEvent.wait(locker);
        }

        for( auto&& i : cmap )
            i.second.ptTimeout.reset();

        tcpserver->setMaxSessions( sessMaxNum );
        tcpserver->setSessionTimeout(sessTimeout);
        tcpserver->setSocketTimeout(sockTimeout);

        if( vaddr.empty() )
        {
            mbcrit << "(execute_tcp): Unknown my modbus addresses!" << endl << flush;
            uterminate();
            return;
        }

        // Чтобы не создавать отдельный поток для tcpserver (или для обновления статистики)
        // воспользуемся таймером tcpserver-а..
        tcpserver->signal_timer().connect( sigc::mem_fun(this, &MBSlave::updateTCPStatistics) );
        tcpserver->setTimer(updateStatTime);

        // для обновления пороговых датчиков
        tcpserver->signal_post_receive().connect( sigc::mem_fun(this, &MBSlave::postReceiveEvent) );

        runTCPServer();
    }
    // -------------------------------------------------------------------------
    void MBSlave::runTCPServer()
    {
        mbinfo << myname << "(runTCPServer): run tcpserver ("
               << tcpserver->getInetAddress() << ":" << tcpserver->getInetPort()
               << ")"
               << "["
               << " tcpBreakIfFailRun=" << tcpBreakIfFailRun
               << " tcpRepeatCreateSocketPause=" << tcpRepeatCreateSocketPause
               << "]"
               << endl;

        tcpCancelled = false;

        while( isActive() && !tcpserver->isActive() )
        {
            try
            {
                if( tcpserver->async_run(vaddr) )
                    break;

                if( tcpBreakIfFailRun )
                {
                    mbcrit << myname << "(execute_tcp): error run tcpserver: "
                           << tcpserver->getInetAddress()
                           << ":" << tcpserver->getInetPort() << " err: not active.."
                           << endl << flush;
                    uterminate();
                    return;
                }
            }
            catch( ModbusRTU::mbException& ex )
            {
                mbcrit << myname << "(execute_tcp): catch exception: "
                       << tcpserver->getInetAddress()
                       << ":" << tcpserver->getInetPort() << " err: " << ex << endl;

                if( tcpBreakIfFailRun )
                    throw ex;
            }
            catch( const Poco::Net::NetException& e )
            {
                mbcrit << myname << "(execute_tcp): Can`t create socket "
                       << tcpserver->getInetAddress()
                       << ":" << tcpserver->getInetPort()
                       << " err: " << e.displayText() << endl;

                if( tcpBreakIfFailRun )
                    throw e;
            }
            catch( const std::exception& e )
            {
                mbcrit << myname << "(execute_tcp): Can`t create socket "
                       << tcpserver->getInetAddress()
                       << ":" << tcpserver->getInetPort()
                       << " err: " << e.what() << endl;

                if( tcpBreakIfFailRun )
                    throw e;
            }
            catch(...)
            {
                mbcrit << myname << "(execute_tcp): catch exception ... ("
                       << tcpserver->getInetAddress()
                       << ":" << tcpserver->getInetPort()
                       << endl;

                if( tcpBreakIfFailRun )
                    throw;
            }

            msleep(tcpRepeatCreateSocketPause);
        }

    }
    // -------------------------------------------------------------------------
    void MBSlave::updateStatistics()
    {
        try
        {
            if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
            {
                try
                {
                    shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
                    ptHeartBeat.reset();
                }
                catch( const uniset::Exception& ex )
                {
                    mbcrit << myname << "(updateStatistics): (hb) " << ex << std::endl;
                }
            }

            if( respond_id != DefaultObjectId )
            {
                bool state = ptTimeout.checkTime() ? false : true;

                if( respond_invert )
                    state ^= true;

                try
                {
                    shm->localSetValue(itRespond, respond_id, (state ? 1 : 0), getId());
                }
                catch( const uniset::Exception& ex )
                {
                    mbcrit << myname << "(updateStatistics): (respond) " << ex << std::endl;
                }
            }

            if( askcount_id != DefaultObjectId )
            {
                try
                {
                    shm->localSetValue(itAskCount, askcount_id, connCount, getId());
                }
                catch( const uniset::Exception& ex )
                {
                    mbcrit << myname << "(updateStatistics): (askCount) " << ex << std::endl;
                }
            }

            if( sesscount_id != DefaultObjectId )
            {
                try
                {
                    shm->localSetValue(sesscount_it, sesscount_id, tcpserver->getCountSessions(), getId());
                }
                catch( const uniset::Exception& ex )
                {
                    mbcrit << myname << "(updateStatistics): (sessCount) " << ex << std::endl;
                }
            }
        }
        catch( std::exception& ex)
        {
            mbwarn << myname << "(updateStatistics): " << ex.what() << endl;
        }
    }
    // -------------------------------------------------------------------------
    void MBSlave::updateTCPStatistics()
    {
#if 0
        // тестирование вылета при SEGFAULT
        // или перезапуска при невыловленном EXCEPTION
        // для mainloop
        static size_t upCounter = 0;

        upCounter++;

        if( ++upCounter > 5 )
        {
            //          IOMap::iterator i;
            //          cout << "SEGFAULT: " << i->first << endl;

            upCounter = 0;
            cerr << ".....THROW...." << endl;
            throw std::string("TEST STRING EXCEPTION");
        }

#endif
        // ВНИМАНИЕ! Эта функция вызывается из основного eventLoop
        // поэтому она должна быть максимально быстрой и безопасной
        // иначе накроется весь обмен
        // т.к. на это время останавливается работа основного потока (eventLoop)
        // принимающего запросы

        try
        {
            if( !tcpserver )
                return;

            // Обновляем информацию по соединениям
            {
                std::lock_guard<std::mutex> l(sessMutex);
                sess.clear();
                tcpserver->getSessions(sess);
            }

            connCount = tcpserver->getConnectionCount();

            // если список сессий не пустой.. значит связь есть..
            if( !sess.empty() )
                ptTimeout.reset(); // см. updateStatistics()

            for( const auto& s : sess )
            {
                if(!activated || canceled )
                    return;

                auto i = cmap.find( s.iaddr );

                if( i != cmap.end() )
                {
                    // если ещё в списке, значит отвечает (т.е. сбрасываем таймер)
                    if( i->second.tout == 0 )
                        i->second.ptTimeout.setTiming( updateStatTime );
                    else
                        i->second.ptTimeout.reset();

                    i->second.askCount = s.askCount;
                }
            }

            // а теперь проходим по списку и выставляем датчики..
            for( const auto& it : cmap )
            {
                if(!activated || canceled )
                    return;

                auto c = it.second;

                mblog4 << myname << "(work): " << c.iaddr << " resp=" << (c.invert ? c.ptTimeout.checkTime() : !c.ptTimeout.checkTime())
                       << " askcount=" << c.askCount
                       << endl;

                if( c.respond_s != DefaultObjectId )
                {
                    try
                    {
                        bool st = c.invert ? c.ptTimeout.checkTime() : !c.ptTimeout.checkTime();
                        shm->localSetValue(c.respond_it, c.respond_s, st, getId());
                    }
                    catch( const uniset::Exception& ex )
                    {
                        mbcrit << myname << "(updateStatistics): " << ex << std::endl;
                    }
                }

                if( c.askcount_s != DefaultObjectId )
                {
                    try
                    {
                        shm->localSetValue(c.askcount_it, c.askcount_s, c.askCount, getId());
                    }
                    catch( const uniset::Exception& ex )
                    {
                        mbcrit << myname << "(updateStatistics): " << ex << std::endl;
                    }
                }
            }

            if(!activated || canceled )
                return;

            updateStatistics();
        }
        catch( std::exception& ex )
        {
            mbwarn << myname << "(updateStatistics): " << ex.what() << endl;
        }
        catch( ... )
        {
            mbwarn << myname << "(updateStatistics): unknown exception..." << endl;
        }
    }
    // -------------------------------------------------------------------------
    void MBSlave::updateThresholds()
    {
        for( auto&& i : thrlist )
        {
            try
            {
                IOBase::processingThreshold(&i, shm, force);
            }
            catch( std::exception& ex )
            {
                mbwarn << myname << "(updateThresholds): " << ex.what() << endl;
            }
        }
    }
    // -------------------------------------------------------------------------
    void MBSlave::postReceiveEvent( ModbusRTU::mbErrCode res )
    {
        updateThresholds();
    }
    // -------------------------------------------------------------------------
    void MBSlave::sysCommand( const uniset::SystemMessage* sm )
    {
        switch( sm->command )
        {
            case SystemMessage::StartUp:
            {
                if( iomap.empty() )
                {
                    mbcrit << myname << "(sysCommand): iomap EMPTY! terminated..." << endl << flush;
                    uterminate();
                    return;
                }

                if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
                {
                    mbinfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
                    logserv->async_run(logserv_host, logserv_port);
                }

                if( !waitSMReady() )
                {
                    if( !canceled )
                        uterminate();

                    return;
                }

                // подождать пока пройдёт инициализация датчиков
                // см. activateObject()
                msleep(initPause);
                PassiveTimer ptAct(activateTimeout);

                while(!canceled && !activated && !ptAct.checkTime() )
                {
                    cout << myname << "(sysCommand): wait activate..." << endl;
                    msleep(300);

                    if( activated )
                        break;
                }

                if( canceled )
                    return;

                if( !activated )
                {
                    mbcrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
                }
                else
                {
                    uniset::uniset_rwmutex_rlock l(mutex_start);
                    askSensors(UniversalIO::UIONotify);

                    if( mbtype == "RTU" && thr )
                        thr->start();
                    else if( mbtype == "TCP")
                        execute_tcp();

                    askTimer(tmCheckExchange, checkExchangeTime);
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
                mblogany << myname << "(sysCommand): logRotate" << std::endl;
                string fname = mblog->getLogFile();

                if( !fname.empty() )
                {
                    mblog->logFile(fname, true);
                    mblogany << myname << "(sysCommand): ***************** mblog LOG ROTATE *****************" << std::endl;
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
        if( !shm->waitSMworking(sidTestSMReady, activateTimeout, 50) )
        {
            ostringstream err;
            err << myname
                << "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение "
                << activateTimeout << " мсек";

            mbcrit << err.str() << endl << flush;
            uterminate();
            throw SystemError(err.str());
        }

        if( force )
            return;

        for( const auto& rmap : iomap )
        {
            for( const auto& it : rmap.second )
            {
                const IOProperty* p(&it.second);

                try
                {
                    shm->askSensor(p->si.id, cmd);
                }
                catch( const uniset::Exception& ex )
                {
                    mbwarn << myname << "(askSensors): " << ex << std::endl;
                }
                catch(...) {}
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    void MBSlave::sensorInfo( const uniset::SensorMessage* sm )
    {
        for( auto&& regs : iomap )
        {
            auto& rmap = regs.second;

            for( auto it = rmap.begin(); it != rmap.end(); ++it )
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
                    int i = 0;

                    for( ; i < sz && it != rmap.end(); i++, it++ )
                    {
                        p  = &it->second;

                        if( p->si.id == sm->id )
                            p->value = sm->value;
                    }

                    // вообще этого не может случиться
                    // потому-что корректность проверяется при загрузке
                    if( i != sz )
                        mbcrit << myname << "(sensorInfo): update failed for sid=" << sm->id
                               << " (i=" << i << " sz=" << sz << ")" << endl;

                    return;
                }
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    void MBSlave::timerInfo( const TimerMessage* tm )
    {
        if( tm->id == tmCheckExchange )
        {
            if( !tcpserver )
                return;

            if( !tcpserver->isActive() )
            {
                mbwarn << myname << "(timerInfo): tcpserver thread failed! restart.." << endl;
                restartTCPServerCount++;
                runTCPServer();
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    bool MBSlave::activateObject()
    {
        // блокирование обработки StartUp
        // пока не пройдёт инициализация датчиков
        // см. sysCommand()
        {
            activated = false;
            uniset::uniset_rwmutex_wrlock l(mutex_start);
            UniSetObject::activateObject();
            initIterators();
            activated = true;
            startNotifyEvent.notify_all();
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBSlave::deactivateObject()
    {
        mbinfo << myname << "(deactivateObject): ..." << endl;

        if( canceled )
            return UniSetObject::deactivateObject();

        activated = false;
        canceled = true;

        if( tcpserver )
        {
            if( tcpserver->isActive() )
                tcpserver->terminate();
        }
        else
        {
            try
            {
                if( mbslot )
                    mbslot->terminate();

                if( thr )
                    thr->join();
            }
            catch( std::exception& ex)
            {
                mbwarn << myname << "(deactivateObject): " << ex.what() << endl;
            }
        }

        if( logserv && logserv->isRunning() )
            logserv->terminate();

        return UniSetObject::deactivateObject();
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
            mbcrit << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
            return;
        }

        for( ; it.getCurrent(); it.goNext() )
        {
            if( uniset::check_filter(it, s_field, s_fvalue) )
                initItem(it);
        }

        //    readconf_ok = true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBSlave::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
    {
        if( uniset::check_filter(it, s_field, s_fvalue) )
            initItem(it);

        return true;
    }

    // ------------------------------------------------------------------------------------------
    bool MBSlave::initItem( UniXML::iterator& it )
    {
        IOProperty p;

        if( !IOBase::initItem( static_cast<IOBase*>(&p), it, shm, prop_prefix, false, mblog, myname) )
            return false;

        if( p.t_ai != DefaultObjectId )
        {
            // это пороговый датчик.. вносим его в список
            thrlist.emplace_back(std::move(p));
            return true;
        }

        std::string s_mbaddr = IOBase::initProp(it, "mbaddr", prop_prefix, false, default_mbaddr);

        if( s_mbaddr.empty() )
        {
            mbcrit << myname << "(initItem): Unknown '" << prop_prefix << "mbaddr' for " << it.getProp("name") << endl;
            return false;
        }

        // init sidTestSMReady
        if(sidTestSMReady == DefaultObjectId )
            sidTestSMReady = p.si.id;

        ModbusAddr mbaddr = ModbusRTU::str2mbAddr(s_mbaddr);

        // наполняем "таблицу" адресов устройства
        vaddr.emplace(mbaddr); // вставляем всегда (независимо есть или нет уже элемент)

        RegMap& rmap = iomap[mbaddr]; // создаём элемент (если его ещё нет) и сразу используем

        if( mbregFromID )
            p.mbreg = p.si.id;
        else
        {
            string r = IOBase::initProp(it, "mbreg", prop_prefix, false);

            if( r.empty() )
            {
                mbcrit << myname << "(initItem): Unknown '" << prop_prefix << "mbreg' for " << it.getProp("name") << endl;
                return false;
            }

            p.mbreg = ModbusRTU::str2mbData(r);
        }

        int mbfunc = IOBase::initIntProp(it, "mbfunc", prop_prefix, false, default_mbfunc);

        if( !checkMBFunc )
            mbfunc = default_mbfunc;

        // т.к. можно читать сразу несколько регистров независимо от того
        // как они обозначены в файле делаем принудительное преобразование в "многорегистровые" функции
        mbfunc = getOptimizeWriteFunction(mbfunc);

        p.regID = ModbusRTU::genRegID(p.mbreg, mbfunc);

        p.amode = MBSlave::amRW;
        string am(IOBase::initProp(it, "accessmode", prop_prefix, false));

        if( am == "ro" )
            p.amode = MBSlave::amRO;
        else if( am == "wo" )
            p.amode = MBSlave::amWO;

        ssize_t nbit = IOBase::initIntProp(it, "nbit", prop_prefix, false, -1);

        if( nbit != -1 )
        {
            if( nbit >= ModbusRTU::BitsPerData )
            {
                mbcrit << myname << "(initItem): BAD " << prop_prefix << "nbit=" << nbit << ". Must be  0 <= nbit < " << ModbusRTU::BitsPerData
                       << " for '"
                       << it.getProp("name")
                       << "'" << endl;
                return false;
            }

            auto i = rmap.find(p.regID);

            if( i != rmap.end() )
            {
                if( !i->second.bitreg )
                {
                    mbcrit << myname << "(initItem): BAD USE " << prop_prefix << "nbit=" << nbit
                           << " (for '"
                           << it.getProp("name")
                           << "') SENSOR ALREADY ADDED sid='" << i->second.si.id << "'"
                           << "(" << uniset_conf()->oind->getMapName(i->second.si.id) << ")"
                           << endl;
                    return false;
                }

                if( i->second.bitreg->check(p.si) )
                {
                    mbcrit << myname << "(initItem): BIT " << prop_prefix << "nbit=" << nbit
                           << " (for "
                           << it.getProp("name")
                           << ") ALREADY IN USE for sid='" << i->second.bitreg->bvec[nbit].si.id << "'"
                           << "(" << uniset_conf()->oind->getMapName(i->second.bitreg->bvec[nbit].si.id) << ")"
                           << endl;
                    return false;
                }

                i->second.bitreg->bvec[nbit] = std::move(p);
                mbinfo << myname << "(initItem): add new bit: " << i->second.bitreg.get() << endl;
                return true;
            }
            else
            {
                ModbusData mbreg = p.mbreg;
                IOBase b = p.make_iobase_copy();

                AccessMode p_amode = p.amode;
                int p_nbyte = p.nbyte;
                bool p_rawdata = p.rawdata;

                IOProperty p_dummy;
                p_dummy.create_from_iobase(b);
                p_dummy.bitreg = make_shared<BitRegProperty>();
                p_dummy.bitreg->mbreg = mbreg;
                p_dummy.regID = p.regID;
                p.vtype = VTypes::vtUnknown;
                p.wnum = 0;
                p_dummy.amode = p_amode;
                p_dummy.nbyte = p_nbyte;
                p_dummy.rawdata = p_rawdata;

                p_dummy.bitreg->bvec[nbit] = std::move(p); // после этого p использовать нельзя!

                mbinfo << myname << "(initItem): add bit register: " << p_dummy.bitreg.get() << endl;
                rmap[p_dummy.regID] = std::move(p_dummy);
            }

            return true;
        }

        auto i = rmap.find(p.regID);

        if( i != rmap.end() )
        {
            ostringstream err;
            err << myname << "(initItem): FAIL ADD sid='" << it.getProp("name") << "'(" << p.si.id << ")"
                << " reg='" << ModbusRTU::dat2str(p.mbreg) << "(" << (int)p.mbreg << ")"
                << " mbfunc=" << mbfunc << " --> regID=" << p.regID
                << " ALREADY ADDED! for sid='" << uniset_conf()->oind->getMapName(i->second.si.id) << "'("
                << i->second.si.id << ") regID=" << i->first
                << " reg='" << ModbusRTU::dat2str(i->second.mbreg) << "(" << (int)i->second.mbreg << ")"
                << " wnum=" << i->second.wnum;

            mbcrit << err.str() << endl;
            //throw SystemError( err.str() );
            abort();
        }

        string vt(IOBase::initProp(it, "vtype", prop_prefix, false));

        if( vt.empty() )
        {
            p.vtype = VTypes::vtUnknown;
            p.wnum = 0;
            mbinfo << myname << "(initItem): add " << p << endl;
            rmap[p.regID] = std::move(p);
        }
        else
        {
            VTypes::VType v(VTypes::str2type(vt));

            if( v == VTypes::vtUnknown )
            {
                mbcrit << myname << "(initItem): Unknown rtuVType=" << vt << " for "
                       << it.getProp("name")
                       << endl;

                return false;
            }
            else if( v == VTypes::vtByte )
            {
                p.nbyte = IOBase::initIntProp(it, "nbyte", prop_prefix, false);

                if( p.nbyte <= 0 )
                {
                    mbcrit << myname << "(initItem): Unknown " << prop_prefix << "nbyte='' for "
                           << it.getProp("name")
                           << endl;
                    return false;
                }
                else if( p.nbyte > 2 )
                {
                    mbcrit << myname << "(initItem): BAD " << prop_prefix << "nbyte='" << p.nbyte << "' for "
                           << it.getProp("name")
                           << ". Must be [1,2]."
                           << endl;
                    return false;
                }
            }

            p.vtype = v;
            p.wnum = 0;

            // копируем минимум полей, который нужен для обработки.. т.к. нам другие не понадобятся..
            int p_wnum = 0;
            ModbusData p_mbreg = p.mbreg;
            IOBase p_base(p.make_iobase_copy());

            AccessMode p_amode = p.amode;
            VTypes::VType p_vtype = p.vtype;
            int p_nbyte = p.nbyte;
            bool p_rawdata = p.rawdata;

            int wsz = VTypes::wsize(p.vtype);
            int p_regID = p.regID;

            // после std::move  p - использовать нельзя!
            mbinfo << myname << "(initItem): add " << p << endl;

            rmap[p_regID] = std::move(p);

            if( wsz > 1 )
            {
                for( int i = 1; i < wsz; i++ )
                {
                    IOProperty p_dummy;
                    p_dummy.create_from_iobase(p_base);
                    p_dummy.mbreg = p_mbreg + i;
                    p_dummy.wnum = p_wnum + i;
                    p_dummy.vtype = p_vtype;
                    p_dummy.amode = p_amode;
                    p_dummy.nbyte = p_nbyte;
                    p_dummy.rawdata = p_rawdata;
                    p_regID = genRegID(p_dummy.mbreg, mbfunc);
                    p_dummy.regID  = p_regID;

                    mbinfo << myname << "(initItem): add " << p_dummy << endl;
                    rmap[p_regID] = std::move(p_dummy);
                }
            }
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    int MBSlave::getOptimizeWriteFunction( const int fn )
    {
        if( noMBFuncOptimize )
            return fn;

        if( fn == ModbusRTU::fnWriteOutputSingleRegister ) // 0x06 --> 0x10
            return ModbusRTU::fnWriteOutputRegisters;

        if( fn == ModbusRTU::fnForceSingleCoil ) // 0x05 --> 0x0F
            return ModbusRTU::fnForceMultipleCoils;

        return fn;
    }
    // ------------------------------------------------------------------------------------------
    bool MBSlave::BitRegProperty::check( const IOController_i::SensorInfo& si )
    {
        for( const auto& i : bvec )
        {
            if( i.si.id == si.id && i.si.node == si.node )
                return true;
        }

        return false;
    }
    // ------------------------------------------------------------------------------------------
    void MBSlave::initIterators()
    {
        for( auto&& regs : iomap )
        {
            auto& rmap = regs.second;

            auto it = rmap.begin();

            for( ; it != rmap.end(); ++it )
            {
                shm->initIterator(it->second.ioit);

                if( it->second.bitreg )
                {
                    auto b = it->second.bitreg;

                    for( auto i = b->bvec.begin(); i != b->bvec.end(); ++ i )
                        shm->initIterator(i->ioit);
                }
            }
        }

        shm->initIterator(itHeartBeat);
        shm->initIterator(itAskCount);
        shm->initIterator(itRespond);

        // for TCPServer
        shm->initIterator(sesscount_it);

        for( auto&& i : cmap )
            i.second.initIterators(shm);
    }
    // -----------------------------------------------------------------------------
    void MBSlave::help_print( int argc, const char* const* argv )
    {
        cout << "--run-lock file               - Запустить с защитой от повторного запуска" << endl;
        cout << "--mbs-name  name              - ObjectID. По умолчанию: MBSlave1" << endl;
        cout << "--mbs-confnode cnode          - Возможность задать настроечный узел в configure.xml. По умолчанию: name" << endl;
        cout << endl;
        cout << "--mbs-reg-from-id 0,1         - Использовать в качестве регистра sensor ID" << endl;
        cout << "--mbs-filter-field name       - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
        cout << "--mbs-filter-value val        - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
        cout << "--mbs-heartbeat-id            - Данный процесс связан с указанным аналоговым heartbeat-датчиком." << endl;
        cout << "--mbs-heartbeat-max           - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
        cout << "--mbs-sm-ready-timeout        - время на ожидание старта SM" << endl;
        cout << "--mbs-sm-test-sid name        - Датчик для проверки готовности SM к работе. По умолчанию идёт попытка автоопределения." << endl;
        cout << "--mbs-initPause               - Задержка перед инициализацией (время на активизация процесса)" << endl;
        cout << "--mbs-force 1                 - Читать данные из SM каждый раз, а не по изменению." << endl;
        cout << "--mbs-respond-id              - respond sensor id" << endl;
        cout << "--mbs-respond-invert [0|1]    - invert respond logic" << endl;
        cout << "--mbs-sm-ready-timeout        - время на ожидание старта SM" << endl;
        cout << "--mbs-timeout msec            - timeout for check link" << endl;
        cout << "--mbs-after-send-pause msec   - принудительная пауза после посылки ответа. По умолчанию: 0" << endl;
        cout << "--mbs-reply-timeout msec      - Контрольное время для формирования ответа. " << endl
             << "                                   Если обработка запроса превысит это время, ответ не будет послан (timeout)." << endl
             << "                                   По умолчанию: 3 сек" << endl;

        cout << "--mbs-default-mbfunc [0..255] - Функция по умолчанию, если не указан параметр mbfunc в настройках регистра. Только если включён контроль функций. " << endl;
        cout << "--mbs-check-mbfunc  [0|1]     - Включить контроль (обработку) свойства mbfunc. По умолчанию: отключён." << endl;
        cout << "--mbs-no-mbfunc-optimization [0|1] - Отключить принудительное преобразование функций 0x06->0x10,0x05->0x0F" << endl;
        cout << "--mbs-set-prop-prefix [val]   - Использовать для свойств указанный или пустой префикс." << endl;

        cout << "--mbs-allow-setdatetime - On set date and time (0x50) modbus function" << endl;
        cout << "--mbs-my-addr           - адрес текущего узла" << endl;
        cout << "--mbs-type [RTU|TCP]    - modbus server type." << endl;

        cout << " Настройки протокола RTU: " << endl;
        cout << "--mbs-dev devname  - файл устройства" << endl;
        cout << "--mbs-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;
        cout << "--mbs-parity val   - Контроль чётности (odd,even,noparity,space,mark)." << endl;
        cout << "--mbs-charsize val - Битность (cs5, cs6, cs7, cs8). По умолчанию: cs8" << endl;
        cout << "--mbs-stopbits val - Стоп-биты" << endl;
        cout << "                    1 - OneBit" << endl;
        cout << "                    2 - OneAndHalfBits" << endl;
        cout << "                    3 - TwoBits" << endl;
        cout << " Настройки протокола TCP: " << endl;
        cout << "--mbs-inet-addr [xxx.xxx.xxx.xxx | hostname ]  - this modbus server address" << endl;
        cout << "--mbs-inet-port num - this modbus server port. Default: 502" << endl;
        cout << "--mbs-update-stat-time msec  - Период обновления статистики работы. По умолчанию: 4 сек." << endl;
        cout << "--mbs-session-timeout msec   - Таймаут на закрытие соединения с 'клиентом', если от него нет запросов. По умолчанию: 10 сек." << endl;
        cout << "--mbs-session-maxnum num     - Максимальное количество соединений. По умолчанию: 5." << endl;
        cout << "--mbs-session-count-id  id   - Датчик для отслеживания текущего количества соединений." << endl;
        cout << "--mbs-socket-timeout msec    - Таймаут на переоткрытие сокета если долго нет соединений. По умолчанию: 0 (не переоткрывать)" << endl;
        cout << endl;
        cout << " HTTP API: " << endl;
        cout << "--mbs-http-enabled-setparams 1 - Enable API /setparams" << endl;
        cout << endl;
        cout << " Logs: " << endl;
        cout << "--mbs-log-...            - log control" << endl;
        cout << "             add-levels ...  " << endl;
        cout << "             del-levels ...  " << endl;
        cout << "             set-levels ...  " << endl;
        cout << "             logfile filename" << endl;
        cout << "             no-debug " << endl;
        cout << " LogServer: " << endl;
        cout << "--mbs-run-logserver      - run logserver. Default: localhost:id" << endl;
        cout << "--mbs-logserver-host ip  - listen ip. Default: localhost" << endl;
        cout << "--mbs-logserver-port num - listen port. Default: ID" << endl;
        cout << LogServer::help_print("prefix-logserver") << endl;
    }
    // -----------------------------------------------------------------------------
    string MBSlave::amode2str(MBSlave::AccessMode m)
    {
        switch(m)
        {
            case amRW:
                return "rw";

            case amRO:
                return "ro";

            case amWO:
                return "wr";

            default:
                return "";
        }
    }
    // -----------------------------------------------------------------------------
    std::shared_ptr<MBSlave> MBSlave::init_mbslave(int argc, const char* const* argv, uniset::ObjectId icID,
            const std::shared_ptr<SharedMemory>& ic, const string& prefix )
    {
        auto conf = uniset_conf();
        string name = conf->getArgParam("--" + prefix + "-name", "MBSlave1");

        if( name.empty() )
        {
            cerr << "(mbslave): Не задан name'" << endl;
            return 0;
        }

        ObjectId ID = conf->getObjectID(name);

        if( ID == uniset::DefaultObjectId )
        {
            cerr << "(mbslave): идентификатор '" << name
                 << "' не найден в конф. файле!"
                 << " в секции " << conf->getObjectsSection() << endl;
            return 0;
        }

        dinfo << "(mbslave): name = " << name << "(" << ID << ")" << endl;
        return make_shared<MBSlave>(ID, icID, ic, prefix);
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

        for( const auto& i : p.bvec )
            os << "'" << i.si.id << "' ";

        os << "]";

        return os;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, MBSlave::IOProperty& p )
    {
        os  << "reg=" << ModbusRTU::dat2str(p.mbreg) << "(" << (int)p.mbreg << ")"
            << " sid=" << p.si.id
            << " stype=" << p.stype
            << " wnum=" << p.wnum
            << " nbyte=" << p.nbyte
            << " safeval=" << p.safeval
            << " invert=" << p.invert
            << " regID=" << p.regID;

        if( p.stype == UniversalIO::AI || p.stype == UniversalIO::AO )
        {
            os     << p.cal
                   << " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
        }

        return os;
    }
    // -----------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::readOutputRegisters( const ModbusRTU::ReadOutputMessage& query,
            ModbusRTU::ReadOutputRetMessage& reply )
    {
        mbinfo << myname << "(readOutputRegisters): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(readOutputRegisters): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        if( query.count <= 0 )
        {
            mbinfo << myname << "(readOutputRegisters): BAD QUERY: count=0!" << endl;
            return ModbusRTU::erBadDataValue;
        }

        if( query.count == 1 )
        {
            ModbusRTU::ModbusData d = 0;
            ModbusRTU::mbErrCode ret = real_read(regmap->second, query.start, d, query.func);

            if( ret == ModbusRTU::erNoError )
                reply.addData(d);
            else
                reply.addData(0);

            return ret;
        }

        // Формирование ответа:
        ModbusRTU::mbErrCode ret = much_read(regmap->second, query.start, buf, query.count, query.func);

        if( ret == ModbusRTU::erNoError )
        {
            for( uint16_t i = 0; i < query.count; i++ )
                reply.addData( buf[i] );
        }

        return ret;
    }

    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::writeOutputRegisters( const ModbusRTU::WriteOutputMessage& query,
            ModbusRTU::WriteOutputRetMessage& reply )
    {
        mbinfo << myname << "(writeOutputRegisters): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(writeOutputRegisters): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        // Формирование ответа:
        int fn = getOptimizeWriteFunction(query.func);
        ModbusRTU::mbErrCode ret = much_write(regmap->second, query.start, query.data, query.quant, fn);

        if( ret == ModbusRTU::erNoError )
            reply.set(query.start, query.quant);

        return ret;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::writeOutputSingleRegister( const ModbusRTU::WriteSingleOutputMessage& query,
            ModbusRTU::WriteSingleOutputRetMessage& reply )
    {
        mbinfo << myname << "(writeOutputSingleRegisters): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(writeOutputRegisters): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        int fn = getOptimizeWriteFunction(query.func);
        ModbusRTU::mbErrCode ret = real_write(regmap->second, query.start, query.data, fn);

        if( ret == ModbusRTU::erNoError )
            reply.set(query.start, query.data);

        return ret;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::much_write( RegMap& rmap, const ModbusRTU::ModbusData reg, const ModbusRTU::ModbusData* dat,
            size_t count, const int fn )
    {
        mbinfo << myname << "(much_write): write mbID="
               << ModbusRTU::dat2str(reg) << "(" << (int)reg << ")" << " count=" << count << " fn=" << fn << endl;

        size_t i = 0;
        auto it = rmap.end();

        int mbfunc = checkMBFunc ? fn : default_mbfunc;
        ModbusRTU::RegID regID = genRegID(reg, mbfunc);

        // ищем регистр пропуская "дыры"
        // ведь запросить могут начиная с "несуществующего регистра"
        for( ; i < count; i++ )
        {
            it = rmap.find(regID + i);

            if( it != rmap.end() )
            {
                regID += i;
                break;
            }
        }

        if( it == rmap.end() )
            return ModbusRTU::erBadDataAddress;

        size_t prev_i = i;
        size_t sub = 0;

        for( ; (it != rmap.end()) && (i < count); )
        {
            if( it->first == regID )
            {
                prev_i = i;

                real_write_it(rmap, it, dat, i, count);

                sub = (i - prev_i);

                // если при обработке i не сдвигали..
                // значит сами делаем ++
                if( sub == 0 )
                {
                    sub = 1;
                    i++;
                }

                it = std::next(it, sub);
                regID += sub;
            }
            else
            {
                regID++;
                i++;
            }
        }

        return ModbusRTU::erNoError;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_write( RegMap& rmap, const ModbusRTU::ModbusData reg, ModbusRTU::ModbusData val, const int fn )
    {
        ModbusRTU::ModbusData dat[1] = {val};
        size_t i = 0;
        return real_write(rmap, reg, dat, i, 1, fn);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_write(RegMap& rmap, const ModbusRTU::ModbusData reg, const ModbusRTU::ModbusData* dat, size_t& i, size_t count, const int fn )
    {
        ModbusRTU::ModbusData mbval = dat[i];

        mbinfo << myname << "(write): save mbID="
               << ModbusRTU::dat2str(reg)
               << " data=" << ModbusRTU::dat2str(mbval)
               << "(" << (int)mbval << ")" << endl;

        ModbusRTU::RegID regID = checkMBFunc ? genRegID(reg, fn) : genRegID(reg, default_mbfunc);

        auto it = rmap.find(regID);
        return real_write_it(rmap, it, dat, i, count);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_write_it(RegMap& rmap, RegMap::iterator& it, const ModbusRTU::ModbusData* dat, size_t& i, size_t count )
    {
        if( it == rmap.end() )
            return ModbusRTU::erBadDataAddress;

        IOProperty* p(&it->second);

        if( p->bitreg )
            return real_bitreg_write_it(p->bitreg, dat[i]);

        return real_write_prop(p, dat, i, count);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_bitreg_write_it( std::shared_ptr<BitRegProperty>& bp, const ModbusRTU::ModbusData val )
    {
        mbinfo << myname << "(real_bitreg_write_it): write mbID="
               << ModbusRTU::dat2str(bp->mbreg) << "(" << (int)bp->mbreg << ") val=" << val << endl;

        ModbusRTU::DataBits16 d(val);

        for( size_t i = 0; i < ModbusRTU::BitsPerData; i++ )
        {
            IOProperty* p(&(bp->bvec[i]));

            if( p->si.id == DefaultObjectId )
                continue;

            ModbusRTU::ModbusData dat[] = {d[i]};

            mbinfo << myname << "(real_bitreg_write_it): set " << ModbusRTU::dat2str(bp->mbreg) << "(" << (int)bp->mbreg << ")"
                   << " bit[" << i << "]=" << (int)dat[0]  << " sid=" << p->si.id << endl;

            size_t k = 0;
            real_write_prop(p, dat, k, 1);
        }

        return ModbusRTU::erNoError;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_write_prop( IOProperty* p, const ModbusRTU::ModbusData* dat, size_t& i, size_t count )
    {
        ModbusRTU::ModbusData mbval = dat[i];

        mbinfo << myname << "(real_write_prop): val=" << mbval << " " << (*p) << endl;

        try
        {
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
                long val = (uint16_t)(mbval);
                IOBase::processingAsAI( p, val, shm, force );
            }
            else if( p->vtype == VTypes::vtSigned )
            {
                long val = (int16_t)(mbval);
                IOBase::processingAsAI( p, val, shm, force );
            }
            else if( p->vtype == VTypes::vtI2 )
            {
                if( (i + VTypes::I2::wsize()) > count )
                {
                    i += VTypes::I2::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::I2::wsize());

                for( size_t k = 0; k < VTypes::I2::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::I2 i2(d.data, d.len);
                IOBase::processingAsAI( p, (long)i2, shm, force );
            }
            else if( p->vtype == VTypes::vtI2r )
            {
                if( (i + VTypes::I2r::wsize()) > count )
                {
                    i += VTypes::I2r::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::I2r::wsize());

                for( size_t k = 0; k < VTypes::I2r::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::I2r i2r(d.data, d.len);
                IOBase::processingAsAI( p, (long)i2r, shm, force );
            }
            else if( p->vtype == VTypes::vtU2 )
            {
                if( (i + VTypes::U2::wsize()) > count )
                {
                    i += VTypes::U2::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::U2::wsize());

                for( size_t k = 0; k < VTypes::U2::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::U2 u2(d.data, d.len);
                IOBase::processingAsAI( p, (unsigned long)u2, shm, force );
            }
            else if( p->vtype == VTypes::vtU2r )
            {
                if( (i + VTypes::U2r::wsize()) > count )
                {
                    i += VTypes::U2r::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::U2r::wsize());

                for( size_t k = 0; k < VTypes::U2r::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::U2r u2r(d.data, d.len);
                IOBase::processingAsAI( p, (unsigned long)u2r, shm, force );
            }
            else if( p->vtype == VTypes::vtF2 )
            {
                if( (i + VTypes::F2::wsize()) > count )
                {
                    i += VTypes::F2::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::F2::wsize());

                for( size_t k = 0; k < VTypes::F2::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::F2 f2(d.data, d.len);
                IOBase::processingFasAI( p, (float)f2, shm, force );
            }
            else if( p->vtype == VTypes::vtF2r )
            {
                if( (i + VTypes::F2r::wsize()) > count )
                {
                    i += VTypes::F2r::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::F2r::wsize());

                for( size_t k = 0; k < VTypes::F2r::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::F2r f2r(d.data, d.len);
                IOBase::processingFasAI( p, (float)f2r, shm, force );
            }
            else if( p->vtype == VTypes::vtF4 )
            {
                if( (i + VTypes::F4::wsize()) > count )
                {
                    i += VTypes::F4::wsize();
                    return ModbusRTU::erInvalidFormat;
                }

                ModbusRTU::DataGuard d(VTypes::F4::wsize());

                for( size_t k = 0; k < VTypes::F4::wsize(); k++, i++ )
                    d.data[k] = dat[i];

                VTypes::F4 f4(d.data, d.len);
                IOBase::processingFasAI( p, (double)f4, shm, force );
            }
            else if( p->vtype == VTypes::vtByte )
            {
                VTypes::Byte b(mbval);
                IOBase::processingAsAI( p, b.raw.b[p->nbyte - 1], shm, force );
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
            smPingOK = true;
            return ModbusRTU::erNoError;
        }
        catch( uniset::NameNotFound& ex )
        {
            mbwarn << myname << "(real_write_prop): " << ex << endl;
            return ModbusRTU::erBadDataAddress;
        }
        catch( uniset::OutOfRange& ex )
        {
            mbwarn << myname << "(real_write_prop): " << ex << endl;
            return ModbusRTU::erBadDataValue;
        }
        catch( const CORBA::SystemException& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(real_write_prop): СORBA::SystemException: "
                       << ex.NP_minorString() << endl;
        }
        catch( const uniset::Exception& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(real_write_prop): " << ex << endl;
        }
        catch(...)
        {
            if( smPingOK )
                mbcrit << myname << "(real_write_prop) catch ..." << endl;
        }

        smPingOK = false;
        return ModbusRTU::erTimeOut;
    }
#ifndef DISABLE_REST_API
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::request_regs( const string& req, const Poco::URI::QueryParameters& params )
    {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr jdata = uniset::json::make_child_array(json, "regs");
        auto my = httpGetMyInfo(json);
        auto oind = uniset_conf()->oind;

        std::vector<std::string> q_regs;
        std::vector<std::string> q_addr;

        if( !params.empty() )
        {
            for( const auto& p : params )
            {
                if( p.first == "regs" )
                    q_regs = uniset::explode_str(p.second, ',');
                else if( p.first == "addr" )
                    q_addr = uniset::explode_str(p.second, ',');
            }
        }

        /* Создаём json
         * { {"addr":
         *         {"regs":
         *               {reginfo..},
         *               {reginfo..},
         *   }},
         * }
         */

        // Проход по списку заданных addr..
        if( !q_addr.empty() )
        {
            for( const auto& a : q_addr )
            {
                ModbusRTU::ModbusAddr mbaddr = ModbusRTU::str2mbAddr(a);
                auto i = iomap.find(mbaddr);

                if( i != iomap.end() )
                {
                    Poco::JSON::Object::Ptr jaddr = get_regs(i->first, i->second, q_regs);
                    jdata->add(jaddr);
                }
            }
        }
        else // Проход по всему списку
        {
            for( const auto& i : iomap )
            {
                Poco::JSON::Object::Ptr jaddr = get_regs(i.first, i.second, q_regs);
                jdata->add(jaddr);
            }
        }

        return json;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::get_regs(ModbusRTU::ModbusAddr addr, const RegMap& rmap, const std::vector<string>& q_regs )
    {
        Poco::JSON::Object::Ptr jaddr = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr regs = new Poco::JSON::Array();
        jaddr->set( ModbusRTU::addr2str(addr), regs);

        if( q_regs.empty() )
        {
            for( const auto& r : rmap )
            {
                Poco::JSON::Object::Ptr reginfo = get_reginfo(r.second);
                regs->add(reginfo);
            }
        }
        else
        {
            for( const auto& s : q_regs )
            {
                auto reg = genRegID( ModbusRTU::str2mbData(s), default_mbfunc);
                auto r = rmap.find(reg);

                if( r != rmap.end() )
                {
                    Poco::JSON::Object::Ptr reginfo = get_reginfo(r->second);
                    regs->add(reginfo);
                }
            }
        }

        return jaddr;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::get_reginfo( const IOProperty& prop )
    {
        Poco::JSON::Object::Ptr reginfo = new Poco::JSON::Object();
        reginfo->set("mbreg", prop.mbreg);
        reginfo->set("vtype", VTypes::type2str( prop.vtype));
        reginfo->set("regID",  prop.regID);
        reginfo->set("amode", amode2str( prop.amode));
        reginfo->set("value",  prop.value);
        return reginfo;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::httpHelp( const Poco::URI::QueryParameters& p )
    {
        uniset::json::help::object myhelp(myname, UniSetObject::httpHelp(p));

        {
            uniset::json::help::item cmd("regs", "get registers list");
            cmd.param("regs=reg1,reg2,reg3..", "get these registers");
            cmd.param("addr=mbaddr1,mbaddr2,..", "get registers for mbaddr");
            myhelp.add(cmd);
        }

        {
            uniset::json::help::item cmd("getparam", "read runtime parameters");
            cmd.param("name", "parameter to read; can be repeated");
            cmd.param("note", "supported: force | sockTimeout | sessTimeout | updateStatTime");
            myhelp.add(cmd);
        }

        {
            uniset::json::help::item cmd("setparam", "set runtime parameters");
            cmd.param("force", "0|1");
            cmd.param("sockTimeout", "milliseconds");
            cmd.param("sessTimeout", "milliseconds");
            cmd.param("updateStatTime", "milliseconds");
            cmd.param("note", "may be disabled by httpEnabledSetParams");
            myhelp.add(cmd);
        }

        {
            uniset::json::help::item cmd("status", "get current object status (same as getInfo())");
            myhelp.add(cmd);
        }


        return myhelp;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::httpRequest( const UHttp::HttpRequestContext& ctx )
    {
        if( ctx.depth() > 0 )
        {
            const std::string& req = ctx[0];

            if( req == "regs" )
                return request_regs(req, ctx.params);

            if( req == "getparam" )
                return httpGetParam(ctx.params);

            if( req == "setparam" )
                return httpSetParam(ctx.params);

            if( req == "status" )
                return httpStatus();
        }

        return UniSetObject::httpRequest(ctx);
    }
    // -------------------------------------------------------------------------
    static long to_long(const std::string& s, const std::string& what, const std::string& myname)
    {
        try
        {
            size_t pos = 0;
            long v = std::stol(s, &pos, 10);

            if( pos != s.size() )
                throw std::invalid_argument("garbage");

            return v;
        }
        catch(...)
        {
            throw uniset::SystemError(myname + "(/setparam): invalid value for '" + what + "': '" + s + "'");
        }
    }

    static bool to_bool01(const std::string& s, const std::string& what, const std::string& myname)
    {
        if( s == "0" ) return false;

        if( s == "1" ) return true;

        throw uniset::SystemError(myname + "(/setparam): invalid value for '" + what + "': '" + s + "' (expected 0|1)");
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::httpGetParam( const Poco::URI::QueryParameters& p )
    {
        if( p.empty() )
            throw uniset::SystemError(myname + "(/getparam): pass at least one 'name' parameter");

        std::vector<std::string> names;
        names.reserve(p.size());

        for( const auto& kv : p )
            if( kv.first == "name" && !kv.second.empty() )
                names.push_back(kv.second);

        if( names.empty() )
            throw uniset::SystemError(myname + "(/getparam): parameter 'name' is required (can be repeated)");

        Poco::JSON::Object::Ptr js = new Poco::JSON::Object();
        Poco::JSON::Object::Ptr params = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr unknown = new Poco::JSON::Array();

        for( const auto& n : names )
        {
            if( n == "force" )
                params->set(n, force ? 1 : 0);
            else if( n == "sockTimeout" )
                params->set(n, (int)sockTimeout);
            else if( n == "sessTimeout" )
                params->set(n, (int)sessTimeout);
            else if( n == "updateStatTime" )
                params->set(n, (int)updateStatTime);
            else
                unknown->add(n);
        }

        js->set("result", "OK");
        js->set("params", params);

        if( unknown->size() > 0 )
            js->set("unknown", unknown);

        return js;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::httpSetParam( const Poco::URI::QueryParameters& p )
    {
        if( p.empty() )
            throw uniset::SystemError(myname + "(/setparam): pass key=value pairs, e.g. /setparam?force=1");

        if( !httpEnabledSetParams )
            throw uniset::SystemError(myname + "(/setparam): disabled by httpEnabledSetParams");

        Poco::JSON::Object::Ptr js = new Poco::JSON::Object();
        Poco::JSON::Object::Ptr updated = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr unknown = new Poco::JSON::Array();

        for( const auto& kv : p )
        {
            const std::string& name = kv.first;
            const std::string& val  = kv.second;

            long v = std::stol(val);

            if( name == "force" )
            {
                bool b = to_bool01(val, name, myname);
                force = b;
                updated->set(name, b ? 1 : 0);
            }
            else if( name == "sockTimeout" )
            {
                long v = to_long(val, name, myname);
                if( v < 0 )
                    throw uniset::SystemError(myname + "(/setparam): value must be >= 0 (" + name + ")");

                sockTimeout = (timeout_t)v;
                updated->set(name, (int)sockTimeout);
            }
            else if( name == "sessTimeout" )
            {
                long v = to_long(val, name, myname);
                if( v < 0 )
                    throw uniset::SystemError(myname + "(/setparam): value must be >= 0 (" + name + ")");
                sessTimeout = (timeout_t)v;
                updated->set(name, (int)sessTimeout);
            }
            else if( name == "updateStatTime" )
            {
                long v = to_long(val, name, myname);
                if( v < 0 )
                    throw uniset::SystemError(myname + "(/setparam): value must be >= 0 (" + name + ")");
                updateStatTime = (timeout_t)v;
                updated->set(name, (int)updateStatTime);
            }
            else
            {
                unknown->add(name);
            }
        }

        js->set("result", "OK");
        js->set("updated", updated);

        if( unknown->size() > 0 )
            js->set("unknown", unknown);

        return js;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::httpStatus()
    {
        using Poco::JSON::Array;
        using Poco::JSON::Object;

        Object::Ptr js = new Object();

        // Базовая информация (как в начале getInfo())
        js->set("name", myname);

        // TCP endpoint (если это TCP-слот)
        if( auto sslot = std::dynamic_pointer_cast<ModbusTCPServerSlot>(mbslot) )
        {
            Object::Ptr tcp = new Object();
            tcp->set("ip",   sslot->getInetAddress());
            tcp->set("port", (int)sslot->getInetPort());
            js->set("tcp", tcp);
        }

        // Мониторинг/счётчики (если нужен быстрый обзор строкой)
        // Аналог vmon.pretty_str(); оставим как строку
        js->set("monitor", vmon.pretty_str());

        // LogServer адрес/порт (как печатается в getInfo)
        {
            Object::Ptr log = new Object();
            log->set("host", logserv_host);
            log->set("port", (int)logserv_port);
            js->set("logserver", log);
        }

        // I/O карта (iomap)
        {
            Object::Ptr io = new Object();
            io->set("size", (int)iomap.size());

            Array::Ptr mb = new Array();

            for( const auto& m : iomap )
            {
                Object::Ptr it = new Object();
                it->set("mbaddr", ModbusRTU::addr2str(m.first));   // как в getInfo()
                it->set("iomap",  (int)m.second.size());
                mb->add(it);
            }

            io->set("map", mb);
            js->set("iomap", io);
        }

        // Собственный адрес устройства
        js->set("myaddr", ModbusServer::vaddr2str(vaddr) );

        // Статистика соединений (как в getInfo: connectionCount, smPingOK, restartTCPServerCount)
        {
            Object::Ptr st = new Object();
            st->set("connectionCount",      (int)connCount);
            st->set("smPingOK",             (int)smPingOK);
            st->set("restartTCPServerCount", (int)restartTCPServerCount);
            js->set("stat", st);
        }

        // TCP-клиенты (как печатается список cmap)
        {
            Array::Ptr clients = new Array();

            for( const auto& c : cmap )
            {
                Object::Ptr co = new Object();
                co->set("id",    c.first);
                co->set("short", c.second.getShortInfo()); // как "   <short info>"
                clients->add(co);
            }

            js->set("tcp_clients", clients);
        }

        // TCP-сессии (как печатается в конце getInfo: sess, sessMaxNum, updateStatTime)
        {
            Object::Ptr s = new Object();
            Array::Ptr  arr = new Array();
            {
                std::lock_guard<std::mutex> l(sessMutex);
                s->set("count",        (int)sess.size());
                s->set("max_sessions", (int)sessMaxNum);
                s->set("updateStatTime", (int)updateStatTime);

                for( const auto& m : sess )
                {
                    Object::Ptr so = new Object();
                    so->set("ip",       m.iaddr);
                    so->set("askCount", (int)m.askCount);
                    arr->add(so);
                }
            }
            s->set("items", arr);
            js->set("tcp_sessions", s);
        }

        // Пара удобных «плоских» полей для быстрого доступа
        js->set("sockTimeout",    (int)sockTimeout);
        js->set("sessTimeout",    (int)sessTimeout);
        js->set("updateStatTime", (int)updateStatTime);
        js->set("force",          force ? 1 : 0);

        // Результат в стиле остальных API
        Object::Ptr out = new Object();
        out->set("result", "OK");
        out->set("status", js);
        return out;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBSlave::httpGetMyInfo( Poco::JSON::Object::Ptr root )
    {
        auto my = UniSetObject::httpGetMyInfo(root);
        my->set("extensionType", "MBSlave");
        return my;
    }
#endif
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::much_read(RegMap& rmap, const ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat,
                                            size_t count, const int fn )
    {
        mbinfo << myname << "(much_read): read mbID="
               << ModbusRTU::dat2str(reg) << "(" << (int)reg << ") " << " count=" << count
               << " mbfunc=" << fn << endl;

        auto it = rmap.end();
        size_t i = 0;

        int mbfunc = checkMBFunc ? fn : default_mbfunc;
        ModbusRTU::RegID regID = genRegID(reg, mbfunc);

        // ищем первый регистр из запроса... пропуская несуществующие..
        // ведь запросить могут начиная с "несуществующего регистра"
        for( ; i < count; i++ )
        {
            it = rmap.find(regID + i);

            if( it != rmap.end() )
            {
                regID += i;
                break;
            }

            dat[i] = 0;
        }

        if( it == rmap.end() )
            return ModbusRTU::erBadDataAddress;

        ModbusRTU::ModbusData val = 0;

        for( ; (it != rmap.end()) && (i < count); i++, regID++ )
        {
            val = 0;

            // если регистры идут не подряд, то просто вернём ноль
            if( it->first == regID )
            {
                real_read_it(rmap, it, val);
                ++it;
            }

            dat[i] = val;
        }

        // добиваем нулями "ответ"
        // чтобы он был такой длинны, которую запрашивали
        if( i < count )
        {
            for( ; i < count; i++ )
                dat[i] = 0;
        }

        return ModbusRTU::erNoError;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::bits_read( RegMap& rmap, const ModbusRTU::ModbusData reg,
            ModbusRTU::BitsBuffer* dat,
            size_t count, const int fn )
    {
        mbinfo << myname << "(bits_read): read mbID="
               << ModbusRTU::dat2str(reg) << "(" << (int)reg << ") " << " count=" << count
               << " mbfunc=" << fn << endl;

        auto it = rmap.end();
        size_t i = 0;

        int mbfunc = checkMBFunc ? fn : default_mbfunc;
        ModbusRTU::RegID regID = genRegID(reg, mbfunc);

        // ищем первый регистр из запроса... пропуская несуществующие..
        // ведь запросить могут начиная с "несуществующего регистра"
        for( ; i < count; i++ )
        {
            it = rmap.find(regID + i);

            if( it != rmap.end() )
            {
                regID += i;
                break;
            }

            dat->setByBitNum(i, false);
        }

        if( it == rmap.end() )
            return ModbusRTU::erBadDataAddress;

        ModbusRTU::ModbusData val = 0;

        for( ; (it != rmap.end()) && (i < count); i++, regID++ )
        {
            val = 0;

            // если регистры идут не подряд, то просто вернём ноль
            if( it->first == regID )
            {
                real_read_it(rmap, it, val);
                ++it;
            }

            dat->setByBitNum(i, val != 0 );
        }

        // добиваем нулями "ответ"
        // чтобы он был такой длинны, которую запрашивали
        if( i < count )
        {
            for( ; i < count; i++ )
                dat->setByBitNum(i, false);
        }

        return ModbusRTU::erNoError;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_read( RegMap& rmap, const ModbusRTU::ModbusData reg, ModbusRTU::ModbusData& val, const int fn )
    {
        ModbusRTU::RegID regID = checkMBFunc ? genRegID(reg, fn) : genRegID(reg, default_mbfunc);

        mbinfo << myname << "(real_read): read mbID="
               << ModbusRTU::dat2str(reg) << "(" << (int)reg << ")"  << " fn=" << fn
               << " regID=" << regID
               << " default_mbfunc=" << default_mbfunc
               << " checkMBFunc=" << checkMBFunc
               << endl;


        auto it = rmap.find(regID);
        return real_read_it(rmap, it, val);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_read_it( RegMap& rmap, RegMap::iterator& it, ModbusRTU::ModbusData& val )
    {
        if( it == rmap.end() )
            return ModbusRTU::erBadDataAddress;

        IOProperty* p(&it->second);

        mbinfo << myname << "(real_read_it): read mbID="
               << ModbusRTU::dat2str(p->mbreg) << "(" << (int)(p->mbreg) << ")" << endl;

        if( p->bitreg )
            return real_bitreg_read_it(p->bitreg, val);

        return real_read_prop(p, val);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::real_bitreg_read_it( std::shared_ptr<BitRegProperty>& bp, ModbusRTU::ModbusData& val )
    {
        mbinfo << myname << "(real_bitreg_read_it): read mbID="
               << ModbusRTU::dat2str(bp->mbreg) << "(" << (int)bp->mbreg << ")" << endl;

        ModbusRTU::DataBits16 d;

        for( size_t i = 0; i < ModbusRTU::BitsPerData; i++ )
        {
            IOProperty* p(&(bp->bvec[i]));

            if( p->si.id == DefaultObjectId )
            {
                d.set(i, 0);
                continue;
            }

            ModbusRTU::ModbusData v = 0;
            ModbusRTU::mbErrCode err = real_read_prop(p, v);

            if( err == ModbusRTU::erNoError )
                d.set(i, (bool)v);
            else
                d.set(i, 0);
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
                val = IOBase::processingAsDO(p, shm, force) ? 1 : 0;
            }
            else if( p->stype == UniversalIO::AI ||
                     p->stype == UniversalIO::AO )
            {
                if( p->vtype == VTypes::vtUnknown )
                {
                    val = IOBase::processingAsAO(p, shm, force);
                }
                else if( p->vtype == VTypes::vtF2 )
                {
                    float f = IOBase::processingFasAO(p, shm, force);

                    VTypes::F2 f2(f);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < f2.wsize()
                    val = f2.raw.v[p->wnum];
                }
                else if( p->vtype == VTypes::vtF2r )
                {
                    float f = IOBase::processingFasAO(p, shm, force);
                    VTypes::F2r f2(f);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < f2.wsize()
                    val = f2.raw_backorder.v[p->wnum];
                }
                else if( p->vtype == VTypes::vtF4 )
                {
                    float f = IOBase::processingFasAO(p, shm, force);
                    VTypes::F4 f4(f);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < f4.wsize()
                    val = f4.raw.v[p->wnum];
                }
                else if( p->vtype == VTypes::vtI2 )
                {
                    long v = IOBase::processingAsAO(p, shm, force);
                    VTypes::I2 i2(v);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < i2.wsize()
                    val = i2.raw.v[p->wnum];
                }
                else if( p->vtype == VTypes::vtI2r )
                {
                    long v = IOBase::processingAsAO(p, shm, force);
                    VTypes::I2r i2(v);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < i2.wsize()
                    val = i2.raw_backorder.v[p->wnum];
                }
                else if( p->vtype == VTypes::vtU2 )
                {
                    long v = IOBase::processingAsAO(p, shm, force);
                    VTypes::U2 u2(v);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < u2.wsize()
                    val = u2.raw.v[p->wnum];
                }
                else if( p->vtype == VTypes::vtU2r )
                {
                    long v = IOBase::processingAsAO(p, shm, force);
                    VTypes::U2r u2(v);
                    // оптимизируем и проверку не делаем
                    // считая, что при "загрузке" всё было правильно
                    // инициализировано
                    // if( p->wnum >=0 && p->wnum < u2.wsize()
                    val = u2.raw_backorder.v[p->wnum];
                }
                else
                    val = IOBase::processingAsAO(p, shm, force);
            }
            else
                return ModbusRTU::erBadDataAddress;

            mblog3 << myname << "(real_read_prop): read OK. sid=" << p->si.id << " val=" << val << endl;
            smPingOK = true;
            return ModbusRTU::erNoError;
        }
        catch( uniset::NameNotFound& ex )
        {
            mbwarn << myname << "(real_read_prop): " << ex << endl;
            return ModbusRTU::erBadDataAddress;
        }
        catch( uniset::OutOfRange& ex )
        {
            mbwarn << myname << "(real_read_prop): " << ex << endl;
            return ModbusRTU::erBadDataValue;
        }
        catch( const CORBA::SystemException& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(real_read_prop): CORBA::SystemException: "
                       << ex.NP_minorString() << endl;
        }
        catch( const uniset::Exception& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(real_read_prop): " << ex << endl;
        }
        catch(...)
        {
            if( smPingOK )
                mbcrit << myname << "(real_read_prop) catch ..." << endl;
        }

        mbwarn << myname << "(real_read_prop): read sid=" << p->si.id << " FAILED!!" << endl;

        smPingOK = false;
        return ModbusRTU::erTimeOut;
    }
    // -------------------------------------------------------------------------

    mbErrCode MBSlave::readInputRegisters( const ReadInputMessage& query, ReadInputRetMessage& reply )
    {
        mbinfo << myname << "(readInputRegisters): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(readInputRegisters): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        if( query.count <= 0 )
        {
            mbinfo << myname << "(readInputRegisters): BAD QUERY: count=0!" << endl;
            return ModbusRTU::erBadDataValue;
        }

        if( query.count == 1 )
        {
            ModbusRTU::ModbusData d = 0;
            ModbusRTU::mbErrCode ret = real_read(regmap->second, query.start, d, query.func);

            if( ret == ModbusRTU::erNoError )
                reply.addData(d);
            else
                reply.addData(0);

            return ret;
        }

        // Формирование ответа:
        ModbusRTU::mbErrCode ret = much_read(regmap->second, query.start, buf, query.count, query.func);

        if( ret == ModbusRTU::erNoError )
        {
            for( uint16_t i = 0; i < query.count; i++ )
                reply.addData( buf[i] );
        }

        return ret;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::setDateTime( const ModbusRTU::SetDateTimeMessage& query,
            ModbusRTU::SetDateTimeRetMessage& reply )
    {
        return ModbusServer::replySetDateTime(query, reply, mblog);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::remoteService( const ModbusRTU::RemoteServiceMessage& query,
            ModbusRTU::RemoteServiceRetMessage& reply )
    {
        //    cerr << "(remoteService): " << query << endl;
        return ModbusRTU::erOperationFailed;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::fileTransfer( const ModbusRTU::FileTransferMessage& query,
            ModbusRTU::FileTransferRetMessage& reply )
    {
        mbinfo << myname << "(fileTransfer): " << query << endl;

        auto it = flist.find(query.numfile);

        if( it == flist.end() )
            return ModbusRTU::erBadDataValue;

        std::string fname(it->second);
        return ModbusServer::replyFileTransfer( fname, query, reply, mblog );
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::readCoilStatus( const ReadCoilMessage& query,
            ReadCoilRetMessage& reply )
    {
        mbinfo << myname << "(readCoilStatus): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(readCoilStatus): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        if( query.count <= 0 )
        {
            mbinfo << myname << "(readCoilStatus): BAD QUERY: count=0!" << endl;
            return ModbusRTU::erBadDataValue;
        }

        try
        {
            if( query.count == 1 )
            {
                ModbusRTU::ModbusData d = 0;
                ModbusRTU::mbErrCode ret = real_read(regmap->second, query.start, d, query.func);
                reply.addData(0);

                if( ret == ModbusRTU::erNoError )
                    reply.setBit(0, 0, d);
                else
                    reply.setBit(0, 0, 0);

                smPingOK = true;
                return ret;
            }

            auto err = bits_read(regmap->second, query.start, &reply, query.count, query.func);

            if( err == ModbusRTU::erNoError )
                smPingOK = true;

            return err;
        }
        catch( uniset::NameNotFound& ex )
        {
            mbwarn << myname << "(readCoilStatus): " << ex << endl;
            return ModbusRTU::erBadDataAddress;
        }
        catch( const CORBA::SystemException& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(readCoilStatus): СORBA::SystemException: "
                       << ex.NP_minorString() << endl;
        }
        catch( const uniset::Exception& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(readCoilStatus): " << ex << endl;
        }
        catch(...)
        {
            if( smPingOK )
                mbcrit << myname << "(readCoilStatus): catch ..." << endl;
        }

        smPingOK = false;
        return ModbusRTU::erTimeOut;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::readInputStatus( const ReadInputStatusMessage& query,
            ReadInputStatusRetMessage& reply )
    {
        mbinfo << myname << "(readInputStatus): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(readInputStatus): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        if( query.count <= 0 )
        {
            mbinfo << myname << "(readInputStatus): BAD QUERY: count=0!" << endl;
            return ModbusRTU::erBadDataValue;
        }

        try
        {
            if( query.count == 1 )
            {
                ModbusRTU::ModbusData d = 0;
                ModbusRTU::mbErrCode ret = real_read(regmap->second, query.start, d, query.func);
                reply.addData(0);

                if( ret == ModbusRTU::erNoError )
                    reply.setBit(0, 0, d);
                else
                    reply.setBit(0, 0, 0);

                smPingOK = true;
                return ret;
            }

            auto err = bits_read(regmap->second, query.start, &reply, query.count, query.func);

            if( err == ModbusRTU::erNoError )
                smPingOK = true;

            return err;
        }
        catch( uniset::NameNotFound& ex )
        {
            mbwarn << myname << "(readInputStatus): " << ex << endl;
            return ModbusRTU::erBadDataAddress;
        }
        catch( const CORBA::SystemException& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(readInputStatus): СORBA::SystemException: "
                       << ex.NP_minorString() << endl;
        }
        catch( const uniset::Exception& ex )
        {
            if( smPingOK )
                mbcrit << myname << "(readInputStatus): " << ex << endl;
        }
        catch(...)
        {
            if( smPingOK )
                mbcrit << myname << "(readInputStatus): catch ..." << endl;
        }

        smPingOK = false;
        return ModbusRTU::erTimeOut;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::forceMultipleCoils( const ModbusRTU::ForceCoilsMessage& query,
            ModbusRTU::ForceCoilsRetMessage& reply )
    {
        mbinfo << myname << "(forceMultipleCoils): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(forceMultipleCoils): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
        size_t nbit = 0;

        int fn = getOptimizeWriteFunction(query.func);

        for( size_t i = 0; i < query.bcnt; i++ )
        {
            ModbusRTU::DataBits b(query.data[i]);

            for( size_t k = 0; k < ModbusRTU::BitsPerByte && nbit < query.quant; k++, nbit++ )
            {
                // ModbusRTU::mbErrCode ret =
                real_write(regmap->second, query.start + nbit, (b[k] ? 1 : 0), fn);
                //if( ret == ModbusRTU::erNoError )
            }
        }

        //if( ret == ModbusRTU::erNoError )
        if( nbit == query.quant )
            reply.set(query.start, query.quant);

        return ret;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::forceSingleCoil( const ModbusRTU::ForceSingleCoilMessage& query,
            ModbusRTU::ForceSingleCoilRetMessage& reply )
    {
        mbinfo << myname << "(forceSingleCoil): " << query << endl;

        auto regmap = iomap.find(query.addr);

        if( regmap == iomap.end() )
        {
            mbinfo << myname << "(forceSingleCoil): Unknown addr=" << ModbusRTU::addr2str(query.addr) << endl;
            return ModbusRTU::erTimeOut;
        }

        int fn = getOptimizeWriteFunction(query.func);
        ModbusRTU::mbErrCode ret = real_write(regmap->second, query.start, (query.cmd() ? 1 : 0), fn );

        if( ret == ModbusRTU::erNoError )
            reply.set(query.start, query.data);

        return ret;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::diagnostics( const ModbusRTU::DiagnosticMessage& query,
            ModbusRTU::DiagnosticRetMessage& reply )
    {
        auto mbserver = dynamic_pointer_cast<ModbusServer>(mbslot);

        if( !mbserver )
            return ModbusRTU::erHardwareError;

        if( query.subf == ModbusRTU::subEcho )
        {
            reply = query;
            return ModbusRTU::erNoError;
        }

        if( query.subf == ModbusRTU::dgBusErrCount )
        {
            reply = query;
            reply.data[0] =  mbserver->getErrCount(ModbusRTU::erBadCheckSum);
            return ModbusRTU::erNoError;
        }

        if( query.subf == ModbusRTU::dgMsgSlaveCount || query.subf == ModbusRTU::dgBusMsgCount )
        {
            reply = query;
            reply.data[0] = connCount;
            return ModbusRTU::erNoError;
        }

        if( query.subf == ModbusRTU::dgSlaveNAKCount )
        {
            reply = query;
            reply.data[0] =  mbserver->getErrCount(erOperationFailed);
            return ModbusRTU::erNoError;
        }

        if( query.subf == ModbusRTU::dgClearCounters )
        {
            mbserver->resetAskCounter();
            mbserver->resetErrCount(erOperationFailed, 0);
            mbserver->resetErrCount(ModbusRTU::erBadCheckSum, 0);
            // другие счётчики пока не сбрасываем..
            reply = query;
            return ModbusRTU::erNoError;
        }

        return ModbusRTU::erOperationFailed;
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode MBSlave::read4314( const ModbusRTU::MEIMessageRDI& query,
                                            ModbusRTU::MEIMessageRetRDI& reply )
    {
        mbinfo << "(read4314): " << query << endl;

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

        for( const auto& i : oit->second )
            reply.addData( i.first, i.second );

        return erNoError;
    }
    // -------------------------------------------------------------------------
    const std::string MBSlave::ClientInfo::getShortInfo() const
    {
        ostringstream s;

        s << iaddr << " askCount=" << askCount;

        return s.str();
    }
    // -------------------------------------------------------------------------
    uniset::SimpleInfo* MBSlave::getInfo( const char* userparam )
    {
        uniset::SimpleInfo_var i = UniSetObject::getInfo(userparam);

        auto sslot = dynamic_pointer_cast<ModbusTCPServerSlot>(mbslot);


        ostringstream inf;

        inf << i->info << endl;

        if( sslot ) // т.е. если у нас tcp
        {
            inf << "TCPModbusSlave: " << sslot->getInetAddress() << ":" << sslot->getInetPort() << endl;
        }

        inf << vmon.pretty_str() << endl;

        inf << "LogServer:  " << logserv_host << ":" << logserv_port << endl;
        inf << "iomap[" << iomap.size() << "]: " << endl;

        for( const auto& m : iomap )
            inf << "  " << ModbusRTU::addr2str(m.first) << ": iomap=" << m.second.size() << endl;

        inf << " myaddr: " << ModbusServer::vaddr2str(vaddr) << endl;
        inf << "Statistic:"
            << " connectionCount=" << connCount
            << " smPingOK=" << smPingOK;

        if( tcpserver )
            inf << " restartTCPServerCount=" << restartTCPServerCount;

        inf << endl;

        if( sslot ) // т.е. если у нас tcp
        {
            inf << "TCP: " << sslot->getInetAddress() << ":" << sslot->getInetPort() << endl;
        }


        if( tcpserver )
        {
            size_t snum = 0;
            {
                std::lock_guard<std::mutex> l(sessMutex);
                snum = sess.size();
            }

            inf << "TCP Clients[" << snum << "]:" << endl;

            for( const auto& m : cmap )
                inf << "   " << m.second.getShortInfo() << endl;

            {
                std::lock_guard<std::mutex> l(sessMutex);
                inf << "TCP sessions[" << sess.size() << "]: max=" << sessMaxNum << " updateStatTime=" << updateStatTime << endl;

                for( const auto& m : sess )
                    inf << "   " << m.iaddr << " askCount=" << m.askCount << endl;
            }
            inf << endl;
        }

        i->info = inf.str().c_str();
        return i._retn();
    }

    // ----------------------------------------------------------------------------
    void MBSlave::initTCPClients( UniXML::iterator confnode )
    {
        auto conf = uniset_conf();
        UniXML::iterator cit(confnode);

        if( cit.find("clients") && cit.goChildren() )
        {
            for( ; cit; cit++ )
            {
                ClientInfo c;
                c.iaddr = cit.getProp("ip");

                if( c.iaddr.empty() )
                {
                    ostringstream err;
                    err << myname << "(init): Unknown ip=''";
                    mbcrit << err.str() << endl;
                    throw SystemError(err.str());
                }

                // resolve (если получиться)
                Poco::Net::SocketAddress sa(c.iaddr);
                c.iaddr = sa.host().toString();

                if( !cit.getProp("respond").empty() )
                {
                    c.respond_s = conf->getSensorID(cit.getProp("respond"));

                    if( c.respond_s == DefaultObjectId )
                    {
                        ostringstream err;
                        err << myname << "(init): Not found sensor ID for " << cit.getProp("respond");
                        mbcrit << err.str() << endl;
                        throw SystemError(err.str());
                    }
                }

                if( !cit.getProp("askcount").empty() )
                {
                    c.askcount_s = conf->getSensorID(cit.getProp("askcount"));

                    if( c.askcount_s == DefaultObjectId )
                    {
                        ostringstream err;
                        err << myname << "(init): Not found sensor ID for " << cit.getProp("askcount");
                        mbcrit << err.str() << endl;
                        throw SystemError(err.str());
                    }
                }

                c.invert = cit.getIntProp("invert");

                if( !cit.getProp("timeout").empty() )
                {
                    c.tout = cit.getIntProp("timeout");
                    c.ptTimeout.setTiming(c.tout);
                }

                cmap[c.iaddr] = c;
                mbinfo << myname << "(init): add client: " << c.iaddr << " respond=" << c.respond_s << " askcount=" << c.askcount_s << endl;
            }
        }
    }
    // ----------------------------------------------------------------------------
} // end of namespace uniset
