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
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <UniSetTypes.h>
#include <extensions/Extensions.h>
#include <ORepHelpers.h>
#include "MBExchange.h"
#include "modbus/MBLogSugar.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    using namespace std;
    using namespace uniset::extensions;
    // -----------------------------------------------------------------------------
    MBExchange::MBExchange(uniset::ObjectId objId, uniset::ObjectId shmId,
                           const std::shared_ptr<SharedMemory>& _ic, const std::string& prefix ):
        UniSetObject(objId),
        allInitOK(false),
        force(false),
        force_out(false),
        sidExchangeMode(DefaultObjectId),
        exchangeMode(MBConfig::emNone),
        activated(false),
        notUseExchangeTimer(false),
        poll_count(0),
        mb(nullptr),
        ic(_ic)
    {
        if( objId == DefaultObjectId )
            throw uniset::SystemError("(MBExchange): objId=-1?!! Use --" + prefix + "-name" );

        auto conf = uniset_conf();
        mutex_start.setName(myname + "_mutgc7vdgchex_start");

        string conf_name(conf->getArgParam("--" + prefix + "-confnode", myname));

        cnode = conf->getNode(conf_name);

        if( cnode == NULL )
            throw uniset::SystemError("(MBExchange): Not found node <" + conf_name + " ...> for " + myname );

        shm = make_shared<SMInterface>(shmId, ui, objId, ic);
        mbconf = make_shared<MBConfig>(conf, cnode, shm);

        mblog = make_shared<DebugStream>();
        mblog->setLogName(myname);
        conf->initLogStream(mblog, prefix + "-log");
        mbconf->mblog = mblog;
        mbconf->myname = myname;

        loga = make_shared<LogAgregator>(myname + "-loga");
        loga->add(mblog);
        loga->add(ulog());
        loga->add(dlog());

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
        mbconf->s_field = conf->getArgParam("--" + prefix + "-filter-field");
        mbconf->s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
        mbinfo << myname << "(init): read fileter-field='" << mbconf->s_field
               << "' filter-value='" << mbconf->s_fvalue << "'" << endl;

        mbconf->prefix = prefix;
        mbconf->prop_prefix = initPropPrefix(mbconf->s_field, "");

        stat_time = conf->getArgPInt("--" + prefix + "-statistic-sec", it.getProp("statistic_sec"), 0);
        vmonit(stat_time);

        if( stat_time > 0 )
            ptStatistic.setTiming(stat_time * 1000);

        mbconf->recv_timeout = conf->getArgPInt("--" + prefix + "-recv-timeout", it.getProp("recv_timeout"), 500);
        mbconf->default_timeout = conf->getArgPInt("--" + prefix + "-timeout", it.getProp("timeout"), 5000);

        int tout = conf->getArgPInt("--" + prefix + "-reopen-timeout", it.getProp("reopen_timeout"), mbconf->default_timeout * 2);
        ptReopen.setTiming(tout);

        int reinit_tout = conf->getArgPInt("--" + prefix + "-reinit-timeout", it.getProp("reinit_timeout"), mbconf->default_timeout);
        ptInitChannel.setTiming(reinit_tout);

        mbconf->aftersend_pause = conf->getArgPInt("--" + prefix + "-aftersend-pause", it.getProp("aftersend_pause"), 0);
        mbconf->noQueryOptimization = conf->getArgInt("--" + prefix + "-no-query-optimization", it.getProp("no_query_optimization"));
        mbconf->mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id", it.getProp("reg_from_id"));
        mbinfo << myname << "(init): mbregFromID=" << mbconf->mbregFromID << endl;

        mbconf->polltime = conf->getArgPInt("--" + prefix + "-polltime", it.getProp("polltime"), 100);
        initPause = conf->getArgPInt("--" + prefix + "-initPause", it.getProp("initPause"), 3000);

        mbconf->sleepPause_msec = conf->getArgPInt("--" + prefix + "-sleepPause-msec", it.getProp("sleepPause"), 10);

        force = conf->getArgInt("--" + prefix + "-force", it.getProp("force"));
        force_out = conf->getArgInt("--" + prefix + "-force-out", it.getProp("force_out"));
        vmonit(force);
        vmonit(force_out);

        mbconf->defaultMBtype = conf->getArg2Param("--" + prefix + "-default-mbtype", it.getProp("default_mbtype"), "rtu");
        mbconf->defaultMBaddr = conf->getArg2Param("--" + prefix + "-default-mbaddr", it.getProp("default_mbaddr"), "");
        mbconf->defaultMBinitOK = conf->getArgPInt("--" + prefix + "-default-mbinit-ok", it.getProp("default_mbinitOK"), 0);
        mbconf->maxQueryCount = conf->getArgPInt("--" + prefix + "-query-max-count", it.getProp("queryMaxCount"), ModbusRTU::MAXDATALEN);

        // ********** HEARTBEAT *************
        string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", it.getProp("heartbeat_id"));

        if( !heart.empty() )
        {
            sidHeartBeat = conf->getSensorID(heart);

            if( sidHeartBeat == DefaultObjectId )
            {
                ostringstream err;
                err << myname << ": ID not found ('HeartBeat') for " << heart;
                mbcrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }

            int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time", it.getProp("heartbeatTime"), conf->getHeartBeatTime());

            if( heartbeatTime )
                ptHeartBeat.setTiming(heartbeatTime);
            else
                ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

            maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", it.getProp("heartbeat_max"), 10);
            test_id = sidHeartBeat;
        }
        else
        {
            test_id = conf->getSensorID("TestMode_S");

            if( test_id == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): test_id unknown. 'TestMode_S' not found...";
                mbcrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }
        }

        mbinfo << myname << "(init): test_id=" << test_id << endl;

        string emode = conf->getArgParam("--" + prefix + "-exchange-mode-id", it.getProp("exchangeModeID"));

        if( !emode.empty() )
        {
            sidExchangeMode = conf->getSensorID(emode);

            if( sidExchangeMode == DefaultObjectId )
            {
                ostringstream err;
                err << myname << ": ID not found ('exchangeModeID') for " << emode;
                mbcrit << myname << "(init): " << err.str() << endl;
                throw SystemError(err.str());
            }
        }

        activateTimeout = conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

        vmonit(allInitOK);
    }
    // -----------------------------------------------------------------------------
    std::string MBExchange::initPropPrefix( const std::string& s_field,
                                            const std::string& def_prop_prefix )
    {
        auto conf = uniset_conf();

        string pp = def_prop_prefix;

        // если задано поле для "фильтрации"
        // то в качестве префикса используем его
        if( !s_field.empty() )
            pp = s_field + "_";

        // если "принудительно" задан префикс используем его.
        const string p = "--" + mbconf->prefix + "-set-prop-prefix";
        const string v = conf->getArgParam(p, "");

        if( !v.empty() && v[0] != '-' )
            pp = v;
        // если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
        else if( findArgParam(p, conf->getArgc(), conf->getArgv()) != -1 )
            pp = "";

        return pp;
    }
    // -----------------------------------------------------------------------------
    void MBExchange::help_print( int argc, const char* const* argv )
    {
        cout << "--prefix-name name              - ObjectId (имя) процесса. По умолчанию: MBExchange1" << endl;
        cout << "--prefix-confnode name          - Настроечная секция в конф. файле <name>. " << endl;
        cout << "--prefix-polltime msec          - Пауза между опросами. По умолчанию 100 мсек." << endl;
        cout << "--prefix-recv-timeout msec      - Таймаут на приём одного сообщения" << endl;
        cout << "--prefix-timeout msec           - Таймаут для определения отсутствия соединения" << endl;
        cout << "--prefix-aftersend-pause msec   - Пауза после посылки запроса (каждого). По умолчанию: 0." << endl;
        cout << "--prefix-reopen-timeout msec    - Таймаут для 'переоткрытия соединения' при отсутствия соединения msec милисекунд. По умолчанию 10 сек." << endl;
        cout << "--prefix-heartbeat-id  name     - Данный процесс связан с указанным аналоговым heartbeat-датчиком." << endl;
        cout << "--prefix-heartbeat-max val      - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
        cout << "--prefix-ready-timeout msec     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
        cout << "--prefix-force 0,1              - Сохранять значения в SM на каждом шаге, независимо от, того менялось ли значение" << endl;
        cout << "--prefix-force-out 0,1          - Считывать значения 'выходов' из SM на каждом шаге (а не по изменению)" << endl;
        cout << "--prefix-initPause msec         - Задержка перед инициализацией (время на активизация процесса)" << endl;
        cout << "--prefix-no-query-optimization 0,1 - Не оптимизировать запросы (не объединять соседние регистры в один запрос)" << endl;
        cout << "--prefix-reg-from-id 0,1        - Использовать в качестве регистра sensor ID" << endl;
        cout << "--prefix-filter-field name      - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
        cout << "--prefix-filter-value val       - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
        cout << "--prefix-statistic-sec sec      - Выводить статистику опроса каждые sec секунд" << endl;
        cout << "--prefix-sm-ready-timeout       - время на ожидание старта SM" << endl;
        cout << "--prefix-exchange-mode-id       - Идентификатор (AI) датчика, позволяющего управлять работой процесса" << endl;
        cout << "--prefix-set-prop-prefix val    - Использовать для свойств указанный или пустой префикс." << endl;
        cout << "--prefix-default-mbtype [rtu|rtu188|mtr]  - У датчиков которых не задан 'mbtype' использовать данный. По умолчанию: 'rtu'" << endl;
        cout << "--prefix-default-mbadd addr     - У датчиков которых не задан 'mbaddr' использовать данный. По умолчанию: ''" << endl;
        cout << "--prefix-default-mbinit-ok 0,1  - Флаг инициализации. 1 - не ждать первого обмена с устройством, а сохранить при старте в SM значение 'default'" << endl;
        cout << "--prefix-query-max-count max    - Максимальное количество запрашиваемых за один раз регистров (При условии no-query-optimization=0). По умолчанию: " << ModbusRTU::MAXDATALEN << "." << endl;
        cout << endl;
        cout << " Logs: " << endl;
        cout << "--prefix-log-...            - log control" << endl;
        cout << "             add-levels ...  " << endl;
        cout << "             del-levels ...  " << endl;
        cout << "             set-levels ...  " << endl;
        cout << "             logfile filanme " << endl;
        cout << "             no-debug " << endl;
        cout << " LogServer: " << endl;
        cout << "--prefix-run-logserver      - run logserver. Default: localhost:id" << endl;
        cout << "--prefix-logserver-host ip  - listen ip. Default: localhost" << endl;
        cout << "--prefix-logserver-port num - listen port. Default: ID" << endl;
        cout << LogServer::help_print("prefix-logserver") << endl;
    }

    // -----------------------------------------------------------------------------
    MBExchange::~MBExchange()
    {
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::waitSMReady()
    {
        // waiting for SM is ready...
        int tout = uniset_conf()->getArgInt("--" + mbconf->prefix + "-sm-ready-timeout", "");
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
    void MBExchange::step()
    {
        if( !isProcActive() )
            return;

        if( ptInitChannel.checkTime() )
            updateRespondSensors();

        if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
        {
            try
            {
                shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
                ptHeartBeat.reset();
            }
            catch( const uniset::Exception& ex )
            {
                mbcrit << myname << "(step): (hb) " << ex << std::endl;
            }
        }
    }

    // -----------------------------------------------------------------------------
    bool MBExchange::isProcActive() const
    {
        return activated && !canceled;
    }
    // -----------------------------------------------------------------------------
    void MBExchange::setProcActive( bool st )
    {
        activated = st;
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::deactivateObject()
    {
        setProcActive(false);
        canceled = true;

        if( logserv && logserv->isRunning() )
            logserv->terminate();

        return UniSetObject::deactivateObject();
    }
    // ------------------------------------------------------------------------------------------
    bool MBExchange::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
    {
        if( uniset::check_filter(it, mbconf->s_field, mbconf->s_fvalue) )
        {
            try
            {
                mbconf->initItem(it);
            }
            catch( std::exception& ex )
            {
                cerr << ex.what() << endl;
                std::abort();
            }
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    void MBExchange::initIterators()
    {
        shm->initIterator(itHeartBeat);
        shm->initIterator(itExchangeMode);

        for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
        {
            auto d = it1->second;
            shm->initIterator(d->resp_it);
            shm->initIterator(d->mode_it);

            for( auto&& m : d->pollmap )
            {
                auto& regmap = m.second;

                for( auto it = regmap->begin(); it != regmap->end(); ++it )
                {
                    for( auto it2 = it->second->slst.begin(); it2 != it->second->slst.end(); ++it2 )
                    {
                        shm->initIterator(it2->ioit);
                        shm->initIterator(it2->t_ait);
                    }
                }
            }
        }

        for( auto t = mbconf->thrlist.begin(); t != mbconf->thrlist.end(); ++t )
        {
            shm->initIterator(t->ioit);
            shm->initIterator(t->t_ait);
        }
    }
    // -----------------------------------------------------------------------------
    void MBExchange::initValues()
    {
        for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
        {
            auto d = it1->second;

            for( auto&& m : d->pollmap )
            {
                auto regmap = m.second;

                for( auto it = regmap->begin(); it != regmap->end(); ++it )
                {
                    for( auto it2 = it->second->slst.begin(); it2 != it->second->slst.end(); ++it2 )
                    {
                        it2->value = shm->localGetValue(it2->ioit, it2->si.id);
                    }

                    it->second->sm_initOK = true;
                }
            }
        }

        for( auto t = mbconf->thrlist.begin(); t != mbconf->thrlist.end(); ++t )
        {
            t->value = shm->localGetValue(t->ioit, t->si.id);
        }
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::isUpdateSM( bool wrFunc, long mdev ) const noexcept
    {
        if( exchangeMode == MBConfig::emSkipExchange || mdev == MBConfig::emSkipExchange )
        {
            if( wrFunc )
                return true; // данные для посылки, должны обновляться всегда (чтобы быть актуальными, когда режим переключиться обратно..)

            mblog3 << "(checkUpdateSM):"
                   << " skip... mode='emSkipExchange' " << endl;
            return false;
        }

        if( wrFunc && (exchangeMode == MBConfig::emReadOnly || mdev == MBConfig::emReadOnly) )
        {
            mblog3 << "(checkUpdateSM):"
                   << " skip... mode='emReadOnly' " << endl;
            return false;
        }

        if( !wrFunc && (exchangeMode == MBConfig::emWriteOnly || mdev == MBConfig::emWriteOnly) )
        {
            mblog3 << "(checkUpdateSM):"
                   << " skip... mode='emWriteOnly' " << endl;
            return false;
        }

        if( !wrFunc && (exchangeMode == MBConfig::emSkipSaveToSM || mdev == MBConfig::emSkipSaveToSM) )
        {
            mblog3 << "(checkUpdateSM):"
                   << " skip... mode='emSkipSaveToSM' " << endl;
            return false;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::isPollEnabled( bool wrFunc ) const noexcept
    {
        if( exchangeMode == MBConfig::emWriteOnly && !wrFunc )
        {
            mblog3 << myname << "(checkPoll): skip.. mode='emWriteOnly'" << endl;
            return false;
        }

        if( exchangeMode == MBConfig::emReadOnly && wrFunc )
        {
            mblog3 << myname << "(checkPoll): skip.. poll mode='emReadOnly'" << endl;
            return false;
        }

        if( exchangeMode == MBConfig::emSkipExchange )
        {
            mblog3 << myname << "(checkPoll): skip.. poll mode='emSkipExchange'" << endl;
            return false;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::isSafeMode( std::shared_ptr<MBConfig::RTUDevice>& dev ) const noexcept
    {
        if( !dev )
            return false;

        // если режим, сброс когда исчезла связь
        // то проверяем таймер
        // resp_Delay - это задержка на отпускание "пропадание" связи,
        // поэтому проверка на "0" (0 - связи нет), а значит должен включиться safeMode
        if( dev->safeMode == MBConfig::safeResetIfNotRespond )
            return !dev->resp_Delay.get();

        return ( dev->safeMode != MBConfig::safeNone );
    }
    // -----------------------------------------------------------------------------
    void MBExchange::firstInitRegisters()
    {
        // если все вернут TRUE, значит OK.
        allInitOK = true;

        for( auto it = mbconf->initRegList.begin(); it != mbconf->initRegList.end(); ++it )
        {
            try
            {
                it->initOK = preInitRead(it);
                allInitOK = it->initOK;
            }
            catch( ModbusRTU::mbException& ex )
            {
                allInitOK = false;
                mblog3 << myname << "(preInitRead): FAILED ask addr=" << ModbusRTU::addr2str(it->dev->mbaddr)
                       << " reg=" << ModbusRTU::dat2str(it->mbreg)
                       << " err: " << ex << endl;

                if( !it->dev->ask_every_reg )
                    break;
            }
        }
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::preInitRead( MBConfig::InitList::iterator& p )
    {
        mbinfo << "init mbreg=" << ModbusRTU::dat2str(p->mbreg) << " initOK=" << p->initOK << endl;

        if( p->initOK )
            return true;

        auto dev = p->dev;
        size_t q_count = p->p.rnum;

        if( mblog->is_level3()  )
        {
            mblog3 << myname << "(preInitRead): poll "
                   << " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
                   << " mbreg=" << ModbusRTU::dat2str(p->mbreg)
                   << " mbfunc=" << p->mbfunc
                   << " q_count=" << q_count
                   << endl;

            if( q_count > mbconf->maxQueryCount /* ModbusRTU::MAXDATALEN */ )
            {
                mblog3 << myname << "(preInitRead): count(" << q_count
                       << ") > MAXDATALEN(" << mbconf->maxQueryCount /* ModbusRTU::MAXDATALEN */
                       << " ..ignore..."
                       << endl;
            }
        }

        switch( p->mbfunc )
        {
            case ModbusRTU::fnReadOutputRegisters:
            {
                ModbusRTU::ReadOutputRetMessage ret = mb->read03(dev->mbaddr, p->mbreg, q_count);
                p->initOK = initSMValue(ret.data, ret.count, &(p->p));
            }
            break;

            case ModbusRTU::fnReadInputRegisters:
            {
                ModbusRTU::ReadInputRetMessage ret1 = mb->read04(dev->mbaddr, p->mbreg, q_count);
                p->initOK = initSMValue(ret1.data, ret1.count, &(p->p));
            }
            break;

            case ModbusRTU::fnReadInputStatus:
            {
                ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(dev->mbaddr, p->mbreg, q_count);
                ModbusRTU::DataGuard d(q_count);
                size_t m = 0;

                for( size_t i = 0; i < ret.bcnt; i++ )
                {
                    ModbusRTU::DataBits b(ret.data[i]);

                    for( size_t k = 0; k < ModbusRTU::BitsPerByte && m < q_count; k++, m++ )
                        d.data[m] = b[k];
                }

                p->initOK = initSMValue(d.data, d.len, &(p->p));
            }
            break;

            case ModbusRTU::fnReadCoilStatus:
            {
                ModbusRTU::ReadCoilRetMessage ret = mb->read01(dev->mbaddr, p->mbreg, q_count);
                ModbusRTU::DataGuard d(q_count);
                size_t m = 0;

                for( size_t i = 0; i < ret.bcnt; i++ )
                {
                    ModbusRTU::DataBits b(ret.data[i]);

                    for( size_t k = 0; k < ModbusRTU::BitsPerByte && m < q_count; k++, m++ )
                        d.data[m] = b[k];
                }

                p->initOK = initSMValue(d.data, d.len, &(p->p));
            }
            break;

            default:
                p->initOK = false;
                break;
        }

        if( p->initOK )
        {
            bool f_out = force_out;
            // выставляем флаг принудительного обновления
            force_out = true;
            p->ri->mb_initOK = true;
            // p->ri->sm_initOK = false;
            updateRTU(p->ri->rit);
            force_out = f_out;
        }

        return p->initOK;
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::initSMValue( ModbusRTU::ModbusData* data, int count, MBConfig::RSProperty* p )
    {
        using namespace ModbusRTU;

        try
        {
            if( p->vType == VTypes::vtUnknown )
            {
                ModbusRTU::DataBits16 b(data[0]);

                if( p->nbit >= 0 )
                {
                    bool set = b[p->nbit];
                    IOBase::processingAsDI( p, set, shm, true );
                    return true;
                }

                if( p->rnum <= 1 )
                {
                    if( p->stype == UniversalIO::DI ||
                            p->stype == UniversalIO::DO )
                    {
                        IOBase::processingAsDI( p, data[0], shm, true );
                    }
                    else
                        IOBase::processingAsAI( p, (int16_t)(data[0]), shm, true );

                    return true;
                }

                mbcrit << myname << "(initSMValue): IGNORE item: rnum=" << p->rnum
                       << " > 1 ?!! for id=" << p->si.id << endl;

                return false;
            }
            else if( p->vType == VTypes::vtSigned )
            {
                if( p->stype == UniversalIO::DI ||
                        p->stype == UniversalIO::DO )
                {
                    IOBase::processingAsDI( p, data[0], shm, true );
                }
                else
                    IOBase::processingAsAI( p, (int16_t)(data[0]), shm, true );

                return true;
            }
            else if( p->vType == VTypes::vtUnsigned )
            {
                if( p->stype == UniversalIO::DI ||
                        p->stype == UniversalIO::DO )
                {
                    IOBase::processingAsDI( p, data[0], shm, true );
                }
                else
                    IOBase::processingAsAI( p, (uint16_t)data[0], shm, true );

                return true;
            }
            else if( p->vType == VTypes::vtByte )
            {
                if( p->nbyte <= 0 || p->nbyte > VTypes::Byte::bsize )
                {
                    mbcrit << myname << "(initSMValue): IGNORE item: sid=" << ModbusRTU::dat2str(p->si.id)
                           << " vtype=" << p->vType << " but nbyte=" << p->nbyte << endl;
                    return false;
                }

                VTypes::Byte b(data[0]);
                IOBase::processingAsAI( p, b.raw.b[p->nbyte - 1], shm, true );
                return true;
            }
            else if( p->vType == VTypes::vtF2 )
            {
                VTypes::F2 f(data, VTypes::F2::wsize());
                IOBase::processingFasAI( p, (float)f, shm, true );
            }
            else if( p->vType == VTypes::vtF2r )
            {
                VTypes::F2r f(data, VTypes::F2r::wsize());
                IOBase::processingFasAI( p, (float)f, shm, true );
            }
            else if( p->vType == VTypes::vtF4 )
            {
                VTypes::F4 f(data, VTypes::F4::wsize());
                IOBase::processingF64asAI( p, (double)f, shm, true );
            }
            else if( p->vType == VTypes::vtI2 )
            {
                VTypes::I2 i2(data, VTypes::I2::wsize());
                IOBase::processingAsAI( p, (int32_t)i2, shm, true );
            }
            else if( p->vType == VTypes::vtI2r )
            {
                VTypes::I2r i2(data, VTypes::I2::wsize());
                IOBase::processingAsAI( p, (int32_t)i2, shm, true );
            }
            else if( p->vType == VTypes::vtU2 )
            {
                VTypes::U2 u2(data, VTypes::U2::wsize());
                IOBase::processingAsAI( p, (uint32_t)u2, shm, true );
            }
            else if( p->vType == VTypes::vtU2r )
            {
                VTypes::U2r u2(data, VTypes::U2::wsize());
                IOBase::processingAsAI( p, (uint32_t)u2, shm, true );
            }

            return true;
        }
        catch(IOController_i::NameNotFound& ex)
        {
            mblog3 << myname << "(initSMValue):(NameNotFound) " << ex.err << endl;
        }
        catch(IOController_i::IOBadParam& ex )
        {
            mblog3 << myname << "(initSMValue):(IOBadParam) " << ex.err << endl;
        }
        catch(IONotifyController_i::BadRange& ex )
        {
            mblog3 << myname << "(initSMValue): (BadRange)..." << endl;
        }
        catch( const CORBA::SystemException& ex)
        {
            mblog3 << myname << "(initSMValue): CORBA::SystemException: "
                   << ex.NP_minorString() << endl;
        }
        catch( const uniset::Exception& ex )
        {
            mblog3 << myname << "(initSMValue): " << ex << endl;
        }
        catch(...)
        {
            mblog3 << myname << "(initSMValue): catch ..." << endl;
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::pollRTU( std::shared_ptr<MBConfig::RTUDevice>& dev, MBConfig::RegMap::iterator& it )
    {
        auto p = it->second;

        if( dev->mode == MBConfig::emSkipExchange )
        {
            mblog3 << myname << "(pollRTU): SKIP EXCHANGE (mode=emSkipExchange) "
                   << " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
                   << endl;
            return false;
        }

        if( mblog->is_level3() )
        {
            mblog3 << myname << "(pollRTU): poll "
                   << " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
                   << " mbreg=" << ModbusRTU::dat2str(p->mbreg)
                   << " mbfunc=" << p->mbfunc
                   << " q_count=" << p->q_count
                   << " mb_initOK=" << p->mb_initOK
                   << " sm_initOK=" << p->sm_initOK
                   << " mbval=" << p->mbval
                   << endl;

            if( p->q_count > mbconf->maxQueryCount /* ModbusRTU::MAXDATALEN */ )
            {
                mblog3 << myname << "(pollRTU): count(" << p->q_count
                       << ") > MAXDATALEN(" << mbconf->maxQueryCount /* ModbusRTU::MAXDATALEN */
                       << " ..ignore..."
                       << endl;
            }
        }

        if( !isPollEnabled(ModbusRTU::isWriteFunction(p->mbfunc)) )
            return true;

        if( p->q_count == 0 )
        {
            mbinfo << myname << "(pollRTU): q_count=0 for mbreg=" << ModbusRTU::dat2str(p->mbreg)
                   << " IGNORE register..." << endl;
            return false;
        }

        switch( p->mbfunc )
        {
            case ModbusRTU::fnReadInputRegisters:
            {
                ModbusRTU::ReadInputRetMessage ret = mb->read04(dev->mbaddr, p->mbreg, p->q_count);

                for( size_t i = 0; i < p->q_count; i++, it++ )
                {
                    it->second->mbval = ret.data[i];
                    it->second->mb_initOK = true;
                }

                it--;
            }
            break;

            case ModbusRTU::fnReadOutputRegisters:
            {
                ModbusRTU::ReadOutputRetMessage ret = mb->read03(dev->mbaddr, p->mbreg, p->q_count);

                for( size_t i = 0; i < p->q_count; i++, it++ )
                {
                    it->second->mbval = ret.data[i];
                    it->second->mb_initOK = true;
                }

                it--;
            }
            break;

            case ModbusRTU::fnReadInputStatus:
            {
                ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(dev->mbaddr, p->mbreg, p->q_count);
                size_t m = 0;

                for( uint i = 0; i < ret.bcnt; i++ )
                {
                    ModbusRTU::DataBits b(ret.data[i]);

                    for( size_t k = 0; k < ModbusRTU::BitsPerByte && m < p->q_count; k++, it++, m++ )
                    {
                        it->second->mbval = b[k];
                        it->second->mb_initOK = true;
                    }
                }

                it--;
            }
            break;

            case ModbusRTU::fnReadCoilStatus:
            {
                ModbusRTU::ReadCoilRetMessage ret = mb->read01(dev->mbaddr, p->mbreg, p->q_count);
                size_t m = 0;

                for( auto i = 0; i < ret.bcnt; i++ )
                {
                    ModbusRTU::DataBits b(ret.data[i]);

                    for( size_t k = 0; k < ModbusRTU::BitsPerByte && m < p->q_count; k++, it++, m++ )
                    {
                        it->second->mbval = b[k] ? 1 : 0;
                        it->second->mb_initOK = true;
                    }
                }

                it--;
            }
            break;

            case ModbusRTU::fnWriteOutputSingleRegister:
            {
                if( p->q_count != 1 )
                {
                    mbcrit << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                           << " IGNORE WRITE SINGLE REGISTER (0x06) q_count=" << p->q_count << " ..." << endl;
                    return false;
                }

                if( !p->sm_initOK )
                {
                    mblog3 << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                           << " slist=" << (*p)
                           << " IGNORE...(sm_initOK=false)" << endl;
                    return true;
                }

                // игнорируем return т.к. в случае ошибки будет исключение..
                (void)mb->write06(dev->mbaddr, p->mbreg, p->mbval);
            }
            break;

            case ModbusRTU::fnWriteOutputRegisters:
            {
                if( !p->sm_initOK )
                {
                    // может быть такая ситуация, что
                    // некоторые регистры уже инициализированы, а другие ещё нет
                    // при этом после оптимизации они попадают в один запрос
                    // поэтому здесь сделано так:
                    // если q_num > 1, значит этот регистр относится к предыдущему запросу
                    // и его просто надо пропустить..
                    if( p->q_num > 1 )
                        return true;

                    mblog3 << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                           << " IGNORE...(sm_initOK=false)" << endl;
                    return true;
                }

                ModbusRTU::WriteOutputMessage msg(dev->mbaddr, p->mbreg);

                for( size_t i = 0; i < p->q_count; i++, it++ )
                    msg.addData(it->second->mbval);

                it--;

                // игнорируем return т.к. в случае ошибки будет исключение..
                (void)mb->write10(msg);
            }
            break;

            case ModbusRTU::fnForceSingleCoil:
            {
                if( p->q_count != 1 )
                {
                    mbcrit << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                           << " IGNORE FORCE SINGLE COIL (0x05) q_count=" << p->q_count << " ..." << endl;
                    return false;
                }

                if( !p->sm_initOK )
                {
                    mblog3 << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                           << " IGNORE...(sm_initOK=false)" << endl;
                    return true;
                }

                // игнорируем return т.к. в случае ошибки будет исключение..
                (void)mb->write05(dev->mbaddr, p->mbreg, p->mbval);
            }
            break;

            case ModbusRTU::fnForceMultipleCoils:
            {
                if( !p->sm_initOK )
                {
                    mblog3 << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                           << " IGNORE...(sm_initOK=false)" << endl;
                    return true;
                }

                ModbusRTU::ForceCoilsMessage msg(dev->mbaddr, p->mbreg);

                for( size_t i = 0; i < p->q_count; i++, it++ )
                    msg.addBit( (it->second->mbval ? true : false) );

                it--;
                // игнорируем return т.к. в случае ошибки будет исключение..
                (void)mb->write0F(msg);
            }
            break;

            default:
            {
                mbwarn << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
                       << " IGNORE mfunc=" << (int)p->mbfunc << " ..." << endl;
                return false;
            }
            break;
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    void MBExchange::updateSM()
    {
        for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
        {
            auto d = it1->second;

            if( d->mode_id != DefaultObjectId )
            {
                try
                {
                    if( !shm->isLocalwork() )
                        d->mode = shm->localGetValue(d->mode_it, d->mode_id);
                }
                catch(IOController_i::NameNotFound& ex)
                {
                    mblog3 << myname << "(updateSM):(NameNotFound) " << ex.err << endl;
                }
                catch(IOController_i::IOBadParam& ex )
                {
                    mblog3 << myname << "(updateSM):(IOBadParam) " << ex.err << endl;
                }
                catch( IONotifyController_i::BadRange& ex )
                {
                    mblog3 << myname << "(updateSM): (BadRange)..." << endl;
                }
                catch( const CORBA::SystemException& ex )
                {
                    mblog3 << myname << "(updateSM): CORBA::SystemException: "
                           << ex.NP_minorString() << endl;
                }
                catch( const uniset::Exception& ex )
                {
                    mblog3 << myname << "(updateSM): " << ex << endl;
                }
                catch( std::exception& ex )
                {
                    mblog3 << myname << "(updateSM): check modeSensor: " << ex.what() << endl;
                }

                if( d->safemode_id != DefaultObjectId )
                {
                    try
                    {
                        if( !shm->isLocalwork() )
                        {
                            if( shm->localGetValue(d->safemode_it, d->safemode_id) == d->safemode_value )
                                d->safeMode = MBConfig::safeExternalControl;
                            else
                                d->safeMode = MBConfig::safeNone;
                        }
                    }
                    catch(IOController_i::NameNotFound& ex)
                    {
                        mblog3 << myname << "(updateSM):(NameNotFound) " << ex.err << endl;
                    }
                    catch(IOController_i::IOBadParam& ex )
                    {
                        mblog3 << myname << "(updateSM):(IOBadParam) " << ex.err << endl;
                    }
                    catch( IONotifyController_i::BadRange& ex )
                    {
                        mblog3 << myname << "(updateSM): (BadRange)..." << endl;
                    }
                    catch( const CORBA::SystemException& ex )
                    {
                        mblog3 << myname << "(updateSM): CORBA::SystemException: "
                               << ex.NP_minorString() << endl;
                    }
                    catch( const uniset::Exception& ex )
                    {
                        mblog3 << myname << "(updateSM): " << ex << endl;
                    }
                    catch( std::exception& ex )
                    {
                        mblog3 << myname << "(updateSM): check safemodeSensor: " << ex.what() << endl;
                    }
                }
            }

            for( auto&& m : d->pollmap )
            {
                auto& regmap = m.second;

                // обновление датчиков связи происходит в другом потоке
                // чтобы не зависеть от TCP таймаутов
                // см. updateRespondSensors()

                // update values...
                for( auto it = regmap->begin(); it != regmap->end(); ++it )
                {
                    try
                    {
                        if( d->dtype == MBConfig::dtRTU )
                            updateRTU(it);
                        else if( d->dtype == MBConfig::dtMTR )
                            updateMTR(it);
                        else if( d->dtype == MBConfig::dtRTU188 )
                            updateRTU188(it);
                    }
                    catch(IOController_i::NameNotFound& ex)
                    {
                        mblog3 << myname << "(updateSM):(NameNotFound) " << ex.err << endl;
                    }
                    catch(IOController_i::IOBadParam& ex )
                    {
                        mblog3 << myname << "(updateSM):(IOBadParam) " << ex.err << endl;
                    }
                    catch(IONotifyController_i::BadRange& ex )
                    {
                        mblog3 << myname << "(updateSM): (BadRange)..." << endl;
                    }
                    catch( const CORBA::SystemException& ex )
                    {
                        mblog3 << myname << "(updateSM): CORBA::SystemException: "
                               << ex.NP_minorString() << endl;
                    }
                    catch( const uniset::Exception& ex )
                    {
                        mblog3 << myname << "(updateSM): " << ex << endl;
                    }
                    catch( const std::exception& ex )
                    {
                        mblog3 << myname << "(updateSM): catch ..." << endl;
                    }

                    // эта проверка
                    if( it == regmap->end() )
                        break;
                }
            }
        }
    }

    // -----------------------------------------------------------------------------
    void MBExchange::updateRTU( MBConfig::RegMap::iterator& rit )
    {
        auto& r = rit->second;

        for( auto&& it : r->slst )
            updateRSProperty( &it, false );

    }
    // -----------------------------------------------------------------------------
    void MBExchange::updateRSProperty( MBConfig::RSProperty* p, bool write_only )
    {
        using namespace ModbusRTU;
        auto& r = p->reg->rit->second;

        bool save = isWriteFunction( r->mbfunc );

        if( !save && write_only )
            return;

        if( !isUpdateSM(save, r->dev->mode) )
            return;

        // если ещё не обменивались ни разу с устройством, то ингнорируем (не обновляем значение в SM)
        if( !r->mb_initOK && !isSafeMode(r->dev) )
            return;

        bool useSafeval = isSafeMode(r->dev) && p->safevalDefined;

        mblog3 << myname << "(updateP): sid=" << p->si.id
               << " mbval=" << r->mbval
               << " vtype=" << p->vType
               << " rnum=" << p->rnum
               << " nbit=" << (int)p->nbit
               << " save=" << save
               << " ioype=" << p->stype
               << " mb_initOK=" << r->mb_initOK
               << " sm_initOK=" << r->sm_initOK
               << " safeMode=" << isSafeMode(r->dev)
               << " safevalDefined=" << p->safevalDefined
               << endl;

        try
        {
            if( p->vType == VTypes::vtUnknown )
            {
                ModbusRTU::DataBits16 b(r->mbval);

                if( p->nbit >= 0 )
                {
                    if( save )
                    {
                        if( r->mb_initOK )
                        {
                            bool set = useSafeval ? p->safeval : IOBase::processingAsDO( p, shm, force_out );
                            b.set(p->nbit, set);
                            r->mbval = b.mdata();
                            r->sm_initOK = true;
                        }
                    }
                    else
                    {
                        bool set = useSafeval ? (bool)p->safeval : b[p->nbit];
                        IOBase::processingAsDI( p, set, shm, force );
                    }

                    return;
                }

                if( p->rnum <= 1 )
                {
                    if( save )
                    {
                        if(  r->mb_initOK )
                        {
                            if( useSafeval )
                                r->mbval = p->safeval;
                            else
                            {
                                if( p->stype == UniversalIO::DI || p->stype == UniversalIO::DO )
                                    r->mbval = IOBase::processingAsDO( p, shm, force_out );
                                else
                                    r->mbval = IOBase::processingAsAO( p, shm, force_out );
                            }

                            r->sm_initOK = true;
                        }
                    }
                    else
                    {
                        if( p->stype == UniversalIO::DI ||
                                p->stype == UniversalIO::DO )
                        {
                            if( useSafeval )
                                IOBase::processingAsDI( p, p->safeval, shm, force );
                            else
                                IOBase::processingAsDI( p, r->mbval, shm, force );
                        }
                        else
                        {
                            if( useSafeval )
                                IOBase::processingAsAI( p, p->safeval, shm, force );
                            else
                                IOBase::processingAsAI( p, (signed short)(r->mbval), shm, force );
                        }
                    }

                    return;
                }

                mbcrit << myname << "(updateRSProperty): IGNORE item: rnum=" << p->rnum
                       << " > 1 ?!! for id=" << p->si.id << endl;
                return;
            }
            else if( p->vType == VTypes::vtSigned )
            {
                if( save )
                {
                    if( r->mb_initOK )
                    {
                        if( useSafeval )
                            r->mbval = p->safeval;
                        else
                        {
                            if( p->stype == UniversalIO::DI || p->stype == UniversalIO::DO )
                                r->mbval = (int16_t)IOBase::processingAsDO( p, shm, force_out );
                            else
                                r->mbval = (int16_t)IOBase::processingAsAO( p, shm, force_out );
                        }

                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( p->stype == UniversalIO::DI || p->stype == UniversalIO::DO )
                    {
                        if( useSafeval )
                            IOBase::processingAsDI( p, p->safeval, shm, force );
                        else
                            IOBase::processingAsDI( p, r->mbval, shm, force );
                    }
                    else
                    {
                        if( useSafeval )
                            IOBase::processingAsAI( p, p->safeval, shm, force );
                        else
                            IOBase::processingAsAI( p, (signed short)(r->mbval), shm, force );
                    }
                }

                return;
            }
            else if( p->vType == VTypes::vtUnsigned )
            {
                if( save )
                {
                    if( r->mb_initOK )
                    {
                        if( useSafeval )
                            r->mbval = p->safeval;
                        else
                        {
                            if( p->stype == UniversalIO::DI || p->stype == UniversalIO::DO )
                                r->mbval = (uint16_t)IOBase::processingAsDO( p, shm, force_out );
                            else
                                r->mbval = (uint16_t)IOBase::processingAsAO( p, shm, force_out );
                        }

                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( p->stype == UniversalIO::DI || p->stype == UniversalIO::DO )
                        IOBase::processingAsDI( p, useSafeval ? p->safeval : r->mbval, shm, force );
                    else
                        IOBase::processingAsAI( p, useSafeval ? p->safeval : (uint16_t)r->mbval, shm, force );
                }

                return;
            }
            else if( p->vType == VTypes::vtByte )
            {
                if( p->nbyte <= 0 || p->nbyte > VTypes::Byte::bsize )
                {
                    mbcrit << myname << "(updateRSProperty): IGNORE item: reg=" << ModbusRTU::dat2str(r->mbreg)
                           << " vtype=" << p->vType << " but nbyte=" << p->nbyte << endl;
                    return;
                }

                if( save )
                {
                    if( r->mb_initOK )
                    {
                        long v = useSafeval ? p->safeval : IOBase::processingAsAO( p, shm, force_out );
                        VTypes::Byte b(r->mbval);
                        b.raw.b[p->nbyte - 1] = v;
                        r->mbval = b.raw.w;
                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( useSafeval )
                        IOBase::processingAsAI( p, p->safeval, shm, force );
                    else
                    {
                        VTypes::Byte b(r->mbval);
                        IOBase::processingAsAI( p, b.raw.b[p->nbyte - 1], shm, force );
                    }
                }

                return;
            }
            else if( p->vType == VTypes::vtF2 || p->vType == VTypes::vtF2r )
            {
                auto i = p->reg->rit;

                if( save )
                {
                    if( r->mb_initOK )
                    {
                        //! \warning НЕ РЕШЕНО safeval --> float !
                        float f = useSafeval ? (float)p->safeval : IOBase::processingFasAO( p, shm, force_out );

                        if( p->vType == VTypes::vtF2 )
                        {
                            VTypes::F2 f2(f);

                            for( size_t k = 0; k < VTypes::F2::wsize(); k++, i++ )
                                i->second->mbval = f2.raw.v[k];
                        }
                        else if( p->vType == VTypes::vtF2r )
                        {
                            VTypes::F2r f2(f);

                            for( size_t k = 0; k < VTypes::F2r::wsize(); k++, i++ )
                                i->second->mbval = f2.raw.v[k];
                        }

                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( useSafeval )
                    {
                        IOBase::processingAsAI( p, p->safeval, shm, force );
                    }
                    else
                    {
                        DataGuard d(VTypes::F2::wsize());

                        for( size_t k = 0; k < VTypes::F2::wsize(); k++, i++ )
                            d.data[k] = i->second->mbval;

                        float f = 0;

                        if( p->vType == VTypes::vtF2 )
                        {
                            VTypes::F2 f1(d.data, d.len);
                            f = (float)f1;
                        }
                        else if( p->vType == VTypes::vtF2r )
                        {
                            VTypes::F2r f1(d.data, VTypes::F2r::wsize());
                            f = (float)f1;
                        }

                        IOBase::processingFasAI( p, f, shm, force );
                    }
                }
            }
            else if( p->vType == VTypes::vtF4 )
            {
                auto i = p->reg->rit;

                if( save )
                {
                    if( r->mb_initOK )
                    {
                        //! \warning НЕ РЕШЕНО safeval --> float !
                        float f = useSafeval ? (float)p->safeval : IOBase::processingFasAO( p, shm, force_out );
                        VTypes::F4 f4(f);

                        for( size_t k = 0; k < VTypes::F4::wsize(); k++, i++ )
                            i->second->mbval = f4.raw.v[k];

                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( useSafeval )
                    {
                        IOBase::processingAsAI( p, p->safeval, shm, force );
                    }
                    else
                    {
                        DataGuard d(VTypes::F4::wsize());

                        for( size_t k = 0; k < VTypes::F4::wsize(); k++, i++ )
                            d.data[k] = i->second->mbval;

                        VTypes::F4 f(d.data, d.len);

                        IOBase::processingF64asAI( p, (double)f, shm, force );
                    }
                }
            }
            else if( p->vType == VTypes::vtI2 || p->vType == VTypes::vtI2r )
            {
                auto i = p->reg->rit;

                if( save )
                {
                    if( r->mb_initOK )
                    {
                        long v = useSafeval ? p->safeval : IOBase::processingAsAO( p, shm, force_out );

                        if( p->vType == VTypes::vtI2 )
                        {
                            VTypes::I2 i2( (int32_t)v );

                            for( size_t k = 0; k < VTypes::I2::wsize(); k++, i++ )
                                i->second->mbval = i2.raw.v[k];
                        }
                        else if( p->vType == VTypes::vtI2r )
                        {
                            VTypes::I2r i2( (int32_t)v );

                            for( size_t k = 0; k < VTypes::I2::wsize(); k++, i++ )
                                i->second->mbval = i2.raw.v[k];
                        }

                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( useSafeval )
                    {
                        IOBase::processingAsAI( p, p->safeval, shm, force );
                    }
                    else
                    {
                        DataGuard d(VTypes::I2::wsize());

                        for( size_t k = 0; k < VTypes::I2::wsize(); k++, i++ )
                            d.data[k] = i->second->mbval;

                        int v = 0;

                        if( p->vType == VTypes::vtI2 )
                        {
                            VTypes::I2 i2(d.data, d.len);
                            v = (int)i2;
                        }
                        else if( p->vType == VTypes::vtI2r )
                        {
                            VTypes::I2r i2(d.data, d.len);
                            v = (int)i2;
                        }

                        IOBase::processingAsAI( p, v, shm, force );
                    }
                }
            }
            else if( p->vType == VTypes::vtU2 || p->vType == VTypes::vtU2r ) // -V560
            {
                auto i = p->reg->rit;

                if( save )
                {
                    if( r->mb_initOK )
                    {
                        long v = useSafeval ? p->safeval : IOBase::processingAsAO( p, shm, force_out );

                        if( p->vType == VTypes::vtU2 )
                        {
                            VTypes::U2 u2(v);

                            for( size_t k = 0; k < VTypes::U2::wsize(); k++, i++ )
                                i->second->mbval = u2.raw.v[k];
                        }
                        else if( p->vType == VTypes::vtU2r )
                        {
                            VTypes::U2r u2(v);

                            for( size_t k = 0; k < VTypes::U2::wsize(); k++, i++ )
                                i->second->mbval = u2.raw.v[k];
                        }

                        r->sm_initOK = true;
                    }
                }
                else
                {
                    if( useSafeval )
                    {
                        IOBase::processingAsAI( p, p->safeval, shm, force );
                    }
                    else
                    {

                        DataGuard d(VTypes::U2::wsize());

                        for( size_t k = 0; k < VTypes::U2::wsize(); k++, i++ )
                            d.data[k] = i->second->mbval;

                        uint32_t v = 0;

                        if( p->vType == VTypes::vtU2 )
                        {
                            VTypes::U2 u2(d.data, d.len);
                            v = (uint32_t)u2;
                        }
                        else if( p->vType == VTypes::vtU2r )
                        {
                            VTypes::U2r u2(d.data, d.len);
                            v = (uint32_t)u2;
                        }

                        IOBase::processingAsAI( p, v, shm, force );
                    }
                }
            }

            return;
        }
        catch(IOController_i::NameNotFound& ex)
        {
            mblog3 << myname << "(updateRSProperty):(NameNotFound) " << ex.err << endl;
        }
        catch(IOController_i::IOBadParam& ex )
        {
            mblog3 << myname << "(updateRSProperty):(IOBadParam) " << ex.err << endl;
        }
        catch(IONotifyController_i::BadRange& ex )
        {
            mblog3 << myname << "(updateRSProperty): (BadRange)..." << endl;
        }
        catch( const CORBA::SystemException& ex )
        {
            mblog3 << myname << "(updateRSProperty): CORBA::SystemException: "
                   << ex.NP_minorString() << endl;
        }
        catch( const uniset::Exception& ex )
        {
            mblog3 << myname << "(updateRSProperty): " << ex << endl;
        }
        catch(...)
        {
            mblog3 << myname << "(updateRSProperty): catch ..." << endl;
        }

        // Если SM стала (или была) недоступна
        // то флаг инициализации надо сбросить
        r->sm_initOK = false;
    }
    // -----------------------------------------------------------------------------
    void MBExchange::updateMTR( MBConfig::RegMap::iterator& rit )
    {
        auto& r = rit->second;

        if( !r || !r->dev )
            return;

        using namespace ModbusRTU;
        bool save = isWriteFunction( r->mbfunc );

        if( !isUpdateSM(save, r->dev->mode) )
            return;

        for( auto it = r->slst.begin(); it != r->slst.end(); ++it )
        {
            try
            {
                bool safeMode = isSafeMode(r->dev) && it->safevalDefined;

                if( r->mtrType == MTR::mtT1 )
                {
                    if( save )
                        r->mbval = safeMode ? it->safeval : IOBase::processingAsAO( &(*it), shm, force_out );
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            MTR::T1 t(r->mbval);
                            IOBase::processingAsAI( &(*it), t.val, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT2 )
                {
                    if( save )
                    {
                        long val = safeMode ? it->safeval : IOBase::processingAsAO( &(*it), shm, force_out );
                        MTR::T2 t(val);
                        r->mbval = t.val;
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            MTR::T2 t(r->mbval);
                            IOBase::processingAsAI( &(*it), t.val, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT3 )
                {
                    auto i = rit;

                    if( save )
                    {
                        long val = safeMode ? it->safeval : IOBase::processingAsAO( &(*it), shm, force_out );

                        MTR::T3 t(val);

                        for( size_t k = 0; k < MTR::T3::wsize(); k++, i++ )
                            i->second->mbval = t.raw.v[k];
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            DataGuard d(MTR::T3::wsize());

                            for( size_t k = 0; k < MTR::T3::wsize(); k++, i++ )
                                d.data[k] = i->second->mbval;

                            MTR::T3 t(d.data, d.len);
                            IOBase::processingAsAI( &(*it), (long)t, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT4 )
                {
                    if( save )
                    {
                        mbwarn << myname << "(updateMTR): write (T4) reg(" << dat2str(r->mbreg) << ") to MTR NOT YET!!!" << endl;
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            MTR::T4 t(r->mbval);
                            IOBase::processingAsAI( &(*it), uni_atoi(t.sval), shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT5 )
                {
                    auto i = rit;

                    if( save )
                    {
                        long val = safeMode ? it->safeval : IOBase::processingAsAO( &(*it), shm, force_out );

                        MTR::T5 t(val);

                        for( size_t k = 0; k < MTR::T5::wsize(); k++, i++ )
                            i->second->mbval = t.raw.v[k];
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            DataGuard d(MTR::T5::wsize());

                            for( size_t k = 0; k < MTR::T5::wsize(); k++, i++ )
                                d.data[k] = i->second->mbval;

                            MTR::T5 t(d.data, d.len);

                            IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT6 )
                {
                    auto i = rit;

                    if( save )
                    {
                        long val = safeMode ? it->safeval : IOBase::processingAsAO( &(*it), shm, force_out );

                        MTR::T6 t(val);

                        for( size_t k = 0; k < MTR::T6::wsize(); k++, i++ )
                            i->second->mbval = t.raw.v[k];
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            DataGuard d(MTR::T6::wsize());

                            for( size_t k = 0; k < MTR::T6::wsize(); k++, i++ )
                                d.data[k] = i->second->mbval;

                            MTR::T6 t(d.data, d.len);

                            IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT7 )
                {
                    auto i = rit;

                    if( save )
                    {
                        long val = safeMode ? it->safeval : IOBase::processingAsAO( &(*it), shm, force_out );
                        MTR::T7 t(val);

                        for( size_t k = 0; k < MTR::T7::wsize(); k++, i++ )
                            i->second->mbval = t.raw.v[k];
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            DataGuard d(MTR::T7::wsize());

                            for( size_t k = 0; k < MTR::T7::wsize(); k++, i++ )
                                d.data[k] = i->second->mbval;

                            MTR::T7 t(d.data, d.len);

                            IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT16 )
                {
                    if( save )
                    {
                        //! \warning НЕ РЕШЕНО safeval --> float !
                        float fval = safeMode ? (float)it->safeval : IOBase::processingFasAO( &(*it), shm, force_out );

                        MTR::T16 t(fval);
                        r->mbval = t.val;
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            MTR::T16 t(r->mbval);
                            IOBase::processingFasAI( &(*it), t.fval, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtT17 )
                {
                    if( save )
                    {
                        //! \warning НЕ РЕШЕНО safeval --> float !
                        float fval = safeMode ? (float)it->safeval : IOBase::processingFasAO( &(*it), shm, force_out );

                        MTR::T17 t(fval);
                        r->mbval = t.val;
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            MTR::T17 t(r->mbval);
                            IOBase::processingFasAI( &(*it), t.fval, shm, force );
                        }
                    }

                    continue;
                }

                if( r->mtrType == MTR::mtF1 )
                {
                    auto i = rit;

                    if( save )
                    {
                        //! \warning НЕ РЕШЕНО safeval --> float !
                        float f = safeMode ? (float)it->safeval : IOBase::processingFasAO( &(*it), shm, force_out );
                        MTR::F1 f1(f);

                        for( size_t k = 0; k < MTR::F1::wsize(); k++, i++ )
                            i->second->mbval = f1.raw.v[k];
                    }
                    else
                    {
                        if( safeMode )
                        {
                            IOBase::processingAsAI( &(*it), it->safeval, shm, force );
                        }
                        else
                        {
                            DataGuard d(MTR::F1::wsize());

                            for( size_t k = 0; k < MTR::F1::wsize(); k++, i++ )
                                d.data[k] = i->second->mbval;

                            MTR::F1 t(d.data, d.len);
                            IOBase::processingFasAI( &(*it), (float)t, shm, force );
                        }
                    }

                    continue;
                }
            }
            catch(IOController_i::NameNotFound& ex)
            {
                mblog3 << myname << "(updateMTR):(NameNotFound) " << ex.err << endl;
            }
            catch(IOController_i::IOBadParam& ex )
            {
                mblog3 << myname << "(updateMTR):(IOBadParam) " << ex.err << endl;
            }
            catch(IONotifyController_i::BadRange& ex )
            {
                mblog3 << myname << "(updateMTR): (BadRange)..." << endl;
            }
            catch( const CORBA::SystemException& ex )
            {
                mblog3 << myname << "(updateMTR): CORBA::SystemException: "
                       << ex.NP_minorString() << endl;
            }
            catch( const uniset::Exception& ex )
            {
                mblog3 << myname << "(updateMTR): " << ex << endl;
            }
            catch(...)
            {
                mblog3 << myname << "(updateMTR): catch ..." << endl;
            }
        }
    }
    // -----------------------------------------------------------------------------
    void MBExchange::updateRTU188( MBConfig::RegMap::iterator& rit )
    {
        auto& r = rit->second;

        if( !r || !r->dev || !r->dev->rtu188 )
            return;

        using namespace ModbusRTU;

        bool save = isWriteFunction( r->mbfunc );

        // пока-что функции записи в обмене с RTU188
        // не реализованы
        if( isWriteFunction(r->mbfunc) )
        {
            mblog3 << myname << "(updateRTU188): write reg(" << dat2str(r->mbreg) << ") to RTU188 NOT YET!!!" << endl;
            return;
        }

        if( !isUpdateSM(save, r->dev->mode) )
            return;

        for( auto it = r->slst.begin(); it != r->slst.end(); ++it )
        {
            try
            {
                bool safeMode = isSafeMode(r->dev) && it->safevalDefined;

                if( it->stype == UniversalIO::DI )
                {
                    bool set = safeMode ? (bool)it->safeval : r->dev->rtu188->getState(r->rtuJack, r->rtuChan, it->stype);
                    IOBase::processingAsDI( &(*it), set, shm, force );
                }
                else if( it->stype == UniversalIO::AI )
                {
                    long val = safeMode ? it->safeval : r->dev->rtu188->getInt(r->rtuJack, r->rtuChan, it->stype);
                    IOBase::processingAsAI( &(*it), val, shm, force );
                }
            }
            catch(IOController_i::NameNotFound& ex)
            {
                mblog3 << myname << "(updateRTU188):(NameNotFound) " << ex.err << endl;
            }
            catch(IOController_i::IOBadParam& ex )
            {
                mblog3 << myname << "(updateRTU188):(IOBadParam) " << ex.err << endl;
            }
            catch(IONotifyController_i::BadRange& ex )
            {
                mblog3 << myname << "(updateRTU188): (BadRange)..." << endl;
            }
            catch( const CORBA::SystemException& ex )
            {
                mblog3 << myname << "(updateRTU188): CORBA::SystemException: "
                       << ex.NP_minorString() << endl;
            }
            catch( const uniset::Exception& ex )
            {
                mblog3 << myname << "(updateRTU188): " << ex << endl;
            }
            catch(...)
            {
                mblog3 << myname << "(updateRTU188): catch ..." << endl;
            }
        }
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::activateObject()
    {
        // блокирование обработки Starsp
        // пока не пройдёт инициализация датчиков
        // см. sysCommand()
        {
            setProcActive(false);
            uniset::uniset_rwmutex_rlock l(mutex_start);
            UniSetObject::activateObject();

            if( !shm->isLocalwork() && !mbconf->noQueryOptimization )
                MBConfig::rtuQueryOptimization(mbconf->devices, mbconf->maxQueryCount);

            initIterators();
            setProcActive(true);
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    void MBExchange::sysCommand( const uniset::SystemMessage* sm )
    {
        switch( sm->command )
        {
            case SystemMessage::StartUp:
            {
                if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
                {
                    mbinfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
                    logserv->async_run(logserv_host, logserv_port);
                }

                if( mbconf->devices.empty() )
                {
                    mbcrit << myname << "(sysCommand): ************* ITEM MAP EMPTY! terminated... *************" << endl << flush;
                    //                  std::terminate();
                    uterminate();
                    return;
                }

                mbinfo << myname << "(sysCommand): device map size= " << mbconf->devices.size() << endl;

                if( !shm->isLocalwork() )
                    mbconf->initDeviceList(uniset_conf()->getConfXML());

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

                while( !isProcActive() && !ptAct.checkTime() )
                {
                    cout << myname << "(sysCommand): wait activate..." << endl;
                    msleep(300);

                    if( isProcActive() )
                        break;
                }

                if( !isProcActive() )
                {
                    mbcrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl << flush;
                    uterminate();
                    return;
                }

                {
                    uniset::uniset_rwmutex_rlock l(mutex_start);
                    askSensors(UniversalIO::UIONotify);
                    initOutput();
                }

                initValues();
                updateSM();
                askTimer(tmExchange, mbconf->polltime);
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
                // (т.е. MBExchange  запущен в одном процессе с SharedMemory2)
                // то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
                // при заказе датчиков, а если SM вылетит, то вместе с этим процессом(MBExchange)
                if( shm->isLocalwork() )
                    break;

                askSensors(UniversalIO::UIONotify);
                initOutput();
                initValues();

                if( !force )
                {
                    force = true;
                    poll();
                    force = false;
                }
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

            case SystemMessage::ReConfiguration:
            {
                mblogany << myname << "(sysCommand): reconfig" << std::endl;
                reload(uniset_conf()->getConfFileName());
            }
            break;

            default:
                break;
        }
    }
    // ------------------------------------------------------------------------------------------
    bool MBExchange::reconfigure( const std::shared_ptr<uniset::UniXML>& xml, const std::shared_ptr<uniset::MBConfig>& mbconf )
    {
        return true;
    }
    // ----------------------------------------------------------------------------
    bool MBExchange::reload( const std::string& confile )
    {
        mbinfo << myname << ": reload from " << confile << endl;

        uniset::uniset_rwmutex_wrlock lock(mutex_conf);

        auto oldConf = mbconf;

        std::shared_ptr<MBConfig> newConf;

        try
        {
            auto xml = make_shared<UniXML>(confile);

            string conf_name(uniset_conf()->getArgParam("--" + mbconf->prefix + "-confnode", myname));
            xmlNode* newCnode = xml->findNode(xml->getFirstNode(), conf_name);

            if( newCnode == NULL )
            {
                mbcrit << myname << "(reload): reload config from '" << confile
                       << "' FAILED: not found conf node '" << conf_name
                       << "'" << endl;
                return false;
            }

            newConf = make_shared<MBConfig>(mbconf->conf, newCnode, mbconf->shm);
            newConf->cloneParams(mbconf);
            newConf->loadConfig(xml, nullptr);

            if( newConf->devices.empty() )
            {
                mbcrit << myname << "(reload): reload config from '" << confile << "' FAILED: empty devices list" << std::endl;
                return false;
            }

            if( !reconfigure(xml, newConf) )
                return false;

            mbconf = newConf;

            if( !shm->isLocalwork() )
                initIterators();

            allInitOK = false;
            initOutput();
            initValues();
            updateSM();
            ptInitChannel.reset();
            ptReopen.reset();
            return true;
        }
        catch( std::exception& ex )
        {
            mbcrit << myname << "(reload): reload config from '" << confile << "' FAILED: " << ex.what() << std::endl;
        }

        mbconf = oldConf;
        return false;
    }
    // ------------------------------------------------------------------------------------------
    void MBExchange::initOutput()
    {
    }
    // ------------------------------------------------------------------------------------------
    void MBExchange::askSensors( UniversalIO::UIOCommand cmd )
    {
        if( !shm->waitSMworking(test_id, activateTimeout, 50) )
        {
            ostringstream err;
            err << myname
                << "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение "
                << activateTimeout << " мсек";

            mbcrit << err.str() << endl << flush;
            //          std::terminate();  // прерываем (перезапускаем) процесс...
            uterminate();
            return;
        }

        try
        {
            if( sidExchangeMode != DefaultObjectId )
                shm->askSensor(sidExchangeMode, cmd);
        }
        catch( uniset::Exception& ex )
        {
            mbwarn << myname << "(askSensors): " << ex << std::endl;
        }
        catch(...)
        {
            mbwarn << myname << "(askSensors): 'sidExchangeMode' catch..." << std::endl;
        }

        for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
        {
            auto d = it1->second;

            try
            {
                if( d->mode_id != DefaultObjectId )
                    shm->askSensor(d->mode_id, cmd);

                if( d->safemode_id != DefaultObjectId )
                    shm->askSensor(d->safemode_id, cmd);
            }
            catch( uniset::Exception& ex )
            {
                mbwarn << myname << "(askSensors): " << ex << std::endl;
            }
            catch(...)
            {
                mbwarn << myname << "(askSensors): "
                       << "("
                       << " mode_id=" << d->mode_id
                       << " safemode_id=" << d->safemode_id
                       << ").. catch..." << std::endl;
            }

            if( force_out )
                return;

            for( auto&& m : d->pollmap )
            {
                auto& regmap = m.second;

                for( auto it = regmap->begin(); it != regmap->end(); ++it )
                {
                    if( !isWriteFunction(it->second->mbfunc) )
                        continue;

                    for( auto i = it->second->slst.begin(); i != it->second->slst.end(); ++i )
                    {
                        try
                        {
                            shm->askSensor(i->si.id, cmd);
                        }
                        catch( uniset::Exception& ex )
                        {
                            mbwarn << myname << "(askSensors): " << ex << std::endl;
                        }
                        catch(...)
                        {
                            mbwarn << myname << "(askSensors): id=" << i->si.id << " catch..." << std::endl;
                        }
                    }
                }
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    void MBExchange::sensorInfo( const uniset::SensorMessage* sm )
    {
        if( sm->id == sidExchangeMode )
        {
            exchangeMode = sm->value;
            mblog3 << myname << "(sensorInfo): exchange MODE=" << sm->value << std::endl;
            //return; // этот датчик может встречаться и в списке обмена.. поэтому делать return нельзя.
        }

        for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
        {
            auto& d(it1->second);

            if( sm->id == d->mode_id )
                d->mode = sm->value;

            if( sm->id == d->safemode_id )
            {
                if( sm->value == d->safemode_value )
                    d->safeMode = MBConfig::safeExternalControl;
                else
                    d->safeMode = MBConfig::safeNone;
            }

            if( force_out )
                continue;

            for( const auto& m : d->pollmap )
            {
                auto&& regmap = m.second;

                for( const auto& it : (*regmap) )
                {
                    if( !isWriteFunction(it.second->mbfunc) )
                        continue;

                    for( auto&& i : it.second->slst )
                    {
                        if( sm->id == i.si.id && sm->node == i.si.node )
                        {
                            mbinfo << myname << "(sensorInfo): si.id=" << sm->id
                                   << " reg=" << ModbusRTU::dat2str(i.reg->mbreg)
                                   << " val=" << sm->value
                                   << " mb_initOK=" << i.reg->mb_initOK << endl;

                            if( !i.reg->mb_initOK )
                                continue;

                            i.value = sm->value;
                            updateRSProperty( &i, true );
                            return;
                        }
                    }
                }
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    void MBExchange::timerInfo( const TimerMessage* tm )
    {
        if( tm->id == tmExchange )
        {
            if( notUseExchangeTimer )
                askTimer(tm->id, 0);
            else
                step();
        }
    }
    // -----------------------------------------------------------------------------
    bool MBExchange::poll()
    {
        uniset::uniset_rwmutex_rlock lock(mutex_conf);

        if( !mb )
        {
            mb = initMB(false);

            if( !isProcActive() )
                return false;

            updateSM();
            allInitOK = false;

            if( !mb )
                return false;

            for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
                it1->second->resp_ptInit.reset();
        }

        if( !allInitOK )
            firstInitRegisters();

        if( !isProcActive() )
            return false;

        ncycle++;
        bool allNotRespond = true;

        for( auto it1 = mbconf->devices.begin(); it1 != mbconf->devices.end(); ++it1 )
        {
            auto d(it1->second);

            if( d->mode_id != DefaultObjectId && d->mode == MBConfig::emSkipExchange )
                continue;

            mblog3 << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr)
                   << " regs=" << d->pollmap.size() << endl;

            d->prev_numreply.store(d->numreply);

            for( auto&& m : d->pollmap )
            {
                if( m.first > 1 && (ncycle % m.first) != 0 )
                    continue;

                auto&& regmap = m.second;

                for( auto it = regmap->begin(); it != regmap->end(); ++it )
                {
                    if( !isProcActive() )
                        return false;

                    if( exchangeMode == MBConfig::emSkipExchange )
                        continue;

                    try
                    {
                        if( d->dtype == MBConfig::dtRTU || d->dtype == MBConfig::dtMTR )
                        {
                            if( pollRTU(d, it) )
                            {
                                d->numreply++;
                                allNotRespond = false;
                            }
                        }
                    }
                    catch( ModbusRTU::mbException& ex )
                    {
                        mblog3 << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
                               << " reg=" << ModbusRTU::dat2str(it->second->mbreg)
                               << " for sensors: ";
                        mbconf->print_plist( (*mblog)(Debug::LEVEL3), it->second->slst)
                                << endl << " err: " << ex << endl;

                        if( ex.err == ModbusRTU::erTimeOut && !d->ask_every_reg )
                            break;
                    }

                    if( it == regmap->end() )
                        break;

                    if( !isProcActive() )
                        return false;
                }
            }

            if( stat_time > 0 )
                poll_count++;
        }

        if( stat_time > 0 && ptStatistic.checkTime() )
        {
            ostringstream s;
            s << "number of calls is " << poll_count << " (poll time: " << stat_time << " sec)";
            statInfo = s.str();
            mblog9 << myname << "(stat): " << statInfo << endl;
            ptStatistic.reset();
            poll_count = 0;
        }

        if( !isProcActive() )
            return false;

        // update SharedMemory...
        updateSM();

        // check thresholds
        for( auto t = mbconf->thrlist.begin(); t != mbconf->thrlist.end(); ++t )
        {
            if( !isProcActive() )
                return false;

            IOBase::processingThreshold(&(*t), shm, force);
        }

        if( trReopen.hi(allNotRespond && exchangeMode != MBConfig::emSkipExchange) )
            ptReopen.reset();

        if( allNotRespond && exchangeMode != MBConfig::emSkipExchange && ptReopen.checkTime() )
        {
            mbwarn << myname << ": REOPEN timeout..(" << ptReopen.getInterval() << ")" << endl;

            mb = initMB(true);
            ptReopen.reset();
        }

        return !allNotRespond;
    }
    // -----------------------------------------------------------------------------
    void MBExchange::updateRespondSensors()
    {
        uniset::uniset_rwmutex_rlock lock(mutex_conf);

        for( const auto& it1 : mbconf->devices )
        {
            auto d(it1.second);

            if( d->checkRespond(mblog) && d->resp_id != DefaultObjectId )
            {
                try
                {
                    bool set = d->resp_invert ? !d->resp_state : d->resp_state;

                    mblog4 << myname << ": SAVE NEW " << (d->resp_invert ? "NOT" : "") << " respond state=" << set
                           << " for addr=" << ModbusRTU::addr2str(d->mbaddr)
                           << " respond_id=" << d->resp_id
                           << " state=" << d->resp_state
                           << " [ invert=" << d->resp_invert
                           << " timeout=" << d->resp_Delay.getOffDelay()
                           << " numreply=" << d->numreply
                           << " prev_numreply=" << d->prev_numreply
                           << " ]"
                           << endl;

                    shm->localSetValue(d->resp_it, d->resp_id, ( set ? 1 : 0 ), getId());
                }
                catch( const uniset::Exception& ex )
                {
                    mbcrit << myname << "(step): (respond) " << ex << std::endl;
                }
            }
        }
    }
    // -----------------------------------------------------------------------------
    void MBExchange::execute()
    {
        notUseExchangeTimer = true;

        try
        {
            askTimer(tmExchange, 0);
        }
        catch( const std::exception& ex )
        {
        }

        initMB(false);

        while(1)
        {
            try
            {
                step();
            }
            catch( const uniset::Exception& ex )
            {
                mbcrit << myname << "(execute): " << ex << std::endl;
            }
            catch( const std::exception& ex )
            {
                mbcrit << myname << "(execute): catch: " << ex.what() << endl;
            }

            msleep(mbconf->polltime);
        }
    }
    // -----------------------------------------------------------------------------
    uniset::SimpleInfo* MBExchange::getInfo( const char* userparam )
    {
        uniset::SimpleInfo_var i = UniSetObject::getInfo(userparam);

        ostringstream inf;

        inf << i->info << endl;
        inf << vmon.pretty_str() << endl;
        inf << "activated: " << activated << endl;
        inf << "LogServer:  " << logserv_host << ":" << logserv_port << endl;
        inf << "Parameters: reopenTimeout=" << ptReopen.getInterval()
            << mbconf->getShortInfo()
            << endl;

        if( stat_time > 0 )
            inf << "Statistics: " << statInfo << endl;

        inf << "Devices: " << endl;

        for( const auto& it : mbconf->devices )
            inf << "  " <<  it.second->getShortInfo() << endl;

        i->info = inf.str().c_str();
        return i._retn();
    }
    // ----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr MBExchange::httpHelp( const Poco::URI::QueryParameters& p )
    {
        uniset::json::help::object myhelp(myname, UniSetObject::httpHelp(p));

        {
            // 'reload'
            uniset::json::help::item cmd("reload", "reload config from file");
            cmd.param("confile", "Absolute path for config file. Optional parameter.");
            myhelp.add(cmd);
        }

        return myhelp;

    }
    // ----------------------------------------------------------------------------
    Poco::JSON::Object::Ptr MBExchange::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
    {
        mbinfo << myname << "(httpRequest): " << req << endl;

        if( req == "reload" )
        {
            std::string confile = uniset_conf()->getConfFileName();

            if( p.size() > 0 && p[0].first == "confile" )
            {
                confile = p[0].second;

                if( !uniset::file_exist(confile) )
                {
                    ostringstream err;
                    err << myname << "(reload): Not found config file '" << confile << "'";
                    throw uniset::SystemError(err.str());
                }
            }

            if( reload(confile) )
            {
                Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
                json->set("result", "OK");
                json->set("config", confile);
                return json;
            }

            ostringstream err;
            err << myname << "(reload): Failed reload from '" << confile << "'";
            throw uniset::SystemError(err.str());
        }

        return UniSetObject::httpRequest(req, p);
    }
    // ----------------------------------------------------------------------------
#endif

} // end of namespace uniset
