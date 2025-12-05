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
    float OPCUAExchange::OPCAttribute::as_float()
    {
        return ( (float)val / pow(10.0, cal.precision) );
    }
    // -----------------------------------------------------------------------------
    int32_t OPCUAExchange::OPCAttribute::set( float fval )
    {
        val = lroundf( fval * pow(10.0, cal.precision) );
        return val;
    }
    // -----------------------------------------------------------------------------
    OPCUAExchange::OPCUAExchange(uniset::ObjectId id, xmlNode* cnode, uniset::ObjectId icID,
                                 const std::shared_ptr<SharedMemory>& ic, const std::string& _prefix ):
        USingleProcess(cnode, uniset_conf()->getArgc(), uniset_conf()->getArgv(), ""),
        UniSetObject(id),
        confnode(cnode),
        iolist(50),
        argprefix( (_prefix.empty() ? myname + "-" : _prefix + "-") )
    {
        auto conf = uniset_conf();

        prop_prefix = "";
        vmonit(argprefix);

        opclog = make_shared<DebugStream>();
        opclog->setLogName(myname);
        conf->initLogStream(opclog, argprefix + "log");

        loga = make_shared<LogAgregator>(myname + "-loga");
        loga->add(opclog);
        loga->add(ulog());
        loga->add(dlog());

        UniXML::iterator it(confnode);

        logserv = make_shared<LogServer>(loga);
        logserv->init( argprefix + "logserver", confnode );

        if( findArgParam("--" + argprefix + "run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
        {
            logserv_host = conf->getArg2Param("--" + argprefix + "logserver-host", it.getProp("logserverHost"), "localhost");
            logserv_port = conf->getArgPInt("--" + argprefix + "logserver-port", it.getProp("logserverPort"), getId());
        }

        httpEnabledSetParams =  conf->getArgPInt("--" + argprefix + "-http-enabled-setparams", it.getProp("httpEnabledSetParams"), 0);

        if( ic )
            ic->logAgregator()->add(loga);

        if( findArgParam("--" + argprefix + "enable-subscription", conf->getArgc(), conf->getArgv()) != -1 )
            enableSubscription = true;
        else
            enableSubscription = conf->getArgInt(it.getProp("enableSubscription"), "0");

        publishingInterval = atof(conf->getArgParam("--" + argprefix + "publishing-interval", it.getProp("publishingInterval")).c_str());
        samplingInterval = atof(conf->getArgParam("--" + argprefix + "sampling-interval", it.getProp("samplingInterval")).c_str());
        timeoutIterate = uniset_conf()->getArgPInt("--" + argprefix + "timeout-iterate", it.getProp("timeoutIterate"), 0);
        vmonit(enableSubscription);
        vmonit(publishingInterval);
        vmonit(samplingInterval);
        vmonit(timeoutIterate);

        stopOnError = uniset_conf()->getArgPInt("--" + argprefix + "stop-on-error", it.getProp("stopOnError"), 0);
        vmonit(stopOnError);

        for( size_t i = 0; i < numChannels; i++ )
        {
            channels[i].num = i + 1;
            channels[i].idx = i;
            channels[i].client = make_shared<OPCUAClient>();
            const std::string num = std::to_string(i + 1);
            const std::string defAddr = (i == 0 ? "opc.tcp://localhost:4840" : "");
            channels[i].addr = conf->getArg2Param("--" + argprefix + "addr" + num, it.getProp("addr" + num), defAddr);
            channels[i].user = conf->getArg2Param("--" + argprefix + "user" + num, it.getProp("user" + num), "");
            channels[i].pass = conf->getArg2Param("--" + argprefix + "pass" + num, it.getProp("pass" + num), "");
            channels[i].trStatus.change(true);

            string tmp = conf->getArg2Param("--" + argprefix + "respond" + num, it.getProp("respond" + num + "_s"), "");

            if( !tmp.empty() )
            {
                channels[i].respond_s = conf->getSensorID(tmp);

                if( channels[i].respond_s == DefaultObjectId )
                {
                    ostringstream err;
                    err << myname << "(init): Not found respond sensor " << tmp;
                    opccrit << myname << "(init): " << err.str() << endl;
                    throw SystemError(err.str());
                }
            }

            //Подписка для opcua по флагу
            if(enableSubscription)
            {
                opclog3 << myname << " Create subscription for channel " << i + 1 << endl;
                createSubscription(i);
            }
        }

        if( channels[0].addr.empty() )
        {
            ostringstream err;
            err << myname << "(init): Unknown OPC server address for channel N1";
            opccrit << myname << "(init): " << err.str() << endl;
            throw SystemError(err.str());
        }

        string tmp = conf->getArg2Param("--" + argprefix + "respond", it.getProp("respond_s"), "");

        if( !tmp.empty() )
        {
            sidRespond = conf->getSensorID(tmp);

            if(sidRespond == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): Not found respond sensor " << sidRespond;
                opccrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }
        }

        polltime = conf->getArgPInt("--" + argprefix + "polltime", it.getProp("polltime"), polltime);
        vmonit(polltime);

        updatetime = conf->getArgPInt("--" + argprefix + "updatetime", it.getProp("updateTime"), updatetime);
        vmonit(updatetime);

        auto timeout = conf->getArgPInt("--" + argprefix + "timeout", it.getProp("timeout"), 5000);

        for( size_t i = 0; i < numChannels; i++ )
            channels[i].ptTimeout.setTiming(timeout);

        reconnectPause = conf->getArgPInt("--" + argprefix + "reconnectPause", it.getProp("reconnectPause"), reconnectPause);
        vmonit(reconnectPause);

        force         = conf->getArgInt("--" + argprefix + "force", it.getProp("force"));
        force_out     = conf->getArgInt("--" + argprefix + "force-out", it.getProp("forceOut"));

        if( findArgParam("--" + argprefix + "write-to-all-channels", conf->getArgc(), conf->getArgv()) != -1 )
            writeToAllChannels = true;
        else
            writeToAllChannels = conf->getArgInt(it.getProp("writeToAllChannels"), "0");

        vmonit(force);
        vmonit(force_out);

        filtersize = conf->getArgPInt("--" + argprefix + "filtersize", it.getProp("filterSize"), 1);
        filterT = atof(conf->getArgParam("--" + argprefix + "filterT", it.getProp("filterT")).c_str());

        vmonit(filtersize);
        vmonit(filterT);

        shm = make_shared<SMInterface>(icID, ui, getId(), ic);

        // определяем фильтр
        s_field = conf->getArg2Param("--" + argprefix + "filter-field", it.getProp("filterField"));
        s_fvalue = conf->getArg2Param("--" + argprefix + "filter-value", it.getProp("filterValue"));
        auto regexp_fvalue = conf->getArg2Param("--" + argprefix + "filter-value-re", it.getProp("filterValueRE"));

        if( !regexp_fvalue.empty() )
        {
            try
            {
                s_fvalue = regexp_fvalue;
                s_fvalue_re = std::regex(regexp_fvalue);
            }
            catch (const std::regex_error& e)
            {
                ostringstream err;
                err << myname << "(init): '--" + argprefix + "filter-value-re' regular expression error: " << e.what();
                throw uniset::SystemError(err.str());
            }
        }

        vmonit(s_field);
        vmonit(s_fvalue);

        opcinfo << myname << "(init): read s_field='" << s_field
                << "' s_fvalue='" << s_fvalue << "'" << endl;

        int sm_tout = conf->getArgInt("--" + argprefix + "sm-ready-timeout", it.getProp("ready_timeout"));

        if( sm_tout == 0 )
            smReadyTimeout = conf->getNCReadyTimeout();
        else if( sm_tout < 0 )
            smReadyTimeout = UniSetTimer::WaitUpTime;
        else
            smReadyTimeout = sm_tout;

        vmonit(smReadyTimeout);

        string sm_ready_sid = conf->getArgParam("--" + argprefix + "sm-test-sid", it.getProp2("smTestSID", "TestMode_S"));
        sidTestSMReady = conf->getSensorID(sm_ready_sid);

        if( sidTestSMReady == DefaultObjectId )
        {
            opcwarn << myname
                    << "(init): Unknown ID for sm-test-sid (--" << argprefix << "sm-test-id)..." << endl;
        }
        else
            opcinfo << myname << "(init): sm-test-sid: " << sm_ready_sid << endl;

        vmonit(sidTestSMReady);

        // -----------------------
        string emode = conf->getArgParam("--" + argprefix + "exchange-mode-id", it.getProp("exchangeModeID"));

        if( !emode.empty() )
        {
            sidExchangeMode = conf->getSensorID(emode);

            if( sidExchangeMode == DefaultObjectId )
            {
                ostringstream err;
                err << myname << ": ID not found ('exchangeModeID') for " << emode;
                opccrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }

            // Режим обмена по-умолчанию при старте процесса
            auto default_emode = conf->getArgInt("--" + argprefix + "default-exchange-mode", it.getProp("defaultExchangeMode"));

            if( default_emode > 0 && default_emode < emLastNumber )
                exchangeMode = default_emode;
        }

        // -----------------------
        string heart = conf->getArgParam("--" + argprefix + "heartbeat-id", it.getProp("heartbeat_id"));

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

            maxHeartBeat = conf->getArgPInt("--" + argprefix + "heartbeat-max", it.getProp("heartbeat_max"), 10);
        }

        activateTimeout    = conf->getArgPInt("--" + argprefix + "activate-timeout", 25000);

        vmonit(activateTimeout);

        maxReadItems = uniset_conf()->getArgPInt("--" + argprefix + "maxNodesPerRead", it.getProp("maxNodesPerRead"), 0);
        maxWriteItems = uniset_conf()->getArgPInt("--" + argprefix + "maxNodesPerWrite", it.getProp("maxNodesPerWrite"), 0);

        vmonit(maxReadItems);
        vmonit(maxWriteItems);

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

        // делаем очередь большой по умолчанию
        if( shm->isLocalwork() )
        {
            int sz = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 10000);

            if( sz > 0 )
                setMaxSizeOfMessageQueue(sz);
        }

        vmonit(connectCount);
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

        for( size_t c = 0; c < numChannels; c++ )
            shm->initIterator(channels[c].respond_it);

        readconf_ok = true; // т.к. waitSM() уже был...

        opcinfo << myname << "(prepare): iolist size = " << iolist.size() << endl;

        bool skip_iout = uniset_conf()->getArgInt("--" + argprefix + "skip-init-output");

        if( !skip_iout && tryConnect(&channels[0]) )
            initOutputs();

        shm->initIterator(itHeartBeat);
        shm->initIterator(itRespond);
        shm->initIterator(itExchangeMode);
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

                if(enableSubscription)
                {
                    try
                    {
                        ch->client->rethrowException();
                    }
                    catch(const std::exception& ex)
                    {
                        if( (!subscription_ok && stopOnError == smFirstOnly && connectCount == 0) ||
                                stopOnError == smAny ) // При неудачной подписке или по флагу остановки при любой ошибке.
                        {
                            opccrit << myname << "(channel" << ch->num << "Thread): " << ex.what() << ". Terminate!" << endl;
                            uterminate();
                            break;
                        }
                        else
                            throw;
                    }
                }

                auto t_start = std::chrono::steady_clock::now();

                ch->status = true;

                if( tick >= std::numeric_limits<Tick>::max() )
                    tick = 1;

                updateToChannel(ch);
                channelExchange(tick++, ch, writeToAllChannels || currentChannel == ch->idx);

                if( currentChannel == ch->idx )
                {
                    updateFromChannel(ch);

                    // Обработка порогов, после обработки всех
                    for( auto&& i : thrlist )
                    {
                        try
                        {
                            IOBase::processingThreshold(&i, shm, force);
                        }
                        catch( std::exception& ex )
                        {
                            opcwarn << myname << "(channel" << ch->num << "Thread): " << ex.what() << endl;
                        }
                    }
                }

                auto t_end = std::chrono::steady_clock::now();
                opclog8 << myname << "(channelThread): " << setw(10) << setprecision(7) << std::fixed
                        << std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count() << " sec" << endl;
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
            catch( const std::exception& ex )
            {
                opclog3 << myname << "(channel" << ch->num << "Thread): " << ex.what() << endl;
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
        if( exchangeMode == emSkipExchange )
            return;

        auto t_start = std::chrono::steady_clock::now();

        if( writeOn && exchangeMode != emReadOnly )
        {
            for( auto&& v : ch->writeValues )
            {
                if (v.first == 0 || tick % v.first == 0)
                {
                    for(auto& it : v.second->ids)
                    {
                        opclog4 << myname << "(channelExchange): channel" << ch->num << " tick " << (int) tick << " write "
                                << it.size() << " attrs" << endl;

                        auto ret = ch->client->write32(it);

                        if( ret != UA_STATUSCODE_GOOD )
                            opcwarn << myname << "(channelExchange): channel" << ch->num << " tick=" << (int) tick
                                    << " write error: " << UA_StatusCode_name(ret) << endl;

                        if( ret == UA_STATUSCODE_BADSESSIONIDINVALID || ret == UA_STATUSCODE_BADSESSIONCLOSED )
                        {
                            ch->client->disconnect();
                            return;
                        }
                    }
                }
            }
        }

        auto t_end = std::chrono::steady_clock::now();
        opclog8 << myname << "(channelExchange): write time " << setw(10) << setprecision(7) << std::fixed
                << std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count() << " sec" << endl;
        t_start = std::chrono::steady_clock::now();

        if( exchangeMode != emWriteOnly )
        {
            if(enableSubscription)
            {
                try
                {
                    if(subscription_ok)
                        ch->client->runIterate(timeoutIterate);
                }
                catch(const std::exception& ex)
                {
                    opcwarn << myname << "(channel" << ch->num << "Thread): Error iteration - " << ex.what() << endl;

                    if( stopOnError == smAny )
                    {
                        opccrit << myname << "(channel" << ch->num << "Thread): terminate!" << endl;
                        uterminate();
                        return;
                    }
                }
            }
            else
            {
                for( auto&& v : ch->readValues )
                {
                    if (v.first == 0 || tick % v.first == 0)
                    {
                        std::vector<std::vector<OPCUAClient::ResultVar>>::iterator rit = v.second->results.begin();

                        for(auto& it : v.second->ids)
                        {
                            opclog4 << myname << "(channelExchange): channel" << ch->num << " tick " << (int) tick << " read "
                                    << it.size() << " attrs" << endl;

                            auto ret = ch->client->read(it, *rit++);

                            if( ret != UA_STATUSCODE_GOOD )
                                opcwarn << myname << "(channelExchange): channel" << ch->num << " tick=" << (int) tick
                                        << " read error: " << UA_StatusCode_name(ret) << endl;

                            if( ret == UA_STATUSCODE_BADSESSIONIDINVALID || ret == UA_STATUSCODE_BADSESSIONCLOSED )
                            {
                                ch->client->disconnect();
                                return;
                            }
                        }
                    }
                }
            }
        }

        t_end = std::chrono::steady_clock::now();
        opclog8 << myname << "(channelExchange): read time " << setw(10) << setprecision(7) << std::fixed
                << std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count() << " sec" << endl;
    }
    // --------------------------------------------------------------------------------
    void OPCUAExchange::updateFromSM()
    {
        if( shm->isLocalwork() )
            return;

        if( !force_out )
            return;

        if( sidExchangeMode != DefaultObjectId )
            exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);

        if( !isUpdateSM(true) )
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
                        io->val = forceSetBits(io->val, IOBase::processingAsAO(ib, shm, true), io->mask, io->offset);
                    }
                    opclog6 << myname << "(updateFromSM): write AO"
                            << " sid=" << io->si.id
                            << " value=" << io->val
                            << " mask=" << io->mask
                            << endl;
                }
                else if( io->stype == UniversalIO::DO )
                {
                    IOBase* ib = static_cast<IOBase*>(io.get());
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);
                        io->val = forceSetBits(io->val, IOBase::processingAsDO(ib, shm, true) ? 1 : 0, io->mask, io->offset);
                    }
                    opclog6 << myname << "(updateFromSM): write DO"
                            << " sid=" << io->si.id
                            << " value=" << io->val
                            << " mask=" << io->mask
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
        if( exchangeMode == emSkipExchange || exchangeMode == emReadOnly )
            return;

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

                        if( io->vtype == OPCUAClient::VarType::Float )
                            io->wval[ch->idx].setF(io->as_float());
                        else
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
        if( exchangeMode == emSkipExchange || exchangeMode == emWriteOnly )
            return;

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
                            << " mask=" << io->mask
                            << endl;
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);

                        if( io->rval[ch->idx].statusOk() )
                        {
                            if( io->vtype == OPCUAClient::VarType::Float )
                                io->set( io->rval[ch->idx].getF() );
                            else
                                io->val = getBits(io->rval[ch->idx].get(), io->mask, io->offset);
                        }
                    }
                }
                else if( io->stype == UniversalIO::DI )
                {
                    opclog5 << myname << "(updateFromChannel" << ch->num << "): read DI"
                            << " sid=" << io->si.id
                            << " attr:" << io->attrName
                            << " rval=" << io->rval[ch->idx].get()
                            << " mask=" << io->mask
                            << endl;
                    {
                        uniset::uniset_rwmutex_wrlock lock(io->vmut);

                        if( io->rval[ch->idx].statusOk() )
                            io->val = getBits(io->rval[ch->idx].get() ? 1 : 0, io->mask, io->offset);
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
        if( !isUpdateSM(false) )
            return;

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
    bool OPCUAExchange::isUpdateSM( bool wrFunc ) const noexcept
    {
        if( exchangeMode == emSkipExchange )
        {
            if( wrFunc )
                return true; // данные для посылки, должны обновляться всегда (чтобы быть актуальными, когда режим переключиться обратно..)

            opclog3 << "(isUpdateSM):"
                    << " skip... mode='emSkipExchange' " << endl;
            return false;
        }

        if( wrFunc && (exchangeMode == emReadOnly) )
        {
            opclog3 << "(isUpdateSM):"
                    << " skip... mode='emReadOnly' " << endl;
            return false;
        }

        if( !wrFunc && (exchangeMode == emWriteOnly) )
        {
            opclog3 << "(isUpdateSM):"
                    << " skip... mode='emWriteOnly' " << endl;
            return false;
        }

        if( !wrFunc && (exchangeMode == emSkipSaveToSM) )
        {
            opclog3 << "(isUpdateSM):"
                    << " skip... mode='emSkipSaveToSM' " << endl;
            return false;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
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
            if( s_fvalue_re.has_value() )
            {
                if( uniset::check_filter_re(it, s_field, *s_fvalue_re) )
                    initIOItem(it);
            }
            else if( uniset::check_filter(it, s_field, s_fvalue) )
                initIOItem(it);
        }

        readconf_ok = true;
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
    {
        if( s_fvalue_re.has_value() )
        {
            if( uniset::check_filter_re(it, s_field, *s_fvalue_re) )
                initIOItem(it);
        }
        else if( uniset::check_filter(it, s_field, s_fvalue) )
            initIOItem(it);

        return true;
    }
    // ------------------------------------------------------------------------------------------
    int32_t OPCUAExchange::OPCAttribute::RdValue::get()
    {
        if( !gr )
            return 0;

        if( !statusOk() || !gr->results[grNumber][grIndex].statusOk() )
            return 0;

        return gr->results[grNumber][grIndex].get();
    }
    // ------------------------------------------------------------------------------------------
    float OPCUAExchange::OPCAttribute::RdValue::getF()
    {
        if( !gr )
            return 0;

        if( !statusOk() || !gr->results[grNumber][grIndex].statusOk() )
            return 0;

        if( gr->results[grNumber][grIndex].type == OPCUAClient::VarType::Float )
            return gr->results[grNumber][grIndex].as<float>();

        return gr->results[grNumber][grIndex].get();
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

        return gr->results[grNumber][grIndex].status;
    }
    // ------------------------------------------------------------------------------------------
    uint8_t OPCUAExchange::firstBit( uint32_t mask )
    {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ctz(mask);
#else
        uint8_t n = 0;

        while( mask != 0 )
        {
            if( mask & 1 )
                break;

            mask = (mask >> 1);
            n++;
        }

        return n;
#endif
    }
    // ------------------------------------------------------------------------------------------
    uint32_t OPCUAExchange::forceSetBits( uint32_t value, uint32_t set, uint32_t mask, uint8_t offset )
    {
        if( mask == 0 )
            return set;

        return OPCUAExchange::setBits(value, set, mask, offset);
    }
    // ------------------------------------------------------------------------------------------
    uint32_t OPCUAExchange::setBits( uint32_t value, uint32_t set, uint32_t mask, uint8_t offset )
    {
        if( mask == 0 )
            return value;

        return (value & (~mask)) | ((set << offset) & mask);
    }
    // ------------------------------------------------------------------------------------------
    uint32_t OPCUAExchange::getBits( uint32_t value, uint32_t mask, uint8_t offset )
    {
        if( mask == 0 )
            return value;

        return (value & mask) >> offset;
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
        else if( stype == "float" )
        {
            float def = (float)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_FLOAT]);
        }
        else if( stype == "double" )
        {
            double def = (double)defvalue;
            UA_Variant_setScalarCopy(&wv->value.value, &def, &UA_TYPES[UA_TYPES_DOUBLE]);
        }
        else
        {
            UA_Variant_setScalarCopy(&wv->value.value, &defvalue, &UA_TYPES[UA_TYPES_INT32]);
        }
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::OPCAttribute::WrValue::setF( float val )
    {
        if( !gr )
            return false;

        auto& wv = gr->ids[grNumber][grIndex];

        if( wv.value.value.type == &UA_TYPES[UA_TYPES_FLOAT] )
        {
            *(float*)(wv.value.value.data) = val;
            return true;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_DOUBLE] )
        {
            *(double*)(wv.value.value.data) = val;
            return true;
        }

        return set(val);
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::OPCAttribute::WrValue::set( int32_t val )
    {
        if( !gr )
            return false;

        auto& wv = gr->ids[grNumber][grIndex];

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
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_FLOAT] )
        {
            *(float*)(wv.value.value.data) = val;
        }
        else if( wv.value.value.type == &UA_TYPES[UA_TYPES_DOUBLE] )
        {
            *(double*)(wv.value.value.data) = val;
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

        return gr->ids[grNumber][grIndex].value.status;
    }
    // ------------------------------------------------------------------------------------------
    static const UA_WriteValue nullWriteValue = UA_WriteValue{};
    const UA_WriteValue& OPCUAExchange::OPCAttribute::WrValue::ref()
    {
        if( !gr )
            return nullWriteValue;

        return gr->ids[grNumber][grIndex];
    }
    // ------------------------------------------------------------------------------------------
    static const UA_ReadValueId nullReadValueId = UA_ReadValueId{};
    const UA_ReadValueId& OPCUAExchange::OPCAttribute::RdValue::ref()
    {
        if( !gr )
            return nullReadValueId;

        return gr->ids[grNumber][grIndex];
    }
    // ------------------------------------------------------------------------------------------
    bool OPCUAExchange::initIOItem( UniXML::iterator& it )
    {
        auto inf = make_shared<OPCAttribute>();

        if( !IOBase::initItem(inf.get(), it, shm, prop_prefix, false, opclog, myname, filtersize, filterT) )
            return false;

        // значит это пороговый датчик..
        if( inf->t_ai != DefaultObjectId )
        {
            opclog3 << myname << "(readItem): add threshold '" << it.getProp("name")
                    << " for '" << uniset_conf()->oind->getNameById(inf->t_ai) << endl;
            thrlist.emplace_back( std::move(*inf) );
            return true;
        }

        string attr = it.getProp(prop_prefix + "opcua_nodeid");

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

        inf->attrName = attr;

        // если вектор уже заполнен
        // то увеличиваем его на 30 элементов (с запасом)
        // после инициализации делается resize
        // под реальное количество
        if( maxItem >= iolist.size() )
            iolist.resize(maxItem + 30);

        int tick = (uint8_t)IOBase::initIntProp(it, "opcua_tick", prop_prefix, false);
        inf->tick = tick;

        std::string smask = IOBase::initProp(it, "opcua_mask", prop_prefix, false);

        if( !smask.empty() )
        {
            inf->mask = uni_atoi(smask);
            inf->offset = firstBit(inf->mask);
        }

        auto vtype = IOBase::initProp(it, "opcua_type", prop_prefix, false, "");

        if( vtype.empty() )
        {
            vtype = "int32";

            if( inf->stype == UniversalIO::DO || inf->stype == UniversalIO::DI )
                vtype = "bool";
        }

        inf->vtype = OPCUAClient::str2vtype(vtype);

        opclog3 << myname << "(readItem): add: " << inf->stype << " " << inf << " vtype=" << vtype << endl;

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
                    gr->ids.emplace_back(std::vector<UA_ReadValueId>());
                    gr->results.emplace_back(std::vector<OPCUAClient::ResultVar>());
                }

                UA_ReadValueId* rv;

                // Добавление нового регистра в зависимости от ограничений на чтение.
                if(maxReadItems)
                {
                    if(gr->ids.back().size() >= maxReadItems)
                    {
                        gr->ids.emplace_back(std::vector<UA_ReadValueId>());
                        gr->results.emplace_back(std::vector<OPCUAClient::ResultVar>());
                    }
                }

                rv = &(gr->ids.back().emplace_back(UA_ReadValueId{}));
                UA_ReadValueId_init(rv);
                rv->attributeId = UA_ATTRIBUTEID_VALUE;
                rv->nodeId = UA_NODEID(attr.c_str());

                OPCUAClient::ResultVar res;
                res.status = UA_STATUSCODE_GOOD;
                res.type = inf->vtype;

                if( inf->vtype == OPCUAClient::VarType::Float )
                {
                    // отключаем обработку "precision" на уровне IOBase
                    // т.к. учитываем precision сами
                    inf->noprecision = true;
                    inf->val = inf->defval;
                    res.value = inf->as_float();
                }
                else
                {
                    res.value = (int32_t)forceSetBits(0, inf->defval, inf->mask, inf->offset);
                }

                gr->results.back().emplace_back(res);
                OPCAttribute::RdValue rd;
                rd.gr = gr;
                rd.grIndex = gr->results.back().size() - 1;
                rd.grNumber = gr->results.size() - 1;
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
                    gr->ids.emplace_back(std::vector<UA_WriteValue>());
                }

                UA_WriteValue* wv;

                // Добавление нового регистра в зависимости от ограничений на запись.
                if(maxWriteItems)
                {
                    if(gr->ids.back().size() >= maxWriteItems)
                    {
                        gr->ids.emplace_back(std::vector<UA_WriteValue>());
                    }
                }

                wv = &(gr->ids.back().emplace_back(UA_WriteValue{}));

                OPCAttribute::WrValue::init(wv, attr, vtype, forceSetBits(0, inf->defval, inf->mask, inf->offset));

                OPCAttribute::WrValue wr;
                wr.gr = gr;
                wr.grIndex = gr->ids.back().size() - 1;
                wr.grNumber = gr->ids.size() - 1;

                if( inf->vtype == OPCUAClient::VarType::Float )
                {
                    // отключаем обработку "precision" на уровне IOBase
                    // т.к. учитываем precision сами
                    inf->noprecision = true;
                    inf->val = inf->defval;
                    wr.setF(inf->as_float());
                }

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
                        {
                            auto errcode = channels[i].client->write(it->wval[i].ref());

                            if( errcode != UA_STATUSCODE_GOOD )
                                opcwarn << myname << "(initOutput): channel" << i
                                        << " sid=" << it->si.id
                                        << " write error: " << UA_StatusCode_name(errcode) << endl;
                        }
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

        string confname = conf->getArgParam("--" + prefix + "-confnode", name);
        xmlNode* cnode = conf->getNode(confname);

        if( !cnode )
        {
            cerr << "(opcua-exchange): " << name << "(init): Not found <" + confname + ">" << endl;
            return nullptr;
        }

        dinfo << "(opcua-exchange): name = " << name << "(" << ID << ")" << endl;
        return make_shared<OPCUAExchange>(ID, cnode, icID, ic, prefix);
    }
    // -----------------------------------------------------------------------------
    void OPCUAExchange::help_print( int argc, const char* const* argv )
    {
        cout << "--run-lock file                - Запустить с защитой от повторного запуска" << endl;
        cout << "--opcua-confnode name          - Использовать для настройки указанный xml-узел" << endl;
        cout << "--opcua-name name              - ID процесса. По умолчанию OPCUAExchange1." << endl;
        cout << "--opcua-polltime msec          - Пауза между циклами обмена. По умолчанию 100 мсек." << endl;
        cout << "--opcua-updatetime msec        - Период обновления данных в/из SM. По умолчанию 100 мсек." << endl;
        cout << "--opcua-filtersize val         - Размерность фильтра для аналоговых входов." << endl;
        cout << "--opcua-filterT val            - Постоянная:: времени фильтра." << endl;
        cout << "--opcua-filter-field           - Идентификатор в configure.xml по которому считывается список относящихся к это процессу датчиков" << endl;
        cout << "--opcua-filter-value           - Значение идентификатора по которому считывается список относящихся к это процессу датчиков" << endl;
        cout << "--opcua-heartbeat-id           - Данный процесс связан с указанным аналоговым heartbeat-датчиком." << endl;
        cout << "--opcua-heartbeat-max          - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
        cout << "--opcua-sm-ready-timeout       - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
        cout << "--opcua-sm-test-sid            - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;
        cout << "--opcua-force                  - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
        cout << "--opcua-force-out              - Обновлять выходы принудительно (не по заказу)" << endl;
        cout << "--opcua-write-to-all-channels  - Всегда писать(write) по всем каналам обмена. По умолчанию только в активном" << endl;
        cout << "--opcua-maxNodesPerRead        - Количество элементов для чтения в одном запросе. По-умолчанию неограниченно" << endl;
        cout << "--opcua-maxNodesPerWrite       - Количество элементов для записи в одном запросе. По-умолчанию неограниченно" << endl;

        cout << "--opcua-skip-init-output  - Не инициализировать 'выходы' при старте" << endl;
        cout << "--opcua-default-exchange-mode  - Режим обмена по-умолчанию при старте процесса" << endl;
        cout << endl;
        cout << " OPC UA: N=[1,2]" << endl;
        cout << "--opcua-addrN addr        - OPC UA server N address (channelN). Default: opc.tcp://localhost:4840/" << endl;
        cout << "--opcua-userN user        - OPC UA server N auth user (channelN). Default: ''(not used)" << endl;
        cout << "--opcua-passM pass        - OPC UA server N auth pass (channelN). Default: ''(not used)" << endl;
        cout << "--opcua-reconnectPause msec  - Pause between reconnect to server, milliseconds" << endl;
        cout << endl;
        cout << " HTTP API: " << endl;
        cout << "--opcua-http-enabled-setparams 1 - Enable API /setparams" << endl;
        cout << endl;
        cout << " Logs: " << endl;
        cout << endl;
        cout << "--opcua-log-...            - log control" << endl;
        cout << "             add-levels ..." << endl;
        cout << "             del-levels ..." << endl;
        cout << "             set-levels ..." << endl;
        cout << "             logfile filename" << endl;
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
        {
            if(v.second->ids.size() == 1)      // Одним запросом опрос происходит
                inf << "write attributes[tick " << setw(2) << (int) v.first << "]: " << v.second->ids[0].size() << endl;
            else if(v.second->ids.size() > 1) // Опрос разбит на несколько запросов из-за ограничений сервера
            {
                inf << "write attributes[tick " << setw(2) << (int) v.first << "]:";
                int sum = 0;

                for(const auto& vv : v.second->ids)
                {
                    sum += vv.size();
                    inf << " " << vv.size();
                }

                inf << " - " << sum << endl;
            }
        }

        if(enableSubscription)
        {
            inf << "subscription items size - " << channels[0].client->getSubscriptionSize() << endl;
        }
        else
        {
            for( const auto& v : channels[0].readValues )
            {
                if(v.second->ids.size() == 1)      // Одним запросом опрос происходит
                    inf << "read attributes[tick " << setw(2) << (int) v.first << "]: " << v.second->ids[0].size() << endl;
                else if(v.second->ids.size() > 1) // Опрос разбит на несколько запросов из-за ограничений сервера
                {
                    inf << "read attributes[tick " << setw(2) << (int) v.first << "]:";
                    int sum = 0;

                    for(const auto& vv : v.second->ids)
                    {
                        sum += vv.size();
                        inf << " " << vv.size();
                    }

                    inf << " - " << sum << endl;
                }
            }
        }

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
                    if( thrChannel[i] && !thrChannel[i]->isRunning() )
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
        if( !shm->isLocalwork() )
            return;

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

        try
        {
            if( sidExchangeMode != DefaultObjectId )
                shm->askSensor(sidExchangeMode, cmd);
        }
        catch( uniset::Exception& ex )
        {
            opccrit << myname << "(askSensors): " << ex << std::endl;
        }
        catch(...)
        {
            opccrit << myname << "(askSensors): 'sidExchangeMode' catch..." << std::endl;
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

            if( !ch->status && ch->ptTimeout.checkTime() )
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

            bool respondOk = false;

            for( size_t i = 0; i < numChannels; i++ )
            {
                Channel* ch = &channels[i];

                if( ch->status || !ch->ptTimeout.checkTime() )
                    respondOk = true;

                if( ch->respond_s == DefaultObjectId )
                    continue;

                try
                {
                    opclog7 << myname << "(timerInfo): channel" << ch->num << " respond=" << (ch->status || !ch->ptTimeout.checkTime()) << endl;
                    shm->localSetValue( ch->respond_it, ch->respond_s, !ch->status && ch->ptTimeout.checkTime() ? 0 : 1, getId());
                }
                catch( const std::exception& ex )
                {
                    opclog6 << myname << "(timerInfo): " << ex.what() << endl;
                }
            }

            if( sidRespond != DefaultObjectId )
            {
                opclog7 << myname << "(timerInfo): respond=" << respondOk << endl;
                shm->localSetValue(itRespond, sidRespond, respondOk, getId());
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

        if( !shm->isLocalwork() )
            return;

        if( force_out )
            return;

        if( sm->id == sidExchangeMode )
        {
            exchangeMode = sm->value;
            opclog3 << myname << "(sensorInfo): exchange MODE=" << sm->value << std::endl;
        }

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
                        it->val = forceSetBits(it->val, IOBase::processingAsAO(ib, shm, force), it->mask, it->offset);
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
                        it->val = forceSetBits(it->val, IOBase::processingAsDO(ib, shm, force) ? 1 : 0, it->mask, it->offset);
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
    void OPCUAExchange::createSubscription(int nchannel)
    {
        if(channels[nchannel].client == nullptr)
            return;

        channels[nchannel].client->onSessionActivated([this, i = nchannel]
        {
            opclog3 << myname << " Session activated " << endl;
            subscription_ok = false;

            auto sub = channels[i].client->createSubscription();

            // Modify and delete the subscription via the returned Subscription<T> object
            opcua::SubscriptionParameters subscriptionParameters{};
            subscriptionParameters.publishingInterval = publishingInterval;
            sub.setSubscriptionParameters(subscriptionParameters);
            sub.setPublishingMode(true);

            std::vector<UA_ReadValueId> items;
            IOList rdlist;
            std::vector<uniset::DataChangeCallback> callbacks;
            std::vector<uniset::DeleteMonitoredItemCallback> deletecallbacks;

            for( const auto& it : iolist )
            {
                if(it->stype != UniversalIO::AI && it->stype != UniversalIO::DI)
                    continue;

                if( it->ignore )
                    continue;

                items.push_back(it->rval[i].ref());
                rdlist.push_back(it);// Копирование std::shared_ptr

                callbacks.emplace_back(
                    [&, i](const auto & item, const opcua::DataValue & value)
                {
                    opclog5 << myname << "item: " << item.itemToMonitor.getNodeId().toString() << " - new value: " << (*(UA_Int32*) value.getValue().data() ) << endl;

                    auto& result = it->rval[i].gr->results[it->rval[i].grNumber][it->rval[i].grIndex];
                    auto data = value.getValue();

                    result.status = value.getStatus();

                    if(data.isType(&UA_TYPES[UA_TYPES_INT32]))
                        result.value = int32_t(*(UA_Int32*) data.data());
                    else if(data.isType(&UA_TYPES[UA_TYPES_UINT32]))
                        result.value = int32_t(*(UA_UInt32*) data.data());

                    if(data.isType(&UA_TYPES[UA_TYPES_INT64]))
                        result.value = int32_t(*(UA_Int64*) data.data());
                    else if(data.isType(&UA_TYPES[UA_TYPES_UINT64]))
                        result.value = int32_t(*(UA_UInt64*) data.data());
                    else if(data.isType(&UA_TYPES[UA_TYPES_BOOLEAN]))
                        result.value = (bool)(*(UA_Boolean*) data.data() ? 1 : 0);

                    if(data.isType(&UA_TYPES[UA_TYPES_INT16]))
                        result.value = int32_t(*(UA_Int16*) data.data());
                    else if(data.isType(&UA_TYPES[UA_TYPES_UINT16]))
                        result.value = int32_t(*(UA_UInt16*) data.data());

                    if(data.isType(&UA_TYPES[UA_TYPES_BYTE]))
                        result.value = int32_t(*(UA_Byte*) data.data());
                    else if(data.isType(&UA_TYPES[UA_TYPES_FLOAT]))
                    {
                        result.type = OPCUAClient::VarType::Float;
                        result.value = (float)(*(UA_Float*) data.data());
                    }
                    else if(data.isType(&UA_TYPES[UA_TYPES_DOUBLE]))
                    {
                        result.type = OPCUAClient::VarType::Float;
                        result.value = (float)(*(UA_Double*) data.data());
                    }
                });
            }

            deletecallbacks.resize(items.size());

            // Create a monitored item within the subscription for data change notifications
            try
            {
                opclog3 << myname << " Create monitoring items : " << items.size() << endl;
                auto t_start = std::chrono::steady_clock::now();

                bool stop = false;

                if((stopOnError == smFirstOnly && connectCount == 0) || stopOnError == smAny)
                    stop = true;

                auto result = channels[i].client->subscribeDataChanges(sub, items, callbacks, deletecallbacks, stop);

                // "result" если запрос прошел успешно, то данные в ответе идут в том же порядке и
                // количестве что и в запросе. Если не удалось подписаться на датчик будет исключение
                // брошено в subscribeDataChanges.
                opcua::MonitoringParametersEx monitoringParameters{};
                monitoringParameters.samplingInterval = samplingInterval;

                assert(result.size() == rdlist.size());

                for(size_t j = 0; j < result.size(); j++)
                {
                    uint32_t monId = result[j].monitoredItemId();

                    if(monId)
                    {
                        result[j].setMonitoringParameters(monitoringParameters);
                        result[j].setMonitoringMode(opcua::MonitoringMode::Reporting);

                        rdlist[j]->rval[i].subscriptionId = result[j].subscriptionId();
                        rdlist[j]->rval[i].monitoredItemId = monId;
                        rdlist[j]->rval[i].subscriptionState = true;
                    }
                    else
                    {
                        opcwarn << "Error subscription for item: " << rdlist[j]->attrName << endl;
                        rdlist[j]->rval[i].subscriptionId = 0;
                        rdlist[j]->rval[i].monitoredItemId = 0;
                        rdlist[j]->rval[i].subscriptionState = false;
                    }
                }

                auto t_end = std::chrono::steady_clock::now();
                opclog8 << myname << "(add monitoring item): " << setw(10) << setprecision(7) << std::fixed
                        << std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count() << " sec" << endl;
            }
            catch(const std::exception& ex)
            {
                // Бросается исключение, а в основном цикле channelThread проверяем на наличие в exceptionCatcher и ретранслируем.
                opcwarn << myname << " Error while subscribe data change : " << ex.what() << endl;
                throw;
            }
            catch(...)
            {
                cerr << "unexpected exception" << endl;
            }

            connectCount++;
            subscription_ok = true;
        });
    }
    // -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr OPCUAExchange::buildLogServerInfo()
    {
        Poco::JSON::Object::Ptr jls = new Poco::JSON::Object();
        jls->set("host", logserv_host);
        jls->set("port", logserv_port);

        if( logserv )
        {
            jls->set("state", logserv->isRunning() ? "RUNNING" : "STOPPED");
            auto info = logserv->httpGetShortInfo();

            if( info )
                jls->set("info", info);
        }
        else
            jls->set("state", "NOT_CONFIGURED");

        return jls;
    }
    // -----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr OPCUAExchange::httpRequest( const std::string& req, const Poco::URI::QueryParameters& p )
    {
        if( req == "getparam" )
            return httpGetParam(p);

        if( req == "setparam" )
            return httpSetParam(p);

        if( req == "status" )
            return httpStatus();

        auto json = UniSetObject::httpRequest(req, p);

        if( !json )
            json = new Poco::JSON::Object();

        json->set("LogServer", buildLogServerInfo());

        return json;
    }
    // -----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr OPCUAExchange::httpHelp( const Poco::URI::QueryParameters& p )
    {
        uniset::json::help::object myhelp(myname, UniSetObject::httpHelp(p));

        {
            uniset::json::help::item cmd("getparam", "read runtime parameters");
            cmd.param("name", "parameter to read; can be repeated");
            cmd.param("note", "supported: polltime | updatetime | reconnectPause | timeoutIterate");
            myhelp.add(cmd);
        }
        {
            uniset::json::help::item cmd("setparam", "set runtime parameters");
            cmd.param("polltime", "ms");
            cmd.param("updatetime", "ms");
            cmd.param("reconnectPause", "ms");
            cmd.param("timeoutIterate", "ms");
            cmd.param("note", "may be disabled by httpEnabledSetParams");
            myhelp.add(cmd);
        }

        return myhelp;
    }
    // -----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr OPCUAExchange::httpGet( const Poco::URI::QueryParameters& p )
    {
        Poco::JSON::Object::Ptr json = UniSetObject::httpGet(p);

        if( !json )
            json = new Poco::JSON::Object();

        Poco::JSON::Object::Ptr jdata = json->getObject(myname);

        if( !jdata )
        {
            jdata = new Poco::JSON::Object();
            json->set(myname, jdata);
        }

        jdata->set("LogServer", buildLogServerInfo());

        return json;
    }
    // -----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr OPCUAExchange::httpSetParam(const Poco::URI::QueryParameters& p)
    {
        if( p.empty() )
            throw uniset::SystemError(myname + "(/setparam): pass key=value pairs");

        if( !httpEnabledSetParams )
            throw uniset::SystemError(myname + "(/setparam): disabled by httpEnabledSetParams");

        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        Poco::JSON::Object::Ptr updated = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr unknown = new Poco::JSON::Array();

        for( const auto& kv : p )
        {
            const auto& name = kv.first;
            const auto& val  = kv.second;

            try
            {
                long v = std::stol(val);

                if( v < 0 )
                    throw uniset::SystemError(myname + "(/setparam): value must be >= 0 (" + name + ")");

                if( name == "polltime" )
                {
                    polltime = (timeout_t)v;
                    updated->set(name, (int)polltime);
                }
                else if( name == "updatetime" )
                {
                    updatetime = (timeout_t)v;
                    updated->set(name, (int)updatetime);
                }
                else if( name == "reconnectPause" )
                {
                    reconnectPause = (timeout_t)v;
                    updated->set(name, (int)reconnectPause);
                }
                else if( name == "timeoutIterate" )
                {
                    timeoutIterate = static_cast<uint16_t>(v);
                    updated->set(name, (int)timeoutIterate);
                }
                else
                {
                    unknown->add(name);
                }
            }
            catch(const std::exception& e)
            {
                throw uniset::SystemError(myname + "(/setparam): invalid value for '" + name + "'");
            }
        }

        out->set("result", "OK");
        out->set("updated", updated);

        if( unknown->size() > 0 )
            out->set("unknown", unknown);

        return out;
    }
    // -----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr OPCUAExchange::httpGetParam(const Poco::URI::QueryParameters& p)
    {
        if( p.empty() )
            throw uniset::SystemError(myname + "(/getparam): pass at least one 'name' parameter");

        std::vector<std::string> names;

        for( const auto& kv : p )
            if( kv.first == "name" && !kv.second.empty() )
                names.push_back(kv.second);

        if( names.empty() )
            throw uniset::SystemError(myname + "(/getparam): parameter 'name' is required");

        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        Poco::JSON::Object::Ptr params = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr unknown = new Poco::JSON::Array();

        for( const auto& n : names )
        {
            if( n == "polltime" )
                params->set(n, (int)polltime);
            else if( n == "updatetime" )
                params->set(n, (int)updatetime);
            else if( n == "reconnectPause" )
                params->set(n, (int)reconnectPause);
            else if( n == "timeoutIterate" )
                params->set(n, (int)timeoutIterate);
            else
                unknown->add(n);
        }

        out->set("result", "OK");
        out->set("params", params);

        if( unknown->size() > 0 )
            out->set("unknown", unknown);

        return out;
    }
    // -----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr OPCUAExchange::httpStatus()
    {
        using Poco::JSON::Object;
        using Poco::JSON::Array;

        Object::Ptr st = new Object();

        st->set("name", myname);

        // 2) LogServer
        st->set("LogServer", buildLogServerInfo());

        // 3) Каналы: статус + addr
        {
            Array::Ptr chs = new Array();

            for( size_t i = 0; i < numChannels; ++i )
            {
                Object::Ptr ch = new Object();
                ch->set("index", (int)i);  // нумерация с 0 (в getInfo печать идет с i+1)
                const bool disabled = channels[i].addr.empty();
                ch->set("disabled", disabled ? 1 : 0);

                if( !disabled )
                {
                    ch->set("ok", channels[i].status ? 1 : 0);
                    ch->set("addr", channels[i].addr);
                }

                chs->add(ch);
            }

            st->set("channels", chs);
        }

        // 4) iolist size
        st->set("iolist_size", (int)iolist.size());

        // 5) Подписка или пачечные чтения/записи
        if( enableSubscription )
        {
            Object::Ptr sub = new Object();
            int subSize = 0;

            if( channels[0].client )
                subSize = channels[0].client->getSubscriptionSize();

            sub->set("enabled", 1);
            sub->set("items", subSize);
            st->set("subscription", sub);
        }
        else
        {
            // read attributes by tick
            Array::Ptr reads = new Array();

            if( channels[0].client )
            {
                for( const auto& kv : channels[0].readValues )
                {
                    Object::Ptr tick = new Object();
                    tick->set("tick", (int)kv.first);
                    const auto& idsVec = kv.second->ids;
                    Array::Ptr batches = new Array();
                    int total = 0;

                    for( const auto& batch : idsVec )
                    {
                        int n = (int)batch.size();
                        batches->add(n);
                        total += n;
                    }

                    tick->set("batches", batches);
                    tick->set("total", total);
                    reads->add(tick);
                }
            }

            st->set("read_attributes", reads);

            // write attributes by tick
            Array::Ptr writes = new Array();

            if( channels[0].client )
            {
                for( const auto& kv : channels[0].writeValues )
                {
                    Object::Ptr tick = new Object();
                    tick->set("tick", (int)kv.first);
                    const auto& idsVec = kv.second->ids;
                    Array::Ptr batches = new Array();
                    int total = 0;

                    for( const auto& batch : idsVec )
                    {
                        int n = (int)batch.size();
                        batches->add(n);
                        total += n;
                    }

                    tick->set("batches", batches);
                    tick->set("total", total);
                    writes->add(tick);
                }
            }

            st->set("write_attributes", writes);
        }

        // 6) Мониторинг (vmon.pretty_str())
        st->set("monitor", vmon.pretty_str());

        if( httpEnabledSetParams )
            st->set("httpEnabledSetParams", 1);
        else
            st->set("httpEnabledSetParams", 0);

        // final
        Poco::JSON::Object::Ptr out = new Object();
        out->set("result", "OK");
        out->set("status", st);
        return out;
    }
    // -----------------------------------------------------------------------------

#endif // DISABLE_REST_API
    // -----------------------------------------------------------------------------
} // end of namespace uniset
