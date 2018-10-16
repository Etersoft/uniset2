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
	// вспомогательная структура для предотвращения утечки памяти
	struct DataGuard
	{
		DataGuard( size_t sz )
		{
			data = new ModbusRTU::ModbusData[sz];
		}

		~DataGuard()
		{
			delete[] data;
		}

		ModbusRTU::ModbusData* data;
	};
	// -----------------------------------------------------------------------------
	MBExchange::MBExchange(uniset::ObjectId objId, uniset::ObjectId shmId,
						   const std::shared_ptr<SharedMemory>& _ic, const std::string& prefix ):
		UniSetObject(objId),
		allInitOK(false),
		initPause(3000),
		force(false),
		force_out(false),
		mbregFromID(false),
		sidExchangeMode(DefaultObjectId),
		exchangeMode(emNone),
		activated(false),
		noQueryOptimization(false),
		notUseExchangeTimer(false),
		prefix(prefix),
		poll_count(0),
		prop_prefix(""),
		mb(nullptr),
		ic(_ic)
	{
		if( objId == DefaultObjectId )
			throw uniset::SystemError("(MBExchange): objId=-1?!! Use --" + prefix + "-name" );

		auto conf = uniset_conf();
		mutex_start.setName(myname + "_mutex_start");

		string conf_name(conf->getArgParam("--" + prefix + "-confnode", myname));

		cnode = conf->getNode(conf_name);

		if( cnode == NULL )
			throw uniset::SystemError("(MBExchange): Not found node <" + conf_name + " ...> for " + myname );

		shm = make_shared<SMInterface>(shmId, ui, objId, ic);

		mblog = make_shared<DebugStream>();
		mblog->setLogName(myname);
		conf->initLogStream(mblog, prefix + "-log");

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
		s_field = conf->getArgParam("--" + prefix + "-filter-field");
		s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
		mbinfo << myname << "(init): read fileter-field='" << s_field
			   << "' filter-value='" << s_fvalue << "'" << endl;

		vmonit(s_field);
		vmonit(s_fvalue);

		prop_prefix = initPropPrefix();
		vmonit(prop_prefix);

		stat_time = conf->getArgPInt("--" + prefix + "-statistic-sec", it.getProp("statistic_sec"), 0);
		vmonit(stat_time);

		if( stat_time > 0 )
			ptStatistic.setTiming(stat_time * 1000);

		recv_timeout = conf->getArgPInt("--" + prefix + "-recv-timeout", it.getProp("recv_timeout"), 500);
		vmonit(recv_timeout);

		default_timeout = conf->getArgPInt("--" + prefix + "-timeout", it.getProp("timeout"), 5000);
		vmonit(default_timeout);

		int tout = conf->getArgPInt("--" + prefix + "-reopen-timeout", it.getProp("reopen_timeout"), default_timeout * 2);
		ptReopen.setTiming(tout);

		int reinit_tout = conf->getArgPInt("--" + prefix + "-reinit-timeout", it.getProp("reinit_timeout"), default_timeout);
		ptInitChannel.setTiming(reinit_tout);

		aftersend_pause = conf->getArgPInt("--" + prefix + "-aftersend-pause", it.getProp("aftersend_pause"), 0);
		vmonit(aftersend_pause);

		noQueryOptimization = conf->getArgInt("--" + prefix + "-no-query-optimization", it.getProp("no_query_optimization"));
		vmonit(noQueryOptimization);

		mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id", it.getProp("reg_from_id"));
		mbinfo << myname << "(init): mbregFromID=" << mbregFromID << endl;
		vmonit(mbregFromID);

		polltime = conf->getArgPInt("--" + prefix + "-polltime", it.getProp("polltime"), 100);
		vmonit(polltime);

		initPause = conf->getArgPInt("--" + prefix + "-initPause", it.getProp("initPause"), 3000);

		sleepPause_msec = conf->getArgPInt("--" + prefix + "-sleepPause-msec", it.getProp("sleepPause"), 10);

		force = conf->getArgInt("--" + prefix + "-force", it.getProp("force"));
		force_out = conf->getArgInt("--" + prefix + "-force-out", it.getProp("force_out"));
		vmonit(force);
		vmonit(force_out);

		defaultMBtype = conf->getArg2Param("--" + prefix + "-default-mbtype", it.getProp("default_mbtype"), "rtu");
		defaultMBaddr = conf->getArg2Param("--" + prefix + "-default-mbaddr", it.getProp("default_mbaddr"), "");

		vmonit(defaultMBtype);
		vmonit(defaultMBaddr);

		defaultMBinitOK = conf->getArgPInt("--" + prefix + "-default-mbinit-ok", it.getProp("default_mbinitOK"), 0);
		maxQueryCount = conf->getArgPInt("--" + prefix + "-query-max-count", it.getProp("queryMaxCount"), ModbusRTU::MAXDATALEN);

		vmonit(defaultMBinitOK);

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

		activateTimeout    = conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

		vmonit(allInitOK);
	}
	// -----------------------------------------------------------------------------
	std::string MBExchange::initPropPrefix( const std::string& def_prop_prefix )
	{
		auto conf = uniset_conf();

		string pp(def_prop_prefix);

		// если задано поле для "фильтрации"
		// то в качестве префикса используем его
		if( !s_field.empty() )
			pp = s_field + "_";

		// если "принудительно" задан префикс
		// используем его.
		{
			string p("--" + prefix + "-set-prop-prefix");
			string v = conf->getArgParam(p, "");

			if( !v.empty() && v[0] != '-' )
				pp = v;
			// если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
			else if( findArgParam(p, conf->getArgc(), conf->getArgv()) != -1 )
				pp = "";
		}

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
		cout << "--prefix-reopen-timeout msec    - Таймаут для 'переоткрытия соединения' при отсутсвия соединения msec милисекунд. По умолчанию 10 сек." << endl;
		cout << "--prefix-heartbeat-id  name     - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
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
	void MBExchange::readConfiguration()
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
	bool MBExchange::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
	{
		if( uniset::check_filter(it, s_field, s_fvalue) )
			initItem(it);

		return true;
	}

	// ------------------------------------------------------------------------------------------
	MBExchange::DeviceType MBExchange::getDeviceType( const std::string& dtype ) noexcept
	{
		if( dtype.empty() )
			return dtUnknown;

		if( dtype == "mtr" || dtype == "MTR" )
			return dtMTR;

		if( dtype == "rtu" || dtype == "RTU" )
			return dtRTU;

		if( dtype == "rtu188" || dtype == "RTU188" )
			return dtRTU188;

		return dtUnknown;
	}
	// ------------------------------------------------------------------------------------------
	void MBExchange::initIterators()
	{
		shm->initIterator(itHeartBeat);
		shm->initIterator(itExchangeMode);

		for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
		{
			auto d = it1->second;
			shm->initIterator(d->resp_it);
			shm->initIterator(d->mode_it);

			for( auto && m : d->pollmap )
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

		for( auto t = thrlist.begin(); t != thrlist.end(); ++t )
		{
			shm->initIterator(t->ioit);
			shm->initIterator(t->t_ait);
		}
	}
	// -----------------------------------------------------------------------------
	void MBExchange::initValues()
	{
		for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
		{
			auto d = it1->second;

			for( auto && m : d->pollmap )
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

		for( auto t = thrlist.begin(); t != thrlist.end(); ++t )
		{
			t->value = shm->localGetValue(t->ioit, t->si.id);
		}
	}
	// -----------------------------------------------------------------------------
	bool MBExchange::isUpdateSM( bool wrFunc, long mdev ) const noexcept
	{
		if( exchangeMode == emSkipExchange || mdev == emSkipExchange )
		{
			if( wrFunc )
				return true; // данные для посылки, должны обновляться всегда (чтобы быть актуальными, когда режим переключиться обратно..)

			mblog3 << "(checkUpdateSM):"
				   << " skip... mode='emSkipExchange' " << endl;
			return false;
		}

		if( wrFunc && (exchangeMode == emReadOnly || mdev == emReadOnly) )
		{
			mblog3 << "(checkUpdateSM):"
				   << " skip... mode='emReadOnly' " << endl;
			return false;
		}

		if( !wrFunc && (exchangeMode == emWriteOnly || mdev == emWriteOnly) )
		{
			mblog3 << "(checkUpdateSM):"
				   << " skip... mode='emWriteOnly' " << endl;
			return false;
		}

		if( !wrFunc && (exchangeMode == emSkipSaveToSM || mdev == emSkipSaveToSM) )
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
		if( exchangeMode == emWriteOnly && !wrFunc )
		{
			mblog3 << myname << "(checkPoll): skip.. mode='emWriteOnly'" << endl;
			return false;
		}

		if( exchangeMode == emReadOnly && wrFunc )
		{
			mblog3 << myname << "(checkPoll): skip.. poll mode='emReadOnly'" << endl;
			return false;
		}

		if( exchangeMode == emSkipExchange )
		{
			mblog3 << myname << "(checkPoll): skip.. poll mode='emSkipExchange'" << endl;
			return false;
		}

		return true;
	}
	// -----------------------------------------------------------------------------
	bool MBExchange::isSafeMode( std::shared_ptr<MBExchange::RTUDevice>& dev ) const noexcept
	{
		if( !dev )
			return false;

		// если режим, сброс когда исчезла связь
		// то проверяем таймер
		// resp_Delay - это задержка на отпускание "пропадание" связи,
		// поэтому проверка на "0" (0 - связи нет), а значит должен включиться safeMode
		if( dev->safeMode == safeResetIfNotRespond )
			return !dev->resp_Delay.get();

		return ( dev->safeMode != safeNone );
	}
	// -----------------------------------------------------------------------------
	void MBExchange::printMap( MBExchange::RTUDeviceMap& m )
	{
		cout << "devices: " << endl;

		for( auto it = m.begin(); it != m.end(); ++it )
			cout << "  " <<  *(it->second) << endl;
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, MBExchange::RTUDeviceMap& m )
	{
		os << "devices: " << endl;

		for( auto it = m.begin(); it != m.end(); ++it )
			os << "  " <<  *(it->second) << endl;

		return os;
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, MBExchange::RTUDevice& d )
	{
		os  << "addr=" << ModbusRTU::addr2str(d.mbaddr)
			<< " type=" << d.dtype
			<< " respond_id=" << d.resp_id
			<< " respond_timeout=" << d.resp_Delay.getOffDelay()
			<< " respond_state=" << d.resp_state
			<< " respond_invert=" << d.resp_invert
			<< " safeMode=" << (MBExchange::SafeMode)d.safeMode
			<< endl;


		os << "  regs: " << endl;

		for( const auto& m : d.pollmap )
		{
			for( const auto& it : * (m.second) )
				os << "     " << it.second << endl;
		}

		return os;
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const MBExchange::RegInfo* r )
	{
		return os << (*r);
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const MBExchange::RegInfo& r )
	{
		os << " id=" << r.regID
		   << " mbreg=" << ModbusRTU::dat2str(r.mbreg)
		   << " mbfunc=" << r.mbfunc
		   << " q_num=" << r.q_num
		   << " q_count=" << r.q_count
		   << " value=" << ModbusRTU::dat2str(r.mbval) << "(" << (int)r.mbval << ")"
		   << " mtrType=" << MTR::type2str(r.mtrType)
		   << endl;

		for( const auto& s : r.slst )
			os << "         " << s << endl;

		return os;
	}
	// -----------------------------------------------------------------------------
	void MBExchange::rtuQueryOptimization( RTUDeviceMap& dm )
	{
		mbinfo << myname << "(rtuQueryOptimization): optimization..." << endl;

		for( const auto& d : dm )
			rtuQueryOptimizationForDevice(d.second);

		//		printMap(dm);
		//		assert(false);
	}
	// -----------------------------------------------------------------------------
	void MBExchange::rtuQueryOptimizationForDevice( const std::shared_ptr<RTUDevice>& d )
	{
		mbinfo << myname << "(rtuQueryOptimizationForDevice): dev addr="
			   << ModbusRTU::addr2str(d->mbaddr) << " optimization..." << endl;

		for( const auto& m : d->pollmap )
			rtuQueryOptimizationForRegMap(m.second);
	}
	// -----------------------------------------------------------------------------
	void MBExchange::rtuQueryOptimizationForRegMap( const std::shared_ptr<RegMap>& regmap )
	{
		if( regmap->size() <= 1 )
			return;

		// Вообще в map они уже лежат в нужном порядке, т.е. функция genRegID() гарантирует
		// что регистры идущие подряд с одниковой функцией чтения/записи получат подряд идущие RegID.
		// так что оптимтизация это просто нахождение мест где RegID идут не подряд...
		for( auto it = regmap->begin(); it != regmap->end(); ++it )
		{
			auto& beg = it->second;
			ModbusRTU::RegID regID = beg->regID;
			beg->q_count = 1;
			beg->q_num = 0;
			++it;

			// склеиваем регистры идущие подряд
			for( ; it != regmap->end(); ++it )
			{
				if( (it->second->regID - regID) > 1 )
				{
					// этот регистр должен войти уже в следующий запрос,
					// надо вернуть на шаг обратно..
					--it;
					break;
				}

				beg->q_count++;
				regID = it->second->regID;
				it->second->q_num = beg->q_count - 1;
				it->second->q_count = 0;

				if( beg->q_count >= maxQueryCount )
					break;
			}

			// Корректировка типа функции, в случае необходимости...
			if( beg->q_count > 1 && beg->mbfunc == ModbusRTU::fnWriteOutputSingleRegister )
			{
				mbwarn << myname << "(rtuQueryOptimization): "
					   << " optimization change func=" << ModbusRTU::fnWriteOutputSingleRegister
					   << " <--> func=" << ModbusRTU::fnWriteOutputRegisters
					   << " for mbaddr=" << ModbusRTU::addr2str(beg->dev->mbaddr)
					   << " mbreg=" << ModbusRTU::dat2str(beg->mbreg);

				beg->mbfunc = ModbusRTU::fnWriteOutputRegisters;
			}
			else if( beg->q_count > 1 && beg->mbfunc == ModbusRTU::fnForceSingleCoil )
			{
				mbwarn << myname << "(rtuQueryOptimization): "
					   << " optimization change func=" << ModbusRTU::fnForceSingleCoil
					   << " <--> func=" << ModbusRTU::fnForceMultipleCoils
					   << " for mbaddr=" << ModbusRTU::addr2str(beg->dev->mbaddr)
					   << " mbreg=" << ModbusRTU::dat2str(beg->mbreg);

				beg->mbfunc = ModbusRTU::fnForceMultipleCoils;
			}

			// надо до внешнего цикла, где будет ++it
			// проверить условие.. (т.к. мы во внутреннем цикле итерировались
			if( it == regmap->end() )
				break;
		}
	}
	// -----------------------------------------------------------------------------
	//std::ostream& operator<<( std::ostream& os, MBExchange::PList& lst )
	std::ostream& MBExchange::print_plist( std::ostream& os, const MBExchange::PList& lst )
	{
		auto conf = uniset_conf();

		os << "[ ";

		for( const auto& p : lst )
			os << "(" << p.si.id << ")" << conf->oind->getBaseName(conf->oind->getMapName(p.si.id)) << " ";

		os << "]";

		return os;
	}
	// -----------------------------------------------------------------------------
	void MBExchange::firstInitRegisters()
	{
		// если все вернут TRUE, значит OK.
		allInitOK = true;

		for( auto it = initRegList.begin(); it != initRegList.end(); ++it )
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
	bool MBExchange::preInitRead( InitList::iterator& p )
	{
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

			if( q_count > maxQueryCount /* ModbusRTU::MAXDATALEN */ )
			{
				mblog3 << myname << "(preInitRead): count(" << q_count
					   << ") > MAXDATALEN(" << maxQueryCount /* ModbusRTU::MAXDATALEN */
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
				DataGuard d(q_count);
				size_t m = 0;

				for( size_t i = 0; i < ret.bcnt; i++ )
				{
					ModbusRTU::DataBits b(ret.data[i]);

					for( size_t k = 0; k < ModbusRTU::BitsPerByte && m < q_count; k++, m++ )
						d.data[m] = b[k];
				}

				p->initOK = initSMValue(d.data, q_count, &(p->p));
			}
			break;

			case ModbusRTU::fnReadCoilStatus:
			{
				ModbusRTU::ReadCoilRetMessage ret = mb->read01(dev->mbaddr, p->mbreg, q_count);
				DataGuard d(q_count);
				size_t m = 0;

				for( size_t i = 0; i < ret.bcnt; i++ )
				{
					ModbusRTU::DataBits b(ret.data[i]);

					for( size_t k = 0; k < ModbusRTU::BitsPerByte && m < q_count; k++, m++ )
						d.data[m] = b[k];
				}

				p->initOK = initSMValue(d.data, q_count, &(p->p));
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
	bool MBExchange::initSMValue( ModbusRTU::ModbusData* data, int count, RSProperty* p )
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
				IOBase::processingFasAI( p, (float)f, shm, true );
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
		catch( const uniset::Exception& ex )
		{
			mblog3 << myname << "(initSMValue): " << ex << endl;
		}
		catch( const CORBA::SystemException& ex)
		{
			mblog3 << myname << "(initSMValue): CORBA::SystemException: "
				   << ex.NP_minorString() << endl;
		}
		catch(...)
		{
			mblog3 << myname << "(initSMValue): catch ..." << endl;
		}

		return false;
	}
	// -----------------------------------------------------------------------------
	bool MBExchange::pollRTU( std::shared_ptr<RTUDevice>& dev, RegMap::iterator& it )
	{
		auto p = it->second;

		if( dev->mode == emSkipExchange )
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

			if( p->q_count > maxQueryCount /* ModbusRTU::MAXDATALEN */ )
			{
				mblog3 << myname << "(pollRTU): count(" << p->q_count
					   << ") > MAXDATALEN(" << maxQueryCount /* ModbusRTU::MAXDATALEN */
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
		for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
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
				catch( const uniset::Exception& ex )
				{
					mblog3 << myname << "(updateSM): " << ex << endl;
				}
				catch( const CORBA::SystemException& ex )
				{
					mblog3 << myname << "(updateSM): CORBA::SystemException: "
						   << ex.NP_minorString() << endl;
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
								d->safeMode = safeExternalControl;
							else
								d->safeMode = safeNone;
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
					catch( const uniset::Exception& ex )
					{
						mblog3 << myname << "(updateSM): " << ex << endl;
					}
					catch( const CORBA::SystemException& ex )
					{
						mblog3 << myname << "(updateSM): CORBA::SystemException: "
							   << ex.NP_minorString() << endl;
					}
					catch( std::exception& ex )
					{
						mblog3 << myname << "(updateSM): check safemodeSensor: " << ex.what() << endl;
					}
				}
			}

			for( auto && m : d->pollmap )
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
						if( d->dtype == dtRTU )
							updateRTU(it);
						else if( d->dtype == dtMTR )
							updateMTR(it);
						else if( d->dtype == dtRTU188 )
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
					catch( const uniset::Exception& ex )
					{
						mblog3 << myname << "(updateSM): " << ex << endl;
					}
					catch( const CORBA::SystemException& ex )
					{
						mblog3 << myname << "(updateSM): CORBA::SystemException: "
							   << ex.NP_minorString() << endl;
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
	void MBExchange::updateRTU( RegMap::iterator& rit )
	{
		auto& r = rit->second;

		for( auto && it : r->slst )
			updateRSProperty( &it, false );

	}
	// -----------------------------------------------------------------------------
	void MBExchange::updateRSProperty( RSProperty* p, bool write_only )
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
							VTypes::F2 f1(d.data, VTypes::F2::wsize());
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

						VTypes::F4 f(d.data, VTypes::F4::wsize());

						IOBase::processingFasAI( p, (float)f, shm, force );
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
							VTypes::I2 i2(d.data, VTypes::I2::wsize());
							v = (int)i2;
						}
						else if( p->vType == VTypes::vtI2r )
						{
							VTypes::I2r i2(d.data, VTypes::I2::wsize());
							v = (int)i2;
						}

						IOBase::processingAsAI( p, v, shm, force );
					}
				}
			}
			else if( p->vType == VTypes::vtU2 || p->vType == VTypes::vtU2r )
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
							VTypes::U2 u2(d.data, VTypes::U2::wsize());
							v = (uint32_t)u2;
						}
						else if( p->vType == VTypes::vtU2r )
						{
							VTypes::U2r u2(d.data, VTypes::U2::wsize());
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
		catch( const uniset::Exception& ex )
		{
			mblog3 << myname << "(updateRSProperty): " << ex << endl;
		}
		catch( const CORBA::SystemException& ex )
		{
			mblog3 << myname << "(updateRSProperty): CORBA::SystemException: "
				   << ex.NP_minorString() << endl;
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
	void MBExchange::updateMTR( RegMap::iterator& rit )
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

							MTR::T3 t(d.data, MTR::T3::wsize());
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

							MTR::T5 t(d.data, MTR::T5::wsize());

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

							MTR::T6 t(d.data, MTR::T6::wsize());

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

							MTR::T7 t(d.data, MTR::T7::wsize());

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

							MTR::F1 t(d.data, MTR::F1::wsize());
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
			catch( const uniset::Exception& ex )
			{
				mblog3 << myname << "(updateMTR): " << ex << endl;
			}
			catch( const CORBA::SystemException& ex )
			{
				mblog3 << myname << "(updateMTR): CORBA::SystemException: "
					   << ex.NP_minorString() << endl;
			}
			catch(...)
			{
				mblog3 << myname << "(updateMTR): catch ..." << endl;
			}
		}
	}
	// -----------------------------------------------------------------------------
	void MBExchange::updateRTU188( RegMap::iterator& rit )
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
			catch( const uniset::Exception& ex )
			{
				mblog3 << myname << "(updateRTU188): " << ex << endl;
			}
			catch( const CORBA::SystemException& ex )
			{
				mblog3 << myname << "(updateRTU188): CORBA::SystemException: "
					   << ex.NP_minorString() << endl;
			}
			catch(...)
			{
				mblog3 << myname << "(updateRTU188): catch ..." << endl;
			}
		}
	}
	// -----------------------------------------------------------------------------

	std::shared_ptr<MBExchange::RTUDevice> MBExchange::addDev( RTUDeviceMap& mp, ModbusRTU::ModbusAddr a, UniXML::iterator& xmlit )
	{
		auto it = mp.find(a);

		if( it != mp.end() )
		{
			string s_dtype(xmlit.getProp(prop_prefix + "mbtype"));

			if( s_dtype.empty() )
				s_dtype = defaultMBtype;

			DeviceType dtype = getDeviceType(s_dtype);

			if( it->second->dtype != dtype )
			{
				mbcrit << myname << "(addDev): OTHER mbtype=" << dtype << " for " << xmlit.getProp("name")
					   << ". Already used devtype=" <<  it->second->dtype
					   << " for mbaddr=" << ModbusRTU::addr2str(it->second->mbaddr)
					   << endl;
				return 0;
			}

			mbinfo << myname << "(addDev): device for addr=" << ModbusRTU::addr2str(a)
				   << " already added. Ignore device params for " << xmlit.getProp("name") << " ..." << endl;
			return it->second;
		}

		auto d = make_shared<MBExchange::RTUDevice>();
		d->mbaddr = a;

		if( !initRTUDevice(d, xmlit) )
		{
			d.reset();
			return 0;
		}

		mp.insert( std::make_pair(a, d) );
		return d;
	}
	// ------------------------------------------------------------------------------------------
	std::shared_ptr<MBExchange::RegInfo> MBExchange::addReg( std::shared_ptr<RegMap>& mp, ModbusRTU::RegID regID, ModbusRTU::ModbusData r,
			UniXML::iterator& xmlit, std::shared_ptr<MBExchange::RTUDevice> dev )
	{
		auto it = mp->find(regID);

		if( it != mp->end() )
		{
			if( !it->second->dev )
			{
				mbcrit << myname << "(addReg): for " << xmlit.getProp("name")
					   << " dev=0!!!! " << endl;
				return 0;
			}

			if( it->second->dev->dtype != dev->dtype )
			{
				mbcrit << myname << "(addReg): OTHER mbtype=" << dev->dtype << " for " << xmlit.getProp("name")
					   << ". Already used devtype=" <<  it->second->dev->dtype << " for " << it->second->dev << endl;
				return 0;
			}

			mbinfo << myname << "(addReg): reg=" << ModbusRTU::dat2str(r)
				   << "(id=" << regID << ")"
				   << " already added for " << (*it->second)
				   << " Ignore register params for " << xmlit.getProp("name") << " ..." << endl;

			it->second->rit = it;
			return it->second;
		}

		auto ri = make_shared<MBExchange::RegInfo>();

		if( !initRegInfo(ri, xmlit, dev) )
			return 0;

		ri->mbreg = r;
		ri->regID = regID;

		mp->insert( std::make_pair(regID, ri) );
		ri->rit = mp->find(regID);

		mbinfo << myname << "(addReg): reg=" << ModbusRTU::dat2str(r) << "(id=" << regID << ")" << endl;

		return ri;
	}
	// ------------------------------------------------------------------------------------------
	MBExchange::RSProperty* MBExchange::addProp( PList& plist, RSProperty&& p )
	{
		for( auto && it : plist )
		{
			if( it.si.id == p.si.id && it.si.node == p.si.node )
				return &it;
		}

		plist.emplace_back( std::move(p) );
		auto it = plist.end();
		--it;
		return &(*it);
	}
	// ------------------------------------------------------------------------------------------
	bool MBExchange::initRSProperty( RSProperty& p, UniXML::iterator& it )
	{
		if( !IOBase::initItem(&p, it, shm, prop_prefix, false, mblog, myname) )
			return false;

		// проверяем не пороговый ли это датчик (т.е. не связанный с обменом)
		// тогда заносим его в отдельный список
		if( p.t_ai != DefaultObjectId )
		{
			thrlist.emplace_back( std::move(p) );
			return true;
		}

		string sbit(IOBase::initProp(it, "nbit", prop_prefix, false));

		if( !sbit.empty() )
		{
			p.nbit = uniset::uni_atoi(sbit.c_str());

			if( p.nbit < 0 || p.nbit >= ModbusRTU::BitsPerData )
			{
				mbcrit << myname << "(initRSProperty): BAD nbit=" << (int)p.nbit
					   << ". (0 >= nbit < " << ModbusRTU::BitsPerData << ")." << endl;
				return false;
			}
		}

		if( p.nbit > 0 &&
				( p.stype == UniversalIO::AI ||
				  p.stype == UniversalIO::AO ) )
		{
			mbwarn << "(initRSProperty): (ignore) uncorrect param`s nbit!=0(" << p.nbit << ")"
				   << " for iotype=" << p.stype << " for " << it.getProp("name") << endl;
		}

		string sbyte(IOBase::initProp(it, "nbyte", prop_prefix, false) );

		if( !sbyte.empty() )
		{
			p.nbyte = uniset::uni_atoi(sbyte.c_str());

			if( p.nbyte > VTypes::Byte::bsize )
			{
				mbwarn << myname << "(initRSProperty): BAD nbyte=" << p.nbyte
					   << ". (0 >= nbyte < " << VTypes::Byte::bsize << ")." << endl;
				return false;
			}
		}

		string vt( IOBase::initProp(it, "vtype", prop_prefix, false) );

		if( vt.empty() )
		{
			p.rnum = VTypes::wsize(VTypes::vtUnknown);
			p.vType = VTypes::vtUnknown;
		}
		else
		{
			VTypes::VType v(VTypes::str2type(vt));

			if( v == VTypes::vtUnknown )
			{
				mbcrit << myname << "(initRSProperty): Unknown tcp_vtype='" << vt << "' for "
					   << it.getProp("name")
					   << endl;

				return false;
			}

			p.vType = v;
			p.rnum = VTypes::wsize(v);
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool MBExchange::initRegInfo( std::shared_ptr<RegInfo>& r, UniXML::iterator& it,  std::shared_ptr<MBExchange::RTUDevice>& dev )
	{
		r->dev = dev;
		r->mbval = IOBase::initIntProp(it, "default", prop_prefix, false);

		if( dev->dtype == MBExchange::dtRTU )
		{
			//        mblog.info() << myname << "(initRegInfo): init RTU.."
		}
		else if( dev->dtype == MBExchange::dtMTR )
		{
			// only for MTR
			if( !initMTRitem(it, r) )
				return false;
		}
		else if( dev->dtype == MBExchange::dtRTU188 )
		{
			// only for RTU188
			if( !initRTU188item(it, r) )
				return false;

			UniversalIO::IOType t = uniset::getIOType(IOBase::initProp(it, "iotype", prop_prefix, false));
			r->mbreg = RTUStorage::getRegister(r->rtuJack, r->rtuChan, t);
			r->mbfunc = RTUStorage::getFunction(r->rtuJack, r->rtuChan, t);

			// т.к. с RTU188 свой обмен
			// mbreg и mbfunc поля не используются
			return true;

		}
		else
		{
			mbcrit << myname << "(initRegInfo): Unknown mbtype='" << dev->dtype
				   << "' for " << it.getProp("name") << endl;
			return false;
		}

		if( mbregFromID )
		{
			if( it.getProp("id").empty() )
				r->mbreg = uniset_conf()->getSensorID(it.getProp("name"));
			else
				r->mbreg = it.getIntProp("id");
		}
		else
		{
			string sr( IOBase::initProp(it, "mbreg", prop_prefix, false) );

			if( sr.empty() )
			{
				mbcrit << myname << "(initItem): Unknown 'mbreg' for " << it.getProp("name") << endl;
				return false;
			}

			r->mbreg = ModbusRTU::str2mbData(sr);
		}

		r->mbfunc     = ModbusRTU::fnUnknown;
		string f( IOBase::initProp(it, "mbfunc", prop_prefix, false) );

		if( !f.empty() )
		{
			r->mbfunc = (ModbusRTU::SlaveFunctionCode)uniset::uni_atoi(f.c_str());

			if( r->mbfunc == ModbusRTU::fnUnknown )
			{
				mbcrit << myname << "(initRegInfo): Unknown mbfunc ='" << f
					   << "' for " << it.getProp("name") << endl;
				return false;
			}
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool MBExchange::initRTUDevice( std::shared_ptr<RTUDevice>& d, UniXML::iterator& it )
	{
		string mbtype(IOBase::initProp(it, "mbtype", prop_prefix, false));

		if(mbtype.empty())
			mbtype = defaultMBtype;

		d->dtype = getDeviceType(mbtype);

		if( d->dtype == dtUnknown )
		{
			mbcrit << myname << "(initRTUDevice): Unknown tcp_mbtype='" << mbtype << "'"
				   << ". Use: rtu "
				   << " for " << it.getProp("name") << endl;
			return false;
		}

		string addr( IOBase::initProp(it, "mbaddr", prop_prefix, false) );

		if( addr.empty() )
			addr = defaultMBaddr;

		if( addr.empty() )
		{
			mbcrit << myname << "(initRTUDevice): Unknown mbaddr for " << it.getProp("name") << endl;
			return false;
		}

		d->mbaddr = ModbusRTU::str2mbAddr(addr);

		if( d->dtype == MBExchange::dtRTU188 )
		{
			if( !d->rtu188 )
				d->rtu188 = make_shared<RTUStorage>(d->mbaddr);
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool MBExchange::initItem( UniXML::iterator& it )
	{
		RSProperty p;

		if( !initRSProperty(p, it) )
			return false;

		if( p.t_ai != DefaultObjectId ) // пороговые датчики в список обмена вносить не надо.
			return true;

		string addr( IOBase::initProp(it, "mbaddr", prop_prefix, false) );

		if( addr.empty() )
			addr = defaultMBaddr;

		if( addr.empty() )
		{
			mbcrit << myname << "(initItem): Unknown mbaddr(" << IOBase::initProp(it, "mbaddr", prop_prefix, false) << ")='" << addr << "' for " << it.getProp("name") << endl;
			return false;
		}

		ModbusRTU::ModbusAddr mbaddr = ModbusRTU::str2mbAddr(addr);

		auto dev = addDev(devices, mbaddr, it);

		if( !dev )
		{
			mbcrit << myname << "(initItem): " << it.getProp("name") << " CAN`T ADD for polling!" << endl;
			return false;
		}

		ModbusRTU::ModbusData mbreg = 0;
		int fn = IOBase::initIntProp(it, "mbfunc", prop_prefix, false);

		if( dev->dtype == dtRTU188 )
		{
			auto r_tmp = make_shared<RegInfo>();

			if( !initRTU188item(it, r_tmp) )
			{
				mbcrit << myname << "(initItem): init RTU188 failed for " << it.getProp("name") << endl;
				r_tmp.reset();
				return false;
			}

			mbreg = RTUStorage::getRegister(r_tmp->rtuJack, r_tmp->rtuChan, p.stype);
			fn = RTUStorage::getFunction(r_tmp->rtuJack, r_tmp->rtuChan, p.stype);
		}
		else
		{
			if( mbregFromID )
				mbreg = p.si.id; // conf->getSensorID(it.getProp("name"));
			else
			{
				string reg( IOBase::initProp(it, "mbreg", prop_prefix, false) );

				if( reg.empty() )
				{
					mbcrit << myname << "(initItem): unknown mbreg(" << prop_prefix << ") for " << it.getProp("name") << endl;
					return false;
				}

				mbreg = ModbusRTU::str2mbData(reg);
			}

			if( p.nbit != -1 )
			{
				if( fn == ModbusRTU::fnReadCoilStatus || fn == ModbusRTU::fnReadInputStatus )
				{
					mbcrit << myname << "(initItem): MISMATCHED CONFIGURATION!  nbit=" << (int)p.nbit << " func=" << fn
						   << " for " << it.getProp("name") << endl;
					return false;
				}
			}
		}

		/*! приоритет опроса:
		 * 1...n - задаёт "часоту" опроса. Т.е. каждые 1...n циклов
		*/
		size_t pollfactor = IOBase::initIntProp(it, "pollfactor", prop_prefix, false, 0);

		std::shared_ptr<RegMap> rmap;

		auto rit = dev->pollmap.find(pollfactor);

		if( rit == dev->pollmap.end() )
		{
			rmap = make_shared<RegMap>();
			dev->pollmap.emplace(pollfactor, rmap);
		}
		else
			rmap = rit->second;

		// формула для вычисления ID
		// требования:
		//  - ID > диапазона возможных регитров
		//  - разные функции должны давать разный ID
		ModbusRTU::RegID rID = ModbusRTU::genRegID(mbreg, fn);

		auto ri = addReg(rmap, rID, mbreg, it, dev);

		if( dev->dtype == dtMTR )
		{
			p.rnum = MTR::wsize(ri->mtrType);

			if( p.rnum <= 0 )
			{
				mbcrit << myname << "(initItem): unknown word size for " << it.getProp("name") << endl;
				return false;
			}
		}

		if( !ri )
			return false;

		ri->dev = dev;

		// ПРОВЕРКА!
		// если функция на запись, то надо проверить
		// что один и тотже регистр не перезапишут несколько датчиков
		// это возможно только, если они пишут биты!!
		// ИТОГ:
		// Если для функций записи список датчиков для регистра > 1
		// значит в списке могут быть только битовые датчики
		// и если идёт попытка внести в список не битовый датчик то ОШИБКА!
		// И наоборот: если идёт попытка внести битовый датчик, а в списке
		// уже сидит датчик занимающий целый регистр, то тоже ОШИБКА!
		if( ModbusRTU::isWriteFunction(ri->mbfunc) )
		{
			if( p.nbit < 0 &&  ri->slst.size() > 1 )
			{
				auto conf = uniset_conf();
				ostringstream sl;
				sl << "[ ";

				for( const auto& i : ri->slst )
					sl << ORepHelpers::getShortName(conf->oind->getMapName(i.si.id)) << ",";

				sl << "]";

				mbcrit << myname << "(initItem): FAILED! Sharing SAVE (not bit saving) to "
					   << " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg) << "(" << (int)ri->mbreg << ")"
					   << " conflict with sensors " << sl.str() << endl;

				std::abort();     // ABORT PROGRAM!!!!
				return false;
			}

			if( p.nbit >= 0 && ri->slst.size() == 1 )
			{
				auto it2 = ri->slst.begin();

				if( it2->nbit < 0 )
				{
					mbcrit << myname << "(initItem): FAILED! Sharing SAVE (mbreg="
						   << ModbusRTU::dat2str(ri->mbreg) << "(" << (int)ri->mbreg << ") already used)!"
						   << " IGNORE --> " << it.getProp("name") << endl;
					std::abort();     // ABORT PROGRAM!!!!
					return false;
				}
			}

			// Раз это регистр для записи, то как минимум надо сперва
			// инициализировать значением из SM
			ri->sm_initOK = IOBase::initIntProp(it, "sm_initOK", prop_prefix, false);
			ri->mb_initOK = true;
		}
		else
		{
			ri->mb_initOK = defaultMBinitOK;
			ri->sm_initOK = false;
		}


		RSProperty* p1 = addProp(ri->slst, std::move(p) );

		if( !p1 )
			return false;

		p1->reg = ri;


		if( p1->rnum > 1 )
		{
			ri->q_count = p1->rnum;
			ri->q_num = 1;

			for( auto i = 1; i < p1->rnum; i++ )
			{
				ModbusRTU::RegID id1 = ModbusRTU::genRegID(mbreg + i, ri->mbfunc);
				auto r = addReg(rmap, id1, mbreg + i, it, dev);
				r->q_num = i + 1;
				r->q_count = 1;
				r->mbfunc = ri->mbfunc;
				r->mb_initOK = defaultMBinitOK;
				r->sm_initOK = false;

				if( ModbusRTU::isWriteFunction(ri->mbfunc) )
				{
					// Если занимает несколько регистров, а указана функция записи "одного",
					// то это ошибка..
					if( ri->mbfunc != ModbusRTU::fnWriteOutputRegisters &&
							ri->mbfunc != ModbusRTU::fnForceMultipleCoils )
					{
						mbcrit << myname << "(initItem): Bad write function ='" << ModbusRTU::fnWriteOutputSingleRegister
							   << "' for vtype='" << p1->vType << "'"
							   << " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg)
							   << " for " << it.getProp("name") << endl;

						abort();     // ABORT PROGRAM!!!!
						return false;
					}
				}
			}
		}

		// Фомируем список инициализации
		bool need_init = IOBase::initIntProp(it, "preinit", prop_prefix, false);

		if( need_init && ModbusRTU::isWriteFunction(ri->mbfunc) )
		{
			InitRegInfo ii;
			ii.p = std::move(p);
			ii.dev = dev;
			ii.ri = ri;

			string s_reg(IOBase::initProp(it, "init_mbreg", prop_prefix, false));

			if( !s_reg.empty() )
				ii.mbreg = ModbusRTU::str2mbData(s_reg);
			else
				ii.mbreg = ri->mbreg;

			string s_mbfunc(it.getProp(prop_prefix + "init_mbfunc"));

			if( !s_mbfunc.empty() )
			{
				ii.mbfunc = (ModbusRTU::SlaveFunctionCode)uniset::uni_atoi(s_mbfunc);

				if( ii.mbfunc == ModbusRTU::fnUnknown )
				{
					mbcrit << myname << "(initItem): Unknown tcp_init_mbfunc ='" << s_mbfunc
						   << "' for " << it.getProp("name") << endl;
					return false;
				}
			}
			else
			{
				switch(ri->mbfunc)
				{
					case ModbusRTU::fnWriteOutputSingleRegister:
						ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
						break;

					case ModbusRTU::fnForceSingleCoil:
						ii.mbfunc = ModbusRTU::fnReadCoilStatus;
						break;

					case ModbusRTU::fnWriteOutputRegisters:
						ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
						break;

					case ModbusRTU::fnForceMultipleCoils:
						ii.mbfunc = ModbusRTU::fnReadCoilStatus;
						break;

					default:
						ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
						break;
				}
			}

			initRegList.emplace_back( std::move(ii) );
			ri->mb_initOK = false;
			ri->sm_initOK = false;
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool MBExchange::initMTRitem( UniXML::iterator& it, std::shared_ptr<RegInfo>& p )
	{
		p->mtrType = MTR::str2type(it.getProp(prop_prefix + "mtrtype"));

		if( p->mtrType == MTR::mtUnknown )
		{
			mbcrit << myname << "(readMTRItem): Unknown mtrtype '"
				   << it.getProp(prop_prefix + "mtrtype")
				   << "' for " << it.getProp("name") << endl;

			return false;
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool MBExchange::initRTU188item( UniXML::iterator& it, std::shared_ptr<RegInfo>& p )
	{
		string jack(IOBase::initProp(it, "jakc", prop_prefix, false));
		string chan(IOBase::initProp(it, "channel", prop_prefix, false));

		if( jack.empty() )
		{
			mbcrit << myname << "(readRTU188Item): Unknown " << prop_prefix << "jack='' "
				   << " for " << it.getProp("name") << endl;
			return false;
		}

		p->rtuJack = RTUStorage::s2j(jack);

		if( p->rtuJack == RTUStorage::nUnknown )
		{
			mbcrit << myname << "(readRTU188Item): Unknown " << prop_prefix << "jack=" << jack
				   << " for " << it.getProp("name") << endl;
			return false;
		}

		if( chan.empty() )
		{
			mbcrit << myname << "(readRTU188Item): Unknown channel='' "
				   << " for " << it.getProp("name") << endl;
			return false;
		}

		p->rtuChan = uniset::uni_atoi(chan);

		mblog2 << myname << "(readRTU188Item): add jack='" << jack << "'"
			   << " channel='" << p->rtuChan << "'" << endl;

		return true;
	}
	// ------------------------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const MBExchange::DeviceType& dt )
	{
		switch(dt)
		{
			case MBExchange::dtRTU:
				os << "RTU";
				break;

			case MBExchange::dtMTR:
				os << "MTR";
				break;

			case MBExchange::dtRTU188:
				os << "RTU188";
				break;

			default:
				os << "Unknown device type (" << (int)dt << ")";
				break;
		}

		return os;
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const MBExchange::RSProperty& p )
	{
		os     << " (" << ModbusRTU::dat2str(p.reg->mbreg) << ")"
			   << " sid=" << p.si.id
			   << " stype=" << p.stype
			   << " nbit=" << (int)p.nbit
			   << " nbyte=" << p.nbyte
			   << " rnum=" << p.rnum
			   << " safeval=" << p.safeval
			   << " invert=" << p.invert;

		if( p.stype == UniversalIO::AI || p.stype == UniversalIO::AO )
		{
			os     << p.cal
				   << " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
		}

		return os;
	}
	// -----------------------------------------------------------------------------
	void MBExchange::initDeviceList()
	{
		auto conf = uniset_conf();
		xmlNode* respNode = 0;
		const std::shared_ptr<UniXML> xml = conf->getConfXML();

		if( xml )
			respNode = xml->extFindNode(cnode, 1, 1, "DeviceList");

		if( respNode )
		{
			UniXML::iterator it1(respNode);

			if( it1.goChildren() )
			{
				for(; it1.getCurrent(); it1.goNext() )
				{
					ModbusRTU::ModbusAddr a = ModbusRTU::str2mbAddr(it1.getProp("addr"));
					initDeviceInfo(devices, a, it1);
				}
			}
			else
				mbwarn << myname << "(init): <DeviceList> empty section..." << endl;
		}
		else
			mbwarn << myname << "(init): <DeviceList> not found..." << endl;
	}
	// -----------------------------------------------------------------------------
	bool MBExchange::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML::iterator& it )
	{
		auto d = m.find(a);

		if( d == m.end() )
		{
			mbwarn << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
			return false;
		}

		auto& dev = d->second;

		dev->ask_every_reg = it.getIntProp("ask_every_reg");

		mbinfo << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a)
			   << " ask_every_reg=" << dev->ask_every_reg << endl;

		string s(it.getProp("respondSensor"));

		if( !s.empty() )
		{
			dev->resp_id = uniset_conf()->getSensorID(s);

			if( dev->resp_id == DefaultObjectId )
			{
				mbinfo << myname << "(initDeviceInfo): not found ID for respondSensor=" << s << endl;
				return false;
			}
		}

		auto conf = uniset_conf();

		string mod(it.getProp("modeSensor"));

		if( !mod.empty() )
		{
			dev->mode_id = conf->getSensorID(mod);

			if( dev->mode_id == DefaultObjectId )
			{
				mbcrit << myname << "(initDeviceInfo): not found ID for modeSensor=" << mod << endl;
				return false;
			}

			UniversalIO::IOType m_iotype = conf->getIOType(dev->mode_id);

			if( m_iotype != UniversalIO::AI )
			{
				mbcrit << myname << "(initDeviceInfo): modeSensor='" << mod << "' must be 'AI'" << endl;
				return false;
			}
		}

		// сперва проверим не задан ли режим "safemodeResetIfNotRespond"
		if( it.getIntProp("safemodeResetIfNotRespond") )
			dev->safeMode = MBExchange::safeResetIfNotRespond;

		// потом проверим датчик для "safeExternalControl"
		string safemode = it.getProp("safemodeSensor");

		if( !safemode.empty() )
		{
			dev->safemode_id = conf->getSensorID(safemode);

			if( dev->safemode_id == DefaultObjectId )
			{
				mbcrit << myname << "(initDeviceInfo): not found ID for safemodeSensor=" << safemode << endl;
				return false;
			}

			string safemodeValue(it.getProp("safemodeValue"));

			if( !safemodeValue.empty() )
				dev->safemode_value = uni_atoi(safemodeValue);

			dev->safeMode = MBExchange::safeExternalControl;
		}

		mbinfo << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) << endl;
		int tout = it.getPIntProp("timeout", default_timeout );

		dev->resp_Delay.set(0, tout);
		dev->resp_invert = it.getIntProp("invert");
		dev->resp_force =  it.getIntProp("force");

		int init_tout = it.getPIntProp("respondInitTimeout", tout);
		dev->resp_ptInit.setTiming(init_tout);

		return true;
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

			if( !shm->isLocalwork() )
				rtuQueryOptimization(devices);

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

				if( devices.empty() )
				{
					mbcrit << myname << "(sysCommand): ************* ITEM MAP EMPTY! terminated... *************" << endl << flush;
					//					std::terminate();
					uterminate();
					return;
				}

				mbinfo << myname << "(sysCommand): device map size= " << devices.size() << endl;

				if( !shm->isLocalwork() )
					initDeviceList();

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
				askTimer(tmExchange, polltime);
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

			default:
				break;
		}
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
			//			std::terminate();  // прерываем (перезапускаем) процесс...
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

		for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
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

			for( auto && m : d->pollmap )
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

		for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
		{
			auto& d(it1->second);

			if( sm->id == d->mode_id )
				d->mode = sm->value;

			if( sm->id == d->safemode_id )
			{
				if( sm->value == d->safemode_value )
					d->safeMode = safeExternalControl;
				else
					d->safeMode = safeNone;
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

					for( auto && i : it.second->slst )
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
		if( !mb )
		{
			mb = initMB(false);

			if( !isProcActive() )
				return false;

			updateSM();
			allInitOK = false;

			if( !mb )
				return false;

			for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
				it1->second->resp_ptInit.reset();
		}

		if( !allInitOK )
			firstInitRegisters();

		if( !isProcActive() )
			return false;

		ncycle++;
		bool allNotRespond = true;

		for( auto it1 = devices.begin(); it1 != devices.end(); ++it1 )
		{
			auto d(it1->second);

			if( d->mode_id != DefaultObjectId && d->mode == emSkipExchange )
				continue;

			mblog3 << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr)
				   << " regs=" << d->pollmap.size() << endl;

			d->prev_numreply.store(d->numreply);

			for( auto && m : d->pollmap )
			{
				if( m.first > 1 && (ncycle % m.first) != 0 )
					continue;

				auto&& regmap = m.second;

				for( auto it = regmap->begin(); it != regmap->end(); ++it )
				{
					if( !isProcActive() )
						return false;

					if( exchangeMode == emSkipExchange )
						continue;

					try
					{
						if( d->dtype == MBExchange::dtRTU || d->dtype == MBExchange::dtMTR )
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
						print_plist(mblog->level3(), it->second->slst)
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
		for( auto t = thrlist.begin(); t != thrlist.end(); ++t )
		{
			if( !isProcActive() )
				return false;

			IOBase::processingThreshold(&(*t), shm, force);
		}

		if( trReopen.hi(allNotRespond && exchangeMode != emSkipExchange) )
			ptReopen.reset();

		if( allNotRespond && exchangeMode != emSkipExchange && ptReopen.checkTime() )
		{
			mbwarn << myname << ": REOPEN timeout..(" << ptReopen.getInterval() << ")" << endl;

			mb = initMB(true);
			ptReopen.reset();
		}

		//    printMap(rmap);

		return !allNotRespond;
	}
	// -----------------------------------------------------------------------------
	bool MBExchange::RTUDevice::checkRespond( std::shared_ptr<DebugStream>& mblog )
	{
		bool prev = resp_state;
		resp_state = resp_Delay.check( prev_numreply != numreply ) && numreply != 0;

		mblog4 << "(checkRespond): addr=" << ModbusRTU::addr2str(mbaddr)
			   << " respond_id=" << resp_id
			   << " state=" << resp_state
			   << " check=" << (prev_numreply != numreply)
			   << " delay_check=" << resp_Delay.get()
			   << " [ timeout=" << resp_Delay.getOffDelay()
			   << " numreply=" << numreply
			   << " prev_numreply=" << prev_numreply
			   << " resp_ptInit=" << resp_ptInit.checkTime()
			   << " ]"
			   << endl;

		// если только что прошла "инициализация" возвращаем true
		// чтобы датчик в SM обновился..
		if( trInitOK.hi(resp_ptInit.checkTime()) )
			return true;

		return ((prev != resp_state || resp_force ) && trInitOK.get() );
	}
	// -----------------------------------------------------------------------------
	void MBExchange::updateRespondSensors()
	{
		for( const auto& it1 : devices )
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

			msleep(polltime);
		}
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const MBExchange::ExchangeMode& em )
	{
		if( em == MBExchange::emNone )
			return os << "emNone";

		if( em == MBExchange::emWriteOnly )
			return os << "emWriteOnly";

		if( em == MBExchange::emReadOnly )
			return os << "emReadOnly";

		if( em == MBExchange::emSkipSaveToSM )
			return os << "emSkipSaveToSM";

		if( em == MBExchange::emSkipExchange )
			return os << "emSkipExchange";

		return os;
	}
	// -----------------------------------------------------------------------------
	std::string to_string( const MBExchange::SafeMode& m )
	{
		if( m == MBExchange::safeNone )
			return "safeNone";

		if( m == MBExchange::safeResetIfNotRespond )
			return "safeResetIfNotRespond";

		if( m == MBExchange::safeExternalControl )
			return "safeExternalControl";

		return "";
	}

	ostream& operator<<( ostream& os, const MBExchange::SafeMode& m )
	{
		return os << to_string(m);
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
			<< endl;

		if( stat_time > 0 )
			inf << "Statistics: " << statInfo << endl;

		inf << "Devices: " << endl;

		for( const auto& it : devices )
			inf << "  " <<  it.second->getShortInfo() << endl;

		i->info = inf.str().c_str();
		return i._retn();
	}
	// ----------------------------------------------------------------------------
	std::string MBExchange::RTUDevice::getShortInfo() const
	{
		ostringstream s;

		s << "mbaddr=" << ModbusRTU::addr2str(mbaddr) << ":"
		  << " resp_state=" << resp_state
		  << " (resp_id=" << resp_id << " resp_force=" << resp_force
		  << " resp_invert=" << resp_invert
		  << " numreply=" << numreply
		  << " timeout=" << resp_Delay.getOffDelay()
		  << " type=" << dtype
		  << " ask_every_reg=" << ask_every_reg
		  << ")" << endl;
		return s.str();
	}
	// ----------------------------------------------------------------------------
} // end of namespace uniset
