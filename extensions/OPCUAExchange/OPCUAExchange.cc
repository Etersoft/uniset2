/*
 * Copyright (c) 2023 Pavel Vainerman.
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
#include <sstream>
#include <limits>
#include "ORepHelpers.h"
#include "UniSetTypes.h"
#include "Extensions.h"
#include "OPCUAExchange.h"
#include "OPCUALogSugar.h"
// -----------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    using namespace std;
    using namespace extensions;
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const std::shared_ptr<OPCUAExchange::OPCAttribute>& inf )
    {
        return os << *(inf.get());
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const OPCUAExchange::OPCAttribute& inf )
    {
        os << "(" << inf.si.id << ")" << uniset_conf()->oind->getMapName(inf.si.id)
           << " name=" << inf.request[0].attr;

        if( inf.cal.minRaw != inf.cal.maxRaw )
            os << inf.cal;

        return os;
    }
    // -----------------------------------------------------------------------------

    OPCUAExchange::OPCUAExchange(uniset::ObjectId id, uniset::ObjectId icID,
                                 const std::shared_ptr<SharedMemory>& ic, const std::string& prefix_ ):
        UniSetObject(id),
        polltime(150),
        iolist(50),
        maxItem(0),
        filterT(0),
        myid(id),
        prefix(prefix_),
        sidHeartBeat(uniset::DefaultObjectId),
        force(false),
        force_out(false),
        activated(false),
        readconf_ok(false)
    {
        auto conf = uniset_conf();

        string cname = conf->getArgParam("--" + prefix + "-confnode", myname);
        confnode = conf->getNode(cname);

        if( confnode == NULL )
            throw SystemError("Not found conf-node " + cname + " for " + myname);

        prop_prefix = "";

        opclog = make_shared<DebugStream>();
        opclog->setLogName(myname);
        conf->initLogStream(opclog, prefix + "-log");

        loga = make_shared<LogAgregator>(myname + "-loga");
        loga->add(opclog);
        loga->add(ulog());
        loga->add(dlog());

        UniXML::iterator it(confnode);

        logserv = make_shared<LogServer>(loga);
        logserv->init( prefix + "-logserver", confnode );

        if( findArgParam("--" + prefix + "-run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
        {
            logserv_host = conf->getArg2Param("--" + prefix + "-logserver-host", it.getProp("logserverHost"), "localhost");
            logserv_port = conf->getArgPInt("--" + prefix + "-logserver-port", it.getProp("logserverPort"), getId());
        }

        if( ic )
            ic->logAgregator()->add(loga);


        for( size_t i = 0; i < channels; i++ )
        {
            const std::string num = std::to_string(i + 1);
            const std::string defAddr = (i == 0 ? "opc.tcp://localhost:4840/" : "");

            addr[i] = conf->getArg2Param("--" + prefix + "-addr" + num, it.getProp("addr" + num), defAddr);
            user[i] = conf->getArg2Param("--" + prefix + "-user" + num, it.getProp("user" + num), "");
            pass[i] = conf->getArg2Param("--" + prefix + "-pass" + num, it.getProp("pass" + num), "");
            client[i] = make_shared<OPCUAClient>();
        }

        if( addr[0].empty() )
        {
            ostringstream err;
            err << myname << "(init): Unknown OPC server address for channel N1";
            opccrit << myname << "(init): " << err.str() << endl;
            throw SystemError(err.str());
        }

        polltime = conf->getArgPInt("--" + prefix + "-polltime", it.getProp("polltime"), polltime);
        vmonit(polltime);

        updatetime = conf->getArgPInt("--" + prefix + "-updatetime", it.getProp("updatetime"), updatetime);
        vmonit(updatetime);

        reconnectPause = conf->getArgPInt("--" + prefix + "-reconnectPause", it.getProp("reconnectPause"), reconnectPause);
        vmonit(reconnectPause);

        force         = conf->getArgInt("--" + prefix + "-force", it.getProp("force"));
        force_out     = conf->getArgInt("--" + prefix + "-force-out", it.getProp("force_out"));

        vmonit(force);
        vmonit(force_out);

        filtersize = conf->getArgPInt("--" + prefix + "-filtersize", it.getProp("filtersize"), 1);
        filterT = atof(conf->getArgParam("--" + prefix + "-filterT", it.getProp("filterT")).c_str());

        vmonit(filtersize);
        vmonit(filterT);

        shm = make_shared<SMInterface>(icID, ui, myid, ic);

        // определяем фильтр
        s_field = conf->getArgParam("--" + prefix + "-s-filter-field");
        s_fvalue = conf->getArgParam("--" + prefix + "-s-filter-value");

        vmonit(s_field);
        vmonit(s_fvalue);

        opcinfo << myname << "(init): read s_field='" << s_field
                << "' s_fvalue='" << s_fvalue << "'" << endl;

        int sm_tout = conf->getArgInt("--" + prefix + "-sm-ready-timeout", it.getProp("ready_timeout"));

        if( sm_tout == 0 )
            smReadyTimeout = conf->getNCReadyTimeout();
        else if( sm_tout < 0 )
            smReadyTimeout = UniSetTimer::WaitUpTime;
        else
            smReadyTimeout = sm_tout;

        vmonit(smReadyTimeout);

        string sm_ready_sid = conf->getArgParam("--" + prefix + "-sm-ready-test-sid", it.getProp("sm_ready_test_sid"));
        sidTestSMReady = conf->getSensorID(sm_ready_sid);

        if( sidTestSMReady == DefaultObjectId )
        {
            sidTestSMReady = conf->getSensorID("TestMode_S");
            opcwarn << myname
                    << "(init): Unknown ID for sm-ready-test-sid (--" << prefix << "-sm-ready-test-sid)."
                    << " Use 'TestMode_S' (if present)" << endl;
        }
        else
            opcinfo << myname << "(init): sm-ready-test-sid: " << sm_ready_sid << endl;

        vmonit(sidTestSMReady);

        // -----------------------
        string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", it.getProp("heartbeat_id"));

        if( !heart.empty() )
        {
            sidHeartBeat = conf->getSensorID(heart);

            if( sidHeartBeat == DefaultObjectId )
            {
                ostringstream err;
                err << myname << ": Not found ID for 'HeartBeat' " << heart;
                opccrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }

            int heartbeatTime = conf->getArgInt("--heartbeat-check-time", "1000");

            if( heartbeatTime )
                ptHeartBeat.setTiming(heartbeatTime);
            else
                ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

            maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", it.getProp("heartbeat_max"), 10);
        }

        activateTimeout    = conf->getArgPInt("--" + prefix + "-activate-timeout", 25000);

        vmonit(activateTimeout);

        if( !shm->isLocalwork() ) // ic
            ic->addReadItem( sigc::mem_fun(this, &OPCUAExchange::readItem) );


        ThreadCreator<OPCUAExchange>::Action funcs[] =
        {
            &OPCUAExchange::channel1Exchange,
            &OPCUAExchange::channel2Exchange
        };

        static_assert(sizeof(funcs) / sizeof(&OPCUAExchange::channel2Exchange) == channels, "num thread functions != num channels");

        if( sizeof(funcs) < channels )
        {
            ostringstream err;
            err << myname << ": Runtime error: thread function[" << sizeof(funcs) << "] < channels[" << channels << "]";
            cerr << myname << "(init): " << err.str() << endl;
            throw SystemError(err.str());
        }

        for( size_t i = 0; i < channels; i++ )
        {
            thrChannel[i] = make_shared<ThreadCreator<OPCUAExchange> >(this, funcs[i]);
        }

        vmonit(sidHeartBeat);
    }

    // --------------------------------------------------------------------------------

    OPCUAExchange::~OPCUAExchange()
    {
    }

    // --------------------------------------------------------------------------------
    bool OPCUAExchange::tryConnect(size_t chan)
    {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */

        if( !user[chan].empty() )
            return client[chan]->connect(addr[chan], user[chan], pass[chan]);

        return client[chan]->connect(addr[chan]);
    }
    // --------------------------------------------------------------------------------
    bool OPCUAExchange::prepare()
    {
        //    set_signals(true);
        UniXML::iterator it(confnode);

        PassiveTimer pt(UniSetTimer::WaitUpTime);

        if( shm->isLocalwork() )
        {
            maxItem = 0;

            try
            {
                readConfiguration();
            }
            catch( std::exception& ex)
            {
                cerr << "(readConfiguration): " << ex.what() << endl;
                uterminate();
            }

            if( sidTestSMReady == uniset::DefaultObjectId && !iolist.empty() )
                sidTestSMReady = iolist[0]->si.id;

            iolist.resize(maxItem);
            cerr << "************************** readConfiguration: " << pt.getCurrent() << " msec " << endl;

            if( !waitSM() ) // необходимо дождаться, чтобы нормально инициализировать итераторы
            {
                if( !cancelled )
                    uterminate();

                return false;
            }
        }
        else
        {
            if( !waitSM() ) // необходимо дождаться, чтобы нормально инициализировать итераторы
            {
                if( !cancelled )
                    uterminate();

                return false;
            }

            iolist.resize(maxItem);

            // init iterators
            for( const auto& it : iolist )
            {
                shm->initIterator(it->ioit);
                shm->initIterator(it->t_ait);
            }

            readconf_ok = true; // т.к. waitSM() уже был...
        }

        opcinfo << myname << "(prepare): iolist size = " << iolist.size() << endl;

        buildRequests();

        bool skip_iout = uniset_conf()->getArgInt("--" + prefix + "-skip-init-output");

        if( !skip_iout && tryConnect(0) )
            initOutputs();

        shm->initIterator(itHeartBeat);
        PassiveTimer ptAct(activateTimeout);

        while( !activated && !cancelled && !ptAct.checkTime() )
        {
            cout << myname << "(prepare): wait activate..." << endl;
            msleep(300);

            if( activated )
            {
                cout << myname << "(prepare): activate OK.." << endl;
                break;
            }
        }

        if( cancelled )
            return false;

        if( !activated )
            opccrit << myname << "(prepare): ************* don`t activate?! ************" << endl;

        return activated;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channel1Exchange()
    {
        opcinfo << myname << "(channel1Exchange): run..." << endl;
        Tick tick = 1;
        size_t chan = 0;

        while( !cancelled )
        {
            try
            {
                if( !tryConnect(chan) )
                {
                    opccrit << myname << "(channel1Exchange): no connection to " << addr[chan] << endl;
                    msleep(reconnectPause);
                    continue;
                }

                if( tick >= std::numeric_limits<Tick>::max() )
                    tick = 1;

                updateToChannel(chan);
                channelExchange(tick++, chan);

                if( currentChannel == chan )
                    updateFromChannel(chan);
            }
            catch( const CORBA::SystemException& ex )
            {
                opclog3 << myname << "(channel1Exchange): CORBA::SystemException: "
                        << ex.NP_minorString() << endl;
            }
            catch( const uniset::Exception& ex )
            {
                opclog3 << myname << "(channel1Exchange): " << ex << endl;
            }
            catch(...)
            {
                opclog3 << myname << "(channel1Exchange): catch ..." << endl;
            }

            if( cancelled )
                break;

            msleep( polltime );
        }

        opcinfo << myname << "(channel1Exchange): terminated..." << endl;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channel2Exchange()
    {
        opcinfo << myname << "(channel2Exchange): run..." << endl;
        Tick tick = 1;
        size_t chan = 1;

        if( addr[chan].empty() )
        {
            opcwarn << myname << "(channel2Exchange): DISABLED (unknown OPC server address)..." << endl;
            return;
        }

        while( !cancelled )
        {
            try
            {
                if( !tryConnect(chan) )
                {
                    opccrit << myname << "(channel2Exchange): no connection to " << addr[chan] << endl;
                    msleep(reconnectPause);
                    continue;
                }

                if( tick >= std::numeric_limits<Tick>::max() )
                    tick = 1;

                updateToChannel(chan);
                channelExchange(tick++, chan);

                if( currentChannel == chan )
                    updateFromChannel(chan);
            }
            catch( const CORBA::SystemException& ex )
            {
                opclog3 << myname << "(channel2Exchange): CORBA::SystemException: "
                        << ex.NP_minorString() << endl;
            }
            catch( const uniset::Exception& ex )
            {
                opclog3 << myname << "(channel2Exchange): " << ex << endl;
            }
            catch(...)
            {
                opclog3 << myname << "(channel2Exchange): catch ..." << endl;
            }

            if( cancelled )
                break;

            msleep( polltime );
        }

        opcinfo << myname << "(channel2Exchange): terminated..." << endl;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channelExchange( Tick tick, size_t chan )
    {
        for( auto&& v : writeRequests[chan] )
        {
            if( v.first == 0 || tick % v.first == 0 )
            {
                opclog4 << myname << "(channelExchange): write[" << (int)v.first << "] " << v.second.size() << " attrs" << endl;

                if(client[chan]->write32(v.second) != UA_STATUSCODE_GOOD )
                    opcwarn << myname << "(prepare): tick=" << (int)tick << " write error" << endl;
            }
        }

        for( auto&& v : readRequests[chan] )
        {
            if( v.first == 0 || tick % v.first == 0 )
            {
                opclog4 << myname << "(channelExchange): read[" << (int)v.first << "] " << v.second.size() << " attrs" << endl;

                if(client[chan]->read32(v.second) != UA_STATUSCODE_GOOD )
                    opcwarn << myname << "(prepare): tick=" << (int)tick << " read error" << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateFromSM()
    {
        if( !force_out )
            return;

        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId )
            {
                opclog3 << myname << "(updateFromSM): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AO )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = IOBase::processingAsAO(ib, shm, force);
                    }
                    opclog3 << myname << "(updateFromSM): write AO"
                            << " sid=" << io->si.id
                            << " value=" << io->val
                            << endl;
                }
                else if( io->stype == UniversalIO::DO )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = IOBase::processingAsDO(ib, shm, force) ? 1 : 0;
                    }
                    opclog3 << myname << "(updateFromSM): write DO"
                            << " sid=" << io->si.id
                            << " value=" << io->val
                            << endl;
                }
            }
            catch (const std::exception& ex)
            {
                opclog3 << myname << "(updateFromSM): " << ex.what() << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateToChannel( size_t chan )
    {
        if( !force_out )
            return;

        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId )
            {
                opclog4 << myname << "(updateToChannel): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AO )
                {
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        io->request[chan].value = io->val;
                    }
                    opclog4 << myname << "(updateToChannel): write AO"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request[chan].attr
                            << " val=" << io->request[chan].value
                            << endl;
                }
                else if( io->stype == UniversalIO::DO )
                {
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        io->request[chan].value = io->val ? 1 : 0;
                    }
                    opclog4 << myname << "(updateToChannel): write DO"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request[chan].attr
                            << " val=" << io->request[chan].value
                            << endl;
                }
            }
            catch (const std::exception& ex)
            {
                opclog4 << myname << "(updateToChannel): " << ex.what() << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateFromChannel( size_t chan )
    {
        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId)
            {
                opclog5 << myname << "(updateFromChannel): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AI )
                {
                    opclog5 << myname << "(updateFromChannel): read AI"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request[chan].attr
                            << " val=" << io->request[chan].value
                            << endl;
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = io->request[chan].value;
                    }
                }
                else if( io->stype == UniversalIO::DI )
                {
                    opclog5 << myname << "(updateFromChannel): read DI"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request[chan].attr
                            << " val=" << io->request[chan].value
                            << endl;
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = io->request[chan].value ? 1 : 0;
                    }
                }
            }
            catch( const std::exception& ex )
            {
                opclog5 << myname << "(updateFromChannel): " << ex.what() << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::writeToSM()
    {
        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId)
            {
                opclog3 << myname << "(writeToSM): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AI )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        opclog3 << myname << "(writeToSM): read AI"
                                << " sid=" << io->si.id
                                << " val=" << io->val
                                << endl;
                        IOBase::processingAsAI(ib, io->val, shm, force);
                    }
                }
                else if( io->stype == UniversalIO::DI )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        opclog3 << myname << "(writeToSM): read DI"
                                << " sid=" << io->si.id
                                << " val=" << io->val
                                << endl;
                        IOBase::processingAsDI(ib, io->val != 0, shm, force);
                    }
                }
            }
            catch( const std::exception& ex )
            {
                opclog3 << myname << "(writeToSM): " << ex.what() << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::readConfiguration()
    {
        readconf_ok = false;
        xmlNode* root = uniset_conf()->getXMLSensorsSection();

        if(!root)
        {
            ostringstream err;
            err << myname << "(readConfiguration): section <sensors> not found";
            throw SystemError(err.str());
        }

        UniXML::iterator it(root);

        if( !it.goChildren() )
        {
            opcwarn << myname << "(readConfiguration): section <sensors> empty?!!\n";
            return;
        }

        for( ; it.getCurrent(); it.goNext() )
        {
            if( uniset::check_filter(it, s_field, s_fvalue) )
                initIOItem(it);
        }

        readconf_ok = true;
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
    {
        if( uniset::check_filter(it, s_field, s_fvalue) )
            initIOItem(it);

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::initIOItem( UniXML::iterator& it )
    {
        auto inf = make_shared<OPCAttribute>();

        const string attr = it.getProp(prop_prefix + "opc_attr");

        if( attr.empty() )
        {
            opcwarn << myname << "(readItem): Unknown OPC UA attribute name. Use opc_attr='name'"
                    << " for " << it.getProp("name") << endl;
            return false;
        }

        if( attr.size() > 2 )
        {
            if( attr[0] == 'i' && attr[1] == '=' )
            {
                inf->request[0].attrNum = uni_atoi(attr.substr(2));
                inf->request[0].attr = attr;
            }
            else if( attr[0] == 's' && attr[1] == '=' )
                inf->request[0].attr = attr.substr(2); // remove prefix 's='
            else
                inf->request[0].attr = attr;
        }
        else
        {
            inf->request[0].attr = attr;
        }

        inf->request[0].nsIndex = it.getPIntProp(prop_prefix + "opc_ns_index", 0);

        if( inf->request[0].attr.empty() )
        {
            opcwarn << myname << "(readItem): Unknown OPC UA attribute name. Use opc_attr='nnnn'"
                    << " for " << it.getProp("name") << endl;
            return false;
        }

        if( !IOBase::initItem(inf.get(), it, shm, prop_prefix, false, opclog, myname, filtersize, filterT) )
            return false;

        if( inf->stype == UniversalIO::DO || inf->stype == UniversalIO::DI )
            inf->request[0].type = UA_TYPES_BOOLEAN;
        else
            inf->request[0].type = UA_TYPES_INT32;

        // если вектор уже заполнен
        // то увеличиваем его на 30 элементов (с запасом)
        // после инициализации делается resize
        // под реальное количество
        if( maxItem >= iolist.size() )
            iolist.resize(maxItem + 30);

        inf->tick = (uint8_t)IOBase::initIntProp(it, "opc_tick", prop_prefix, false);

        // значит это пороговый датчик..
        if( inf->t_ai != DefaultObjectId )
        {
            opclog3 << myname << "(readItem): add threshold '" << it.getProp("name")
                    << " for '" << uniset_conf()->oind->getNameById(inf->t_ai) << endl;
            std::swap(iolist[maxItem++], inf);
            return true;
        }

        inf->request[0].makeNodeId();
        size_t i = 1;

        while( i < channels )
        {
            inf->request[i] = inf->request[0];
            inf->request[i].makeNodeId();
            i++;
        }

        opclog3 << myname << "(readItem): add: " << inf->stype << " " << inf << endl;
        std::swap(iolist[maxItem++], inf);
        return true;
    }
    // ------------------------------------------------------------------------------------------

    bool OPCUAExchange::activateObject()
    {
        // блокирование обработки Startup
        // пока не пройдёт инициализация датчиков
        // см. sysCommand()
        {
            activated = false;
            UniSetObject::activateObject();
            activated = true;
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::deactivateObject()
    {
        cancelled = true;

        for( size_t i = 0; i < channels; i++ )
        {
            if( thrChannel[i] && thrChannel[i] ->isRunning() )
                thrChannel[i]->join();
        }

        // выставляем безопасные состояния
# if 0

        for( const auto& it : iolist )
        {
            if( it->ignore )
                continue;

            try
            {
                if( it->stype == UniversalIO::DO  )
                {
                    uniset_rwmutex_wrlock lock(it->val_lock);
                    bool set = it->invert ? !((bool)it->safeval) : (bool)it->safeval;

                    for( size_t i = 0; i < channels; i++)
                    {
                        if( !addr[i].empty() )
                            client[i]->set(it->request[i].attr, set, it->request[i].nsIndex);
                    }
                }
                else if( it->stype == UniversalIO::AO )
                {
                    uniset_rwmutex_wrlock lock(it->val_lock);

                    for( size_t i = 0; i < channels; i++)
                    {
                        if( !addr[i].empty() )
                            client[i]->write32(it->request[i].attr, it->safeval, it->request[i].nsIndex);
                    }
                }
            }
            catch( const std::exception& ex )
            {
                opclog3 << myname << "(sigterm): " << ex.what() << endl;
            }
        }

#endif
        return UniSetObject::deactivateObject();
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::initOutputs()
    {
        // выставляем значение по умолчанию
        for( const auto& it : iolist )
        {
            if( it->ignore )
                continue;

            try
            {
                if( it->stype == UniversalIO::DO )
                {
                    for( size_t i = 0; i < channels; i++ )
                    {
                        if( !addr[i].empty() )
                            client[i]->set(it->request[i].attr, (bool) it->defval);
                    }
                }
                else if( it->stype == UniversalIO::AO )
                {
                    for (size_t i = 0; i < channels; i++)
                    {
                        if( !addr[i].empty() )
                            client[i]->write32(it->request[i].attr, it->defval);
                    }
                }
            }
            catch( const uniset::Exception& ex )
            {
                opclog3 << myname << "(initOutput): " << ex << endl;
            }
        }
    }
    // -----------------------------------------------------------------------------
    std::shared_ptr<OPCUAExchange> OPCUAExchange::init_opcuaexchange(int argc, const char* const* argv,
            uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
            const std::string& prefix )
    {
        auto conf = uniset_conf();
        string name = conf->getArgParam("--" + prefix + "-name", "OPCUAExchange1");

        if( name.empty() )
        {
            cerr << "(opcuaclient): Unknown name. Use --" << prefix << "-name " << endl;
            return 0;
        }

        ObjectId ID = conf->getObjectID(name);

        if( ID == uniset::DefaultObjectId )
        {
            cerr << "(opcuaclient): Unknown ID for " << name
                 << "' Not found in <objects>" << endl;
            return 0;
        }

        dinfo << "(opcuaclient): name = " << name << "(" << ID << ")" << endl;
        return make_shared<OPCUAExchange>(ID, icID, ic, prefix);
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::help_print( int argc, const char* const* argv )
    {
        cout << "--prefix-confnode name     - Использовать для настройки указанный xml-узел" << endl;
        cout << "--prefix-name name         - ID процесса. По умолчанию OPCUAExchangeler1." << endl;
        cout << "--prefix-polltime msec     - Пауза между опросом карт. По умолчанию 200 мсек." << endl;
        cout << "--prefix-filtersize val    - Размерность фильтра для аналоговых входов." << endl;
        cout << "--prefix-filterT val       - Постоянная времени фильтра." << endl;
        cout << "--prefix-s-filter-field    - Идентификатор в configure.xml по которому считывается список относящихся к это процессу датчиков" << endl;
        cout << "--prefix-s-filter-value    - Значение идентификатора по которому считывается список относящихся к это процессу датчиков" << endl;
        cout << "--prefix-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-датчиком." << endl;
        cout << "--prefix-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
        cout << "--prefix-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
        cout << "--prefix-force             - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
        cout << "--prefix-force-out         - Обновлять выходы принудительно (не по заказу)" << endl;
        cout << "--prefix-skip-init-output  - Не инициализировать 'выходы' при старте" << endl;
        cout << "--prefix-sm-ready-test-sid - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;
        cout << endl;
        cout << " OPC UA: N=[1,2]" << endl;
        cout << "--prefix-addr addrN        - OPC UA server N address. Default: opc.tcp://localhost:4840/" << endl;
        cout << "--prefix-user userN        - OPC UA server N auth user. Default: ''(not used)" << endl;
        cout << "--prefix-pass passN        - OPC UA server N auth pass. Default: ''(not used)" << endl;
        cout << "--prefix-reconnectPause msec  - Pause between reconnect to server, milliseconds" << endl;
        cout << " Logs: " << endl;
        cout << endl;
        cout << "--prefix-log-...            - log control" << endl;
        cout << "             add-levels ..." << endl;
        cout << "             del-levels ..." << endl;
        cout << "             set-levels ..." << endl;
        cout << "             logfile filaname" << endl;
        cout << "             no-debug " << endl;
        cout << " LogServer: " << endl;
        cout << "--prefix-run-logserver       - run logserver. Default: localhost:id" << endl;
        cout << "--prefix-logserver-host ip   - listen ip. Default: localhost" << endl;
        cout << "--prefix-logserver-port num  - listen port. Default: ID" << endl;
        cout << LogServer::help_print("prefix-logserver") << endl;
    }
    // -----------------------------------------------------------------------------
    SimpleInfo* OPCUAExchange::getInfo( const char* userparam )
    {
        uniset::SimpleInfo_var i = UniSetObject::getInfo(userparam);

        ostringstream inf;

        inf << i->info << endl;

        inf << "LogServer:  " << logserv_host << ":" << logserv_port << endl;

        if( logserv )
            inf << logserv->getShortInfo() << endl;
        else
            inf << "No logserver running." << endl;


        inf << endl;
        inf << "iolist: " << iolist.size() << endl;
        inf << "write requests[" << writeRequests[0].size() << "]" << endl;

        for(auto it = writeRequests[0].begin(); it != writeRequests[0].end(); it++ )
            inf << "  [" << (int)it->first << "]: " << it->second.size() << " attrs" << endl;

        inf << endl;
        inf << "read requests[" << readRequests[0].size() << "]" << endl;

        for(auto it = readRequests[0].begin(); it != readRequests[0].end(); it++ )
            inf << "  [" << (int)it->first << "]: " << it->second.size() << " attrs" << endl;

        inf << endl;

        inf << vmon.pretty_str() << endl;

        i->info = inf.str().c_str();
        return i._retn();
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::sysCommand( const SystemMessage* sm )
    {
        switch( sm->command )
        {
            case SystemMessage::StartUp:
            {
                if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
                {
                    opcinfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
                    logserv->async_run(logserv_host, logserv_port);
                }

                PassiveTimer ptAct(activateTimeout);

                while( !cancelled && !activated && !ptAct.checkTime() )
                {
                    opcinfo << myname << "(sysCommand): wait activate..." << endl;
                    msleep(300);

                    if( activated )
                        break;
                }

                if( cancelled )
                    return;

                if( !activated )
                    opccrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl;

                if( !prepare() )
                    return;

                for( size_t i = 0; i < channels; i++ )
                {
                    if (thrChannel[i] && !thrChannel[i]->isRunning())
                        thrChannel[i]->start();
                }

                askSensors(UniversalIO::UIONotify);
                askTimer(tmUpdates, updatetime);
                break;
            }

            case SystemMessage::FoldUp:
            case SystemMessage::Finish:
                askSensors(UniversalIO::UIODontNotify);
                askTimer(tmUpdates, 0);
                break;

            case SystemMessage::WatchDog:
            {
                // ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
                // Если идёт локальная работа
                // (т.е. OPCUAExchange запущен в одном процессе с SharedMemory2)
                // то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
                // при заказе датчиков, а если SM вылетит, то вместе с этим процессом(OPCUAExchange)
                if( shm->isLocalwork() )
                    break;

                askSensors(UniversalIO::UIONotify);
                askTimer(tmUpdates, updatetime);
            }
            break;

            case SystemMessage::LogRotate:
            {
                opclogany << myname << "(sysCommand): logRotate" << endl;
                string fname = opclog->getLogFile();

                if( !fname.empty() )
                {
                    opclog->logFile(fname, true);
                    opclogany << myname << "(sysCommand): ***************** LOG ROTATE *****************" << endl;
                }
            }
            break;

            default:
                break;
        }
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::askSensors( UniversalIO::UIOCommand cmd )
    {
        if( force_out )
            return;

        if( !waitSM() )
        {
            if( !cancelled )
                uterminate();
        }

        PassiveTimer ptAct(activateTimeout);

        while( !cancelled && !readconf_ok && !ptAct.checkTime() )
        {
            opcinfo << myname << "(askSensors): wait read configuration..." << endl;
            msleep(50);

            if( readconf_ok )
                break;
        }

        if( cancelled )
            return;

        if( !readconf_ok )
            opccrit << myname << "(askSensors): ************* don`t read configuration?! ************" << endl;

        for( const auto& it : iolist )
        {
            if( it->ignore )
                continue;

            if( it->stype == UniversalIO::AO || it->stype == UniversalIO::DO )
            {
                try
                {
                    shm->askSensor(it->si.id, cmd, myid);
                }
                catch( const uniset::Exception& ex )
                {
                    opccrit << myname << "(askSensors): " << ex << endl;
                }
            }
        }
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::timerInfo( const uniset::TimerMessage* tm )
    {
        if( tm->id == tmUpdates )
        {
            updateFromSM();
            writeToSM();

            if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
            {
                shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, myid);
                ptHeartBeat.reset();
            }
        }
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::sensorInfo( const uniset::SensorMessage* sm )
    {
        opclog1 << myname << "(sensorInfo): sm->id=" << sm->id
                << " val=" << sm->value << endl;

        if( force_out )
            return;

        for( auto&& it : iolist )
        {
            if( it->si.id == sm->id )
            {
                opcinfo << myname << "(sensorInfo): sid=" << sm->id
                        << " value=" << sm->value
                        << endl;

                if( it->stype == UniversalIO::AO )
                {
                    uniset::uniset_rwmutex_wrlock lock(it->vmut);
                    IOBase* ib = static_cast<IOBase*>(it.get());
                    it->val = IOBase::processingAsAO(ib, shm, force);
                }
                else if( it->stype == UniversalIO::DO )
                {
                    uniset::uniset_rwmutex_wrlock lock(it->vmut);
                    IOBase* ib = static_cast<IOBase*>(it.get());
                    it->val = IOBase::processingAsDO(ib, shm, force) ? 1 : 0;
                }

                break;
            }
        }
    }
    // -----------------------------------------------------------------------------
    bool OPCUAExchange::waitSM()
    {
        if( !shm->waitSMreadyWithCancellation(smReadyTimeout, cancelled, 50) )
        {
            if( !cancelled )
            {
                ostringstream err;
                err << myname << "(execute): did not wait for the ready 'SharedMemory'. Timeout "
                    << smReadyTimeout << " msec";

                opccrit << err.str() << endl;
            }

            return false;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::buildRequests()
    {
        for( const auto& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->stype == UniversalIO::AO || io->stype == UniversalIO::DO )
            {
                for( size_t i = 0; i < channels; i++ )
                {
                    auto it = writeRequests[i].find(io->tick);

                    if( it == writeRequests[i].end() )
                    {
                        auto ret = writeRequests[i].emplace(io->tick, Request{});
                        it = ret.first;
                    }

                    for( size_t i = 0; i < channels; i++ )
                        it->second.push_back(&io->request[i]);
                }
            }
            else if( io->stype == UniversalIO::AI || io->stype == UniversalIO::DI )
            {
                for( size_t i = 0; i < channels; i++ )
                {
                    auto it = readRequests[i].find(io->tick);

                    if( it == readRequests[i].end() )
                    {
                        auto ret = readRequests[i].emplace(io->tick, Request{});
                        it = ret.first;
                    }

                    for( size_t i = 0; i < channels; i++ )
                        it->second.push_back(&io->request[i]);
                }
            }
        }
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
