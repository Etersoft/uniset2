// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBExchange::MBExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, 
							SharedMemory* ic, const std::string prefix ):
UniSetObject_LT(objId),
shm(0),
initPause(0),
force(false),
force_out(false),
mbregFromID(false),
sidExchangeMode(DefaultObjectId),
activated(false),
noQueryOptimization(false),
prefix(prefix)
{
//	cout << "$ $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBExchange): objId=-1?!! Use --" + prefix + "-name" );

//	xmlNode* cnode = conf->getNode(myname);
	string conf_name = conf->getArgParam("--" + prefix + "-confnode",myname);

	cnode = conf->getNode(conf_name);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBExchange): Not found node <" + conf_name + " ...> for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	stat_time = conf->getArgPInt("--" + prefix + "-statistic-sec",it.getProp("statistic_sec"),0);
	if( stat_time > 0 )
		ptStatistic.setTiming(stat_time*1000);

//	recv_timeout = conf->getArgPInt("--" + prefix + "-recv-timeout",it.getProp("recv_timeout"), 500);
//
//	int tout = conf->getArgPInt("--" + prefix + "-timeout",it.getProp("timeout"), 5000);
//	ptTimeout.setTiming(tout);

	noQueryOptimization = conf->getArgInt("--" + prefix + "-no-query-optimization",it.getProp("no_query_optimization"));

	mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id",it.getProp("reg_from_id"));
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	polltime = conf->getArgPInt("--" + prefix + "-polltime",it.getProp("polltime"), 100);

	initPause = conf->getArgPInt("--" + prefix + "-initPause",it.getProp("initPause"), 3000);

	sleepPause_usec = conf->getArgPInt("--" + prefix + "-sleepPause-usec",it.getProp("slepePause"), 100);

	force = conf->getArgInt("--" + prefix + "-force",it.getProp("force"));
	force_out = conf->getArgInt("--" + prefix + "-force-out",it.getProp("force_out"));


	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);
		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('HeartBeat') for " << heart;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = getHeartBeatTime();
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
			err << myname << "(init): test_id unknown. 'TestMode_S' not found...";
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	dlog[Debug::INFO] << myname << "(init): test_id=" << test_id << endl;

	string emode = conf->getArgParam("--" + prefix + "-exchange-mode-id",it.getProp("exchangeModeID"));
	if( !emode.empty() )
	{
		sidExchangeMode = conf->getSensorID(emode);
		if( sidExchangeMode == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('ExchangeMode') for " << emode;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	activateTimeout	= conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);
}
// -----------------------------------------------------------------------------
void MBExchange::help_print( int argc, const char* const* argv )
{
	cout << "--prefix-name name              - ObjectId (имя) процесса. По умолчанию: MBTCPMaster1" << endl;
	cout << "--prefix-confnode name          - Настроечная секция в конф. файле <name>. " << endl;
	cout << "--prefix-polltime msec          - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--prefix-heartbeat-id  name     - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--prefix-heartbeat-max val      - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--prefix-ready-timeout msec     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
	cout << "--prefix-force 0,1              - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
	cout << "--prefix-force-out 0,1          - Считывать значения 'выходов' кажый раз SM (а не по изменению)" << endl;
	cout << "--prefix-initPause msec         - Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--prefix-no-query-optimization 0,1 - Не оптимизировать запросы (не объединять соседние регистры в один запрос)" << endl;
	cout << "--prefix-reg-from-id 0,1        - Использовать в качестве регистра sensor ID" << endl;
	cout << "--prefix-filter-field name      - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
	cout << "--prefix-filter-value val       - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
	cout << "--prefix-statistic-sec sec      - Выводить статистику опроса каждые sec секунд" << endl;
	cout << "--prefix-sm-ready-timeout       - время на ожидание старта SM" << endl;
}
// -----------------------------------------------------------------------------
MBExchange::~MBExchange()
{
	delete shm;
}
// -----------------------------------------------------------------------------
void MBExchange::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--" + prefix +"-sm-ready-timeout","15000");
	if( ready_timeout == 0 )
		ready_timeout = 15000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout, 50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): failed waiting SharedMemory " << ready_timeout << " msec";
		dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}
}
// -----------------------------------------------------------------------------
void MBExchange::step()
{
	if( !checkProcActive() )
		return;

	if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSaveValue(aitHeartBeat,sidHeartBeat,maxHeartBeat,getId());
			ptHeartBeat.reset();
		}
		catch(Exception& ex)
		{
			dlog[Debug::CRIT] << myname
				<< "(step): (hb) " << ex << std::endl;
		}
	}
}

// -----------------------------------------------------------------------------
bool MBExchange::checkProcActive()
{
	uniset_mutex_lock l(actMutex, 300);
	return activated;
}
// -----------------------------------------------------------------------------
void MBExchange::setProcActive( bool st )
{
	uniset_mutex_lock l(actMutex, 400);
	activated = st;
}
// -----------------------------------------------------------------------------
void MBExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	if( force_out )
		return;

	if( sm->id == sidExchangeMode )
	{
		exchangeMode = sm->value;
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(sensorInfo): exchange MODE=" << sm->value << std::endl;
		return;
	}
}
// ------------------------------------------------------------------------------------------
void MBExchange::sigterm( int signo )
{
	dlog[Debug::WARN] << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	setProcActive(false);
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void MBExchange::readConfiguration()
{
//	readconf_ok = false;
	xmlNode* root = conf->getXMLSensorsSection();
	if(!root)
	{
		ostringstream err;
		err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
		throw SystemError(err.str());
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		dlog[Debug::CRIT] << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
			initItem(it);
	}
	
//	readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------
MBExchange::DeviceType MBExchange::getDeviceType( const std::string dtype )
{
	if( dtype.empty() )
		return dtUnknown;
	
	if( dtype == "mtr" || dtype == "MTR" )
		return dtMTR;
	
	if( dtype == "rtu" || dtype == "RTU" )
		return dtRTU;
	
	return dtUnknown;
}
// ------------------------------------------------------------------------------------------
void MBExchange::initIterators()
{
	shm->initAIterator(aitHeartBeat);
	shm->initAIterator(aitExchangeMode);
}
// -----------------------------------------------------------------------------
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
		
		default:
			os << "Unknown device type (" << (int)dt << ")";
		break;
	}
	
	return os;
}
// -----------------------------------------------------------------------------
bool MBExchange::checkUpdateSM( bool wrFunc )
{
	if( wrFunc && exchangeMode == emReadOnly )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emReadOnly' " << endl;
		return false;
	}

	if( !wrFunc && exchangeMode == emWriteOnly )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emWriteOnly' " << endl;
		return false;
	}
	
	if( wrFunc && exchangeMode == emSkipSaveToSM )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emSkipSaveToSM' " << endl;
		return false;
	}

	return true;
}
// -----------------------------------------------------------------------------
bool MBExchange::checkPoll( bool wrFunc )
{
	if( exchangeMode == emWriteOnly && !wrFunc )
	{
		if( dlog.debugging(Debug::LEVEL3)  )
			dlog[Debug::LEVEL3] << myname << "(checkPoll): skip.. mode='emWriteOnly'" << endl;
		return false;
	}
	
	if( exchangeMode == emReadOnly && wrFunc )
	{
		if( dlog.debugging(Debug::LEVEL3)  )
			dlog[Debug::LEVEL3] << myname << "(checkPoll): skip.. poll mode='emReadOnly'" << endl;

		return false;
	}

	return true;
}
// -----------------------------------------------------------------------------
