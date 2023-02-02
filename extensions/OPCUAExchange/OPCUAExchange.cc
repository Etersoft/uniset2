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
           << " name=" << inf.request.attr;

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


        addr = conf->getArg2Param("--" + prefix + "-addr", it.getProp("addr"), "opc.tcp://localhost:4840/");
        user = conf->getArg2Param("--" + prefix + "-user", it.getProp("user"), "");
        pass = conf->getArg2Param("--" + prefix + "-pass", it.getProp("pass"), "");

        polltime = conf->getArgPInt("--" + prefix + "-polltime", it.getProp("polltime"), polltime);
        vmonit(polltime);

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
        client = make_shared<OPCUAClient>();

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

        ioThread = make_shared< ThreadCreator<OPCUAExchange> >(this, &OPCUAExchange::iothread);

        vmonit(sidHeartBeat);
    }

    // --------------------------------------------------------------------------------

    OPCUAExchange::~OPCUAExchange()
    {
    }

    // --------------------------------------------------------------------------------
    bool OPCUAExchange::tryConnect()
    {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */

        if( !user.empty() )
            return client->connect(addr, user, pass);

        return client->connect(addr);
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::iothread()
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

                return;
            }
        }
        else
        {
            if( !waitSM() ) // необходимо дождаться, чтобы нормально инициализировать итераторы
            {
                if( !cancelled )
                    uterminate();

                return;
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

        opcinfo << myname << "(iothread): iolist size = " << iolist.size() << endl;

        buildRequests();

        bool skip_iout = uniset_conf()->getArgInt("--" + prefix + "-skip-init-output");

        if( !skip_iout && tryConnect() )
            initOutputs();

        shm->initIterator(itHeartBeat);
        PassiveTimer ptAct(activateTimeout);

        while( !activated && !cancelled && !ptAct.checkTime() )
        {
            cout << myname << "(iothread): wait activate..." << endl;
            msleep(300);

            if( activated )
            {
                cout << myname << "(iothread): activate OK.." << endl;
                break;
            }
        }

        if( cancelled )
            return;

        if( !activated )
            opccrit << myname << "(iothread): ************* don`t activate?! ************" << endl;

        opcinfo << myname << "(iothread): run..." << endl;
        Tick tick = 1;

        while( !cancelled )
        {
            try
            {
                if( !tryConnect() )
                {
                    opccrit << myname << "(iothread): no connection to " << addr << endl;
                    msleep(reconnectPause);
                    continue;
                }


                if( tick >= std::numeric_limits<Tick>::max() )
                    tick = 1;

                updateFromSM();
                iopoll(tick++);
                updateToSM();

                if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
                {
                    shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, myid);
                    ptHeartBeat.reset();
                }
            }
            catch( const CORBA::SystemException& ex )
            {
                opclog3 << myname << "(execute): CORBA::SystemException: "
                        << ex.NP_minorString() << endl;
            }
            catch( const uniset::Exception& ex )
            {
                opclog3 << myname << "(execute): " << ex << endl;
            }
            catch(...)
            {
                opclog3 << myname << "(execute): catch ..." << endl;
            }

            if( cancelled )
                break;

            msleep( polltime );
        }

        opcinfo << myname << "(iothread): terminated..." << endl;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::iopoll( Tick tick )
    {
        {
            uniset_rwmutex_rlock lock(wmutex);

            for( auto&& v : writeRequests )
            {
                if( v.first == 0 || tick % v.first == 0 )
                {
                    opclog4 << myname << "(iopool): write[" << (int)v.first << "] " << v.second.size() << " attrs" << endl;

                    if( client->write32(v.second) != UA_STATUSCODE_GOOD )
                        opcwarn << myname << "(iothread): tick=" << (int)tick << " write error" << endl;
                }
            }
        }

        {
            uniset_rwmutex_wrlock lock(rmutex);

            for( auto&& v : readRequests )
            {
                if( v.first == 0 || tick % v.first == 0 )
                {
                    opclog4 << myname << "(iopool): read[" << (int)v.first << "] " << v.second.size() << " attrs" << endl;

                    if( client->read32(v.second) != UA_STATUSCODE_GOOD )
                        opcwarn << myname << "(iothread): tick=" << (int)tick << " read error" << endl;
                }
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateFromSM()
    {
        if( !force_out )
            return;

        uniset_rwmutex_wrlock lock(wmutex);

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
                    io->request.value = IOBase::processingAsAO(ib, shm, force);
                    opclog4 << myname << "(iopoll): write AO"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request.attr
                            << " val=" << io->request.value
                            << endl;
                }
                else if( io->stype == UniversalIO::DO )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    io->request.value = IOBase::processingAsDO(ib, shm, force) ? 1 : 0;
                    opclog4 << myname << "(iopoll): write DO"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request.attr
                            << " val=" << io->request.value
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
    void OPCUAExchange::updateToSM()
    {
        uniset_rwmutex_rlock lock(rmutex);

        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId)
            {
                opclog3 << myname << "(iopoll): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AI )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    opclog3 << myname << "(iopoll): read AI"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request.attr
                            << " val=" << io->request.value
                            << endl;

                    IOBase::processingAsAI(ib, io->request.value, shm, force);
                }
                else if( io->stype == UniversalIO::DI )
                {
                    opclog3 << myname << "(iopoll): read DI"
                            << " sid=" << io->si.id
                            << " attribute=" << io->request.attr
                            << " val=" << io->request.value
                            << endl;
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    IOBase::processingAsDI(ib, io->request.value != 0, shm, force);
                }
            }
            catch( const std::exception& ex )
            {
                opclog3 << myname << "(iopoll): " << ex.what() << endl;
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
                inf->request.attrNum = uni_atoi(attr.substr(2));
                inf->request.attr = attr;
            }
            else if( attr[0] == 's' && attr[1] == '=' )
                inf->request.attr = attr.substr(2); // remove prefix 's='
            else
                inf->request.attr = attr;
        }
        else
        {
            inf->request.attr = attr;
        }

        inf->request.nsIndex = it.getPIntProp(prop_prefix + "opc_ns_index", 0);

        if( inf->request.attr.empty() )
        {
            opcwarn << myname << "(readItem): Unknown OPC UA attribute name. Use opc_attr='nnnn'"
                    << " for " << it.getProp("name") << endl;
            return false;
        }

        if( !IOBase::initItem(inf.get(), it, shm, prop_prefix, false, opclog, myname, filtersize, filterT) )
            return false;

        if( inf->stype == UniversalIO::DO || inf->stype == UniversalIO::DI )
            inf->request.type = UA_TYPES_BOOLEAN;
        else
            inf->request.type = UA_TYPES_INT32;

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

        inf->request.makeNodeId();

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

        if( ioThread && ioThread->isRunning() )
            ioThread->join();

        // выставляем безопасные состояния
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
                    client->set(it->request.attr, set, it->request.nsIndex);
                }
                else if( it->stype == UniversalIO::AO )
                {
                    uniset_rwmutex_wrlock lock(it->val_lock);
                    client->write32(it->request.attr, it->safeval, it->request.nsIndex);
                }
            }
            catch( const std::exception& ex )
            {
                opclog3 << myname << "(sigterm): " << ex.what() << endl;
            }
        }

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
                    client->set(it->request.attr, (bool)it->defval);
                else if( it->stype == UniversalIO::AO )
                    client->write32(it->request.attr, it->defval);
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
        cout << " OPC UA: " << endl;
        cout << "--prefix-addr addr            - OPC UA server address. Default: opc.tcp://localhost:4840/" << endl;
        cout << "--prefix-user user            - OPC UA server auth user. Default: ''(not used)" << endl;
        cout << "--prefix-pass pass            - OPC UA server auth pass. Default: ''(not used)" << endl;
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
        inf << "write requests[" << writeRequests.size() << "]" << endl;

        for( auto it = writeRequests.begin(); it != writeRequests.end(); it++ )
            inf << "  [" << (int)it->first << "]: " << it->second.size() << " attrs" << endl;

        inf << endl;
        inf << "read requests[" << readRequests.size() << "]" << endl;

        for( auto it = readRequests.begin(); it != readRequests.end(); it++ )
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

                if( ioThread && !ioThread->isRunning() )
                    ioThread->start();

                askSensors(UniversalIO::UIONotify);
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
                // (т.е. OPCUAExchange запущен в одном процессе с SharedMemory2)
                // то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
                // при заказе датчиков, а если SM вылетит, то вместе с этим процессом(OPCUAExchange)
                if( shm->isLocalwork() )
                    break;

                askSensors(UniversalIO::UIONotify);
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
                    uniset_rwmutex_wrlock lock(wmutex);
                    it->request.value = sm->value;
                    // IOBase* ib = static_cast<IOBase*>(it.get());
                    // it->request.value = IOBase::processingAsAO(ib, shm, force);
                }
                else if( it->stype == UniversalIO::DO )
                {
                    uniset_rwmutex_wrlock lock(wmutex);
                    it->request.value = sm->value ? 1 : 0;
                    // IOBase* ib = static_cast<IOBase*>(it.get());
                    // it->request.value = IOBase::processingAsDO(ib, shm, force) ? 1 : 0;
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
                auto it = writeRequests.find(io->tick);

                if( it == writeRequests.end() )
                    writeRequests.emplace(io->tick, Request{&io->request});
                else
                    it->second.emplace_back(&io->request);
            }
            else if( io->stype == UniversalIO::AI || io->stype == UniversalIO::DI )
            {
                auto it = readRequests.find(io->tick);

                if( it == readRequests.end() )
                    readRequests.emplace(io->tick, Request{&io->request});
                else
                    it->second.emplace_back(&io->request);
            }
        }
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
