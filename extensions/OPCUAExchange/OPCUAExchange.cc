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
#include <iomanip>
#include <open62541/client_highlevel.h>
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
           << " attr:" << inf.attrName;

        if( inf.cal.minRaw != inf.cal.maxRaw )
            os << inf.cal;

        return os;
    }
    // -----------------------------------------------------------------------------
    OPCUAExchange::OPCUAExchange(uniset::ObjectId id, uniset::ObjectId icID,
                                 const std::shared_ptr<SharedMemory>& ic, const std::string& prefix_ ):
        UniSetObject(id),
        iolist(50),
        prefix(prefix_)
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

        for( size_t i = 0; i < numChannels; i++ )
        {
            channels[i].num = i + 1;
            channels[i].idx = i;
            channels[i].client = make_shared<OPCUAClient>();
            const std::string num = std::to_string(i + 1);
            const std::string defAddr = (i == 0 ? "opc.tcp://localhost:4840" : "");
            channels[i].addr = conf->getArg2Param("--" + prefix + "-addr" + num, it.getProp("addr" + num), defAddr);
            channels[i].user = conf->getArg2Param("--" + prefix + "-user" + num, it.getProp("user" + num), "");
            channels[i].pass = conf->getArg2Param("--" + prefix + "-pass" + num, it.getProp("pass" + num), "");
            channels[i].trStatus.change(true);
        }

        if( channels[0].addr.empty() )
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

        auto timeout = conf->getArgPInt("--" + prefix + "-timeout", it.getProp("timeout"), 5000);

        for( size_t i = 0; i < numChannels; i++ )
            channels[i].ptTimeout.setTiming(timeout);

        reconnectPause = conf->getArgPInt("--" + prefix + "-reconnectPause", it.getProp("reconnectPause"), reconnectPause);
        vmonit(reconnectPause);

        force         = conf->getArgInt("--" + prefix + "-force", it.getProp("force"));
        force_out     = conf->getArgInt("--" + prefix + "-force-out", it.getProp("force_out"));

        if( findArgParam("--" + prefix + "-write-to-all-channels", conf->getArgc(), conf->getArgv()) != -1 )
            writeToAllChannels = true;
        else
            writeToAllChannels = conf->getArgInt(it.getProp("writeToAllChannels"), "0");

        vmonit(force);
        vmonit(force_out);

        filtersize = conf->getArgPInt("--" + prefix + "-filtersize", it.getProp("filtersize"), 1);
        filterT = atof(conf->getArgParam("--" + prefix + "-filterT", it.getProp("filterT")).c_str());

        vmonit(filtersize);
        vmonit(filterT);

        shm = make_shared<SMInterface>(icID, ui, getId(), ic);

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
        {
            force_out = true;
            force = true;
            ic->addReadItem(sigc::mem_fun(this, &OPCUAExchange::readItem));
        }
        else
        {
            readConfiguration();
        }

        ThreadCreator<OPCUAExchange>::Action funcs[] =
        {
            &OPCUAExchange::channel1Thread,
            &OPCUAExchange::channel2Thread
        };

        static_assert(sizeof(funcs) / sizeof(&OPCUAExchange::channel2Thread) == numChannels, "num thread functions != num channels");

        if( sizeof(funcs) < numChannels )
        {
            ostringstream err;
            err << myname << ": Runtime error: thread function[" << sizeof(funcs) << "] < channels[" << numChannels << "]";
            cerr << myname << "(init): " << err.str() << endl;
            throw SystemError(err.str());
        }

        for( size_t i = 0; i < numChannels; i++ )
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
    bool OPCUAExchange::tryConnect( Channel* ch )
    {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */

        if( !ch->user.empty() )
            return ch->client->connect(ch->addr, ch->user, ch->pass);

        return ch->client->connect(ch->addr);
    }
    // --------------------------------------------------------------------------------
    bool OPCUAExchange::prepare()
    {
        UniXML::iterator it(confnode);

        PassiveTimer pt(UniSetTimer::WaitUpTime);

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

        opcinfo << myname << "(prepare): iolist size = " << iolist.size() << endl;

        bool skip_iout = uniset_conf()->getArgInt("--" + prefix + "-skip-init-output");

        if( !skip_iout && tryConnect(&channels[0]) )
            initOutputs();

        shm->initIterator(itHeartBeat);
        PassiveTimer ptAct(activateTimeout);

        if( !activated )
            opccrit << myname << "(prepare): ************* don`t activate?! ************" << endl;

        return activated;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channel1Thread()
    {
        channelThread(&channels[0]);
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channel2Thread()
    {
        channelThread(&channels[1]);
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channelThread( Channel* ch )
    {
        opcinfo << myname << "(channel" << ch->num << "Thread): run..." << endl;
        Tick tick = 1;

        if( ch->addr.empty() )
        {
            opcwarn << myname << "(channel" << ch->num << "Thread): DISABLED (unknown OPC server address)..." << endl;
            return;
        }

        while( !cancelled )
        {
            try
            {
                if( !tryConnect(ch) )
                {
                    opccrit << myname << "(channel" << ch->num << "Thread): no connection to " << ch->addr << endl;
                    ch->status = false;
                    msleep(reconnectPause);
                    continue;
                }

                ch->status = true;

                if( tick >= std::numeric_limits<Tick>::max() )
                    tick = 1;

                updateToChannel(ch);
                channelExchange(tick++, ch, writeToAllChannels || currentChannel == ch->idx);

                if( currentChannel == ch->idx )
                    updateFromChannel(ch);
            }
            catch( const CORBA::SystemException& ex )
            {
                opclog3 << myname << "(channel" << ch->num << "Thread): CORBA::SystemException: "
                        << ex.NP_minorString() << endl;
            }
            catch( const uniset::Exception& ex )
            {
                opclog3 << myname << "(channel" << ch->num << "Thread): " << ex << endl;
            }
            catch(...)
            {
                opclog3 << myname << "(channel" << ch->num << "Thread): catch ..." << endl;
            }

            if( cancelled )
                break;

            msleep( polltime );
        }

        opcinfo << myname << "(channel" << ch->num << "Thread): terminated..." << endl;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::channelExchange( Tick tick, Channel* ch, bool writeOn )
    {
        if( writeOn )
        {
            for (auto&& v : ch->writeValues)
            {
                if (v.first == 0 || tick % v.first == 0)
                {
                    opclog4 << myname << "(channelExchange): channel" << ch->num << " tick " << (int) tick << " write "
                            << v.second->ids.size() << " attrs" << endl;
                    auto ret = ch->client->write32(v.second->ids);

                    if (ret != UA_STATUSCODE_GOOD)
                        opcwarn << myname << "(channelExchange): channel" << ch->num << " tick=" << (int) tick
                                << " write error: " << UA_StatusCode_name(ret) << endl;
                }
            }
        }

        for( auto&& v : ch->readValues )
        {
            if( v.first == 0 || tick % v.first == 0 )
            {
                opclog4 << myname << "(channelExchange): channel" << ch->num << " tick " << (int)tick << " read " << v.second->ids.size() << " attrs" << endl;
                auto ret = ch->client->read32(v.second->ids, v.second->results);

                if( ret != UA_STATUSCODE_GOOD )
                    opcwarn << myname << "(channelExchange): channel" << ch->num << " tick=" << (int)tick << " read error: " << UA_StatusCode_name(ret) << endl;
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
                opclog6 << myname << "(updateFromSM): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AO )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = IOBase::processingAsAO(ib, shm, true);
                    }
                    opclog6 << myname << "(updateFromSM): write AO"
                            << " sid=" << io->si.id
                            << " value=" << io->val
                            << endl;
                }
                else if( io->stype == UniversalIO::DO )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = IOBase::processingAsDO(ib, shm, true) ? 1 : 0;
                    }
                    opclog6 << myname << "(updateFromSM): write DO"
                            << " sid=" << io->si.id
                            << " value=" << io->val
                            << endl;
                }
            }
            catch( const std::exception& ex )
            {
                opclog6 << myname << "(updateFromSM): " << ex.what() << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateToChannel( Channel* ch )
    {
        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId )
            {
                opclog4 << myname << "(updateToChannel" << ch->num << "): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AO )
                {
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        io->wval[ch->idx].set(io->val);
                    }
                    opclog4 << myname << "(updateToChannel" << ch->num << "): write AO"
                            << " sid=" << io->si.id
                            << " attr:" << io->attrName
                            << " val=" << io->val
                            << endl;
                }
                else if( io->stype == UniversalIO::DO )
                {
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        io->wval[ch->idx].set(io->val);
                    }
                    opclog4 << myname << "(updateToChannel" << ch->num << "): write DO"
                            << " sid=" << io->si.id
                            << " attr:" << io->attrName
                            << " val=" << io->val
                            << endl;
                }
            }
            catch( const std::exception& ex )
            {
                opclog4 << myname << "(updateToChannel" << ch->num << "): " << ex.what() << endl;
            }
        }
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateFromChannel( Channel* ch )
    {
        for( auto&& io : iolist )
        {
            if( io->ignore )
                continue;

            if( io->si.id == DefaultObjectId )
            {
                opclog5 << myname << "(updateFromChannel" << ch->num << "): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AI )
                {
                    opclog5 << myname << "(updateFromChannel" << ch->num << "): read AI"
                            << " sid=" << io->si.id
                            << " attr:" << io->attrName
                            << " rval=" << io->rval[ch->idx].get()
                            << endl;
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);

                        if( io->rval[ch->idx].statusOk() )
                            io->val = io->rval[ch->idx].get();
                    }
                }
                else if( io->stype == UniversalIO::DI )
                {
                    opclog5 << myname << "(updateFromChannel" << ch->num << "): read DI"
                            << " sid=" << io->si.id
                            << " attr:" << io->attrName
                            << " rval=" << io->rval[ch->idx].get()
                            << endl;
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);

                        if( io->rval[ch->idx].statusOk() )
                            io->val = io->rval[ch->idx].get() ? 1 : 0;
                    }
                }
            }
            catch( const std::exception& ex )
            {
                opclog5 << myname << "(updateFromChannel" << ch->num << "): " << ex.what() << endl;
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

            if( io->si.id == DefaultObjectId )
            {
                opclog6 << myname << "(writeToSM): sid=DefaultObjectId?!" << endl;
                continue;
            }

            try
            {
                if( io->stype == UniversalIO::AI )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_rlock lock(io->vmut);
                        opclog6 << myname << "(writeToSM): AI"
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
                        opclog6 << myname << "(writeToSM): DI"
                                << " sid=" << io->si.id
                                << " val=" << io->val
                                << endl;
                        IOBase::processingAsDI(ib, io->val != 0, shm, force);
                    }
                }
            }
            catch( const std::exception& ex )
            {
                opclog6 << myname << "(writeToSM): " << ex.what() << endl;
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
    int32_t OPCUAExchange::OPCAttribute::RdValue::get()
    {
        if( !gr )
            return 0;

        return gr->results[grIndex].value;
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::OPCAttribute::RdValue::statusOk()
    {
        return status() == UA_STATUSCODE_GOOD;
    }
    // ------------------------------------------------------------------------------------------
    UA_StatusCode OPCUAExchange::OPCAttribute::RdValue::status()
    {
        if( !gr )
            return UA_STATUSCODE_BAD;

        return gr->results[grIndex].status;
    }
    // ------------------------------------------------------------------------------------------
    void OPCUAExchange::OPCAttribute::WrValue::init( UA_WriteValue* wv, const std::string& nodeId, const std::string& stype, int32_t defvalue )
    {
        UA_WriteValue_init(wv);
        wv->attributeId = UA_ATTRIBUTEID_VALUE;
        wv->value.hasValue = true;
        wv->nodeId = UA_NODEID(nodeId.c_str());
        // wv->value.value.storageType = UA_VARIANT_DATA_NODELETE;

        if( stype == "bool" )
        {
            bool val = defvalue != 0;
            UA_Variant_setScalarCopy(&wv->value.value, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
        }
        else if( stype == "int16" )
        {
            int16_t def = (int16_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_INT16]);
        }
        else if( stype == "uint16" )
        {
            uint16_t def = (uint16_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_UINT16]);
        }
        else  if( stype == "int32" )
        {
            int32_t def = (int32_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_INT32]);
        }
        else if( stype == "uint32" )
        {
            uint32_t def = (uint32_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_UINT32]);
        }
        else if( stype == "byte" )
        {
            uint8_t def = (uint8_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_BYTE]);
        }
        else if( stype == "int64" )
        {
            int64_t def = (int64_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_INT64]);
        }
        else if( stype == "uint64" )
        {
            uint64_t def = (uint64_t)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_UINT64]);
        }
        else
        {
            UA_Variant_setScalarCopy(&wv->value.value, &defvalue, &UA_TYPES[UA_TYPES_INT32]);
        }
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::OPCAttribute::WrValue::set( int32_t val )
    {
        if( !gr )
            return false;

        auto& wv = gr->ids[grIndex];

        if( wv.value.value.type == &UA_TYPES[UA_TYPES_BOOLEAN] )
        {
            bool set = val != 0;
            *(bool*)(wv.value.value.data) = set;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_INT16] )
        {
            *(int16_t*)(wv.value.value.data) = (int16_t)val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_UINT16] )
        {
            *(uint16_t*)(wv.value.value.data) = (uint16_t)val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_INT32] )
        {
            *(int32_t*)(wv.value.value.data) = val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_UINT32] )
        {
            *(uint32_t*)(wv.value.value.data) = (uint32_t)val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_BYTE] )
        {
            *(uint8_t*)(wv.value.value.data) = (uint8_t)val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_UINT64] )
        {
            *(uint64_t*)(wv.value.value.data) = val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_INT64] )
        {
            *(int64_t*)(wv.value.value.data) = val;
        }
        else
        {
            return false;
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::OPCAttribute::WrValue::statusOk()
    {
        return status() == UA_STATUSCODE_GOOD;
    }
    // ------------------------------------------------------------------------------------------
    UA_StatusCode OPCUAExchange::OPCAttribute::WrValue::status()
    {
        if( !gr )
            return UA_STATUSCODE_BAD;

        return gr->ids[grIndex].value.status;
    }
    // ------------------------------------------------------------------------------------------
    static const UA_WriteValue nullWriteValue = UA_WriteValue{};
    const UA_WriteValue& OPCUAExchange::OPCAttribute::WrValue::ref()
    {
        if( !gr )
            return nullWriteValue;

        return gr->ids[grIndex];
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::initIOItem( UniXML::iterator& it )
    {
        auto inf = make_shared<OPCAttribute>();

        string attr = it.getProp(prop_prefix + "opcua_nodeid");

        if( attr.empty() )
        {
            opcwarn << myname << "(readItem): Unknown OPC UA attribute name. Use opcua_nodeid='name'"
                    << " for " << it.getProp("name") << endl;
            return false;
        }

        //        * Examples:
        //        *   UA_NODEID("i=13")
        //        *   UA_NODEID("ns=10;i=1")
        //        *   UA_NODEID("ns=10;s=Hello:World")
        //        *   UA_NODEID("g=09087e75-8e5e-499b-954f-f2a9603db28a")
        //        *   UA_NODEID("ns=1;b=b3BlbjYyNTQxIQ==") // base64

        if( attr.empty() )
        {
            opcwarn << myname << "(readItem): Unknown OPC UA attribute name. Use opcua_nodeid='name'"
                    << " for " << it.getProp("name") << endl;
            return false;
        }

        // s=xxx or ns=xxx
        if( attr[1] != '=' && attr[2] != '=' )
            attr = "s=" + attr;

        if( !IOBase::initItem(inf.get(), it, shm, prop_prefix, false, opclog, myname, filtersize, filterT) )
            return false;

        inf->attrName = attr;

        // если вектор уже заполнен
        // то увеличиваем его на 30 элементов (с запасом)
        // после инициализации делается resize
        // под реальное количество
        if( maxItem >= iolist.size() )
            iolist.resize(maxItem + 30);

        int tick = (uint8_t)IOBase::initIntProp(it, "opcua_tick", prop_prefix, false);
        inf->tick = tick;

        // значит это пороговый датчик..
        if( inf->t_ai != DefaultObjectId )
        {
            opclog3 << myname << "(readItem): add threshold '" << it.getProp("name")
                    << " for '" << uniset_conf()->oind->getNameById(inf->t_ai) << endl;
            std::swap(iolist[maxItem++], inf);
            return true;
        }

        opclog3 << myname << "(readItem): add: " << inf->stype << " " << inf << endl;

        for( size_t chan = 0; chan < numChannels; chan++ )
        {
            if( inf->ignore )
                continue;

            if( inf->stype == UniversalIO::AI || inf->stype == UniversalIO::DI )
            {
                std::shared_ptr<ReadGroup> gr;
                auto rIt = channels[chan].readValues.find(inf->tick);

                if( rIt != channels[chan].readValues.end() )
                    gr = rIt->second;
                else
                {
                    gr = std::make_shared<ReadGroup>();
                    channels[chan].readValues.emplace(inf->tick, gr);
                }

                UA_ReadValueId* rv = &(gr->ids.emplace_back(UA_ReadValueId{}));
                UA_ReadValueId_init(rv);
                rv->attributeId = UA_ATTRIBUTEID_VALUE;
                rv->nodeId = UA_NODEID(attr.c_str());

                OPCUAClient::Result32 res;
                res.status = UA_STATUSCODE_GOOD;
                res.value = inf->defval;
                gr->results.emplace_back(res);
                OPCAttribute::RdValue rd;
                rd.gr = gr;
                rd.grIndex = gr->results.size() - 1;
                inf->rval[chan] = rd;
            }
            else
            {
                std::shared_ptr<WriteGroup> gr;

                auto wIt = channels[chan].writeValues.find(inf->tick);

                if( wIt != channels[chan].writeValues.end() )
                    gr = wIt->second;
                else
                {
                    gr = std::make_shared<WriteGroup>();
                    channels[chan].writeValues.emplace(inf->tick, gr);
                }

                string defVType = "int32";

                if( inf->stype == UniversalIO::DO )
                    defVType = "bool";

                const string vtype = it.getProp2("opcua_type", defVType);
                UA_WriteValue* wv = &(gr->ids.emplace_back(UA_WriteValue{}));
                OPCAttribute::WrValue::init(wv, attr, vtype, inf->defval);

                OPCAttribute::WrValue wr;
                wr.gr = gr;
                wr.grIndex = gr->ids.size() - 1;
                inf->wval[chan] = wr;
            }
        }

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

        for( size_t i = 0; i < numChannels; i++ )
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

                    for( size_t i = 0; i < numChannels; i++)
                    {
                        if( !addr[i].empty() )
                            client[i]->set(it->request[i].attr, set, it->request[i].nsIndex);
                    }
                }
                else if( it->stype == UniversalIO::AO )
                {
                    uniset_rwmutex_wrlock lock(it->val_lock);

                    for( size_t i = 0; i < numChannels; i++)
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
                if( it->stype == UniversalIO::DO || it->stype == UniversalIO::AO )
                {
                    for( size_t i = 0; i < numChannels; i++ )
                    {
                        if( !channels[i].addr.empty() )
                            channels[i].client->write(it->wval[i].ref());
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
            cerr << "(opcua-exchange): Unknown name. Use --" << prefix << "-name " << endl;
            return nullptr;
        }

        ObjectId ID = conf->getObjectID(name);

        if( ID == uniset::DefaultObjectId )
        {
            cerr << "(opcua-exchange): Unknown ID for " << name
                 << "' Not found in <objects>" << endl;
            return nullptr;
        }

        dinfo << "(opcua-exchange): name = " << name << "(" << ID << ")" << endl;
        return make_shared<OPCUAExchange>(ID, icID, ic, prefix);
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::help_print( int argc, const char* const* argv )
    {
        cout << "--opcua-confnode name     - Использовать для настройки указанный xml-узел" << endl;
        cout << "--opcua-name name         - ID процесса. По умолчанию OPCUAExchange1." << endl;
        cout << "--opcua-polltime msec     - Пауза между опросом карт. По умолчанию 150 мсек." << endl;
        cout << "--opcua-updatetime msec   - Период обновления данных в/из SM. По умолчанию 150 мсек." << endl;
        cout << "--opcua-filtersize val    - Размерность фильтра для аналоговых входов." << endl;
        cout << "--opcua-filterT val       - Постоянная:: времени фильтра." << endl;
        cout << "--opcua-s-filter-field    - Идентификатор в configure.xml по которому считывается список относящихся к это процессу датчиков" << endl;
        cout << "--opcua-s-filter-value    - Значение идентификатора по которому считывается список относящихся к это процессу датчиков" << endl;
        cout << "--opcua-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-датчиком." << endl;
        cout << "--opcua-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
        cout << "--opcua-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
        cout << "--opcua-force             - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
        cout << "--opcua-force-out         - Обновлять выходы принудительно (не по заказу)" << endl;
        cout << "--opcua-write-to-all-channels - Всегда писать(write) по всем каналам обмена. По умолчанию только в активном" << endl;

        cout << "--opcua-skip-init-output  - Не инициализировать 'выходы' при старте" << endl;
        cout << "--opcua-sm-ready-test-sid - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;
        cout << endl;
        cout << " OPC UA: N=[1,2]" << endl;
        cout << "--opcua-addrN addr        - OPC UA server N address (channelN). Default: opc.tcp://localhost:4840/" << endl;
        cout << "--opcua-userN user        - OPC UA server N auth user (channelN). Default: ''(not used)" << endl;
        cout << "--opcua-passM pass        - OPC UA server N auth pass (channelN). Default: ''(not used)" << endl;
        cout << "--opcua-reconnectPause msec  - Pause between reconnect to server, milliseconds" << endl;
        cout << " Logs: " << endl;
        cout << endl;
        cout << "--opcua-log-...            - log control" << endl;
        cout << "             add-levels ..." << endl;
        cout << "             del-levels ..." << endl;
        cout << "             set-levels ..." << endl;
        cout << "             logfile filaname" << endl;
        cout << "             no-debug " << endl;
        cout << " LogServer: " << endl;
        cout << "--opcua-run-logserver       - run logserver. Default: localhost:id" << endl;
        cout << "--opcua-logserver-host ip   - listen ip. Default: localhost" << endl;
        cout << "--opcua-logserver-port num  - listen port. Default: ID" << endl;
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

        for( size_t i = 0; i < numChannels; i++ )
        {
            if( channels[i].addr.empty() )
                inf << "channel" << i + 1 << ": [DISABLED]" << endl;
            else
                inf << "channel" << i + 1 << ": " << (channels[i].status ? "[OK]" : "[NO CONNECTION]")
                    << " addr: " << channels[i].addr << endl;
        }

        inf << endl;
        inf << "iolist: " << iolist.size() << endl;

        for( const auto& v : channels[0].writeValues )
            inf << "write attributes[tick " << setw(2) << (int) v.first << "]: " << v.second->ids.size() << endl;

        for( const auto& v : channels[0].readValues )
            inf << " read attributes[tick " << setw(2) << (int) v.first << "]: " << v.second->ids.size() << endl;

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

                for( size_t i = 0; i < numChannels; i++ )
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
                    shm->askSensor(it->si.id, cmd, getId());
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

            // check status for active channel
            auto* ch = &channels[currentChannel];

            if( ch->trStatus.change(ch->status) )
            {
                if( !ch->trStatus.get() )
                {
                    opclog7 << myname << "(timerInfo): channel" << ch->num << " lost connection.." << endl;
                    ch->ptTimeout.reset();
                }
                else
                    opcwarn << myname << "(timerInfo): channel" << ch->num << " [ACTIVE]" << endl;
            }

            if(!ch->status && ch->ptTimeout.checkTime() )
            {
                bool foundOk = false;

                for( size_t i = 0; i < numChannels; i++ )
                {
                    opclog7 << myname << "(timerInfo): channel" << i + 1 << (channels[i].status ? " [OK]" : " [NO CONNECTION]" ) << endl;

                    if( channels[i].status )
                    {
                        opcwarn << myname << "(timerInfo): change ACTIVE channel to channel" << i + 1 << endl;
                        currentChannel = i;
                        channels[i].ptTimeout.reset();
                        channels[i].trStatus.hi(true);
                        foundOk = true;
                        break;
                    }
                }

                if( noConnections.low(foundOk) )
                    opccrit << myname << "(timerInfo): did not find any working channel!" << endl;
            }

            if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
            {
                shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
                ptHeartBeat.reset();
            }
        }
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::sensorInfo( const uniset::SensorMessage* sm )
    {
        opclog6 << myname << "(sensorInfo): sm->id=" << sm->id
                << " val=" << sm->value << endl;

        if( force_out )
            return;

        for( auto&& it : iolist )
        {
            if( it->si.id == sm->id )
            {
                if( it->stype == UniversalIO::AO )
                {
                    IOBase* ib = static_cast<IOBase*>(it.get());
                    ib->value = sm->value;
                    {
                        uniset::uniset_rwmutex_wrlock lock(it->vmut);
                        it->val = IOBase::processingAsAO(ib, shm, force);
                    }
                    opclog6 << myname << "(sensorInfo): sid=" << sm->id
                            << " update val=" << it->val
                            << endl;
                }
                else if( it->stype == UniversalIO::DO )
                {
                    IOBase* ib = static_cast<IOBase*>(it.get());
                    ib->value = sm->value;
                    {
                        uniset::uniset_rwmutex_wrlock lock(it->vmut);
                        it->val = IOBase::processingAsDO(ib, shm, force) ? 1 : 0;
                    }
                    opclog6 << myname << "(sensorInfo): sid=" << sm->id
                            << " update val=" << it->val
                            << endl;
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
} // end of namespace uniset
