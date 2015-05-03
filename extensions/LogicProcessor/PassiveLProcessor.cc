#include <iostream>
#include "Configuration.h"
#include "PassiveLProcessor.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
PassiveLProcessor::PassiveLProcessor( std::string lfile, UniSetTypes::ObjectId objId,
									  UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory> ic, const std::string& prefix ):
	UniSetObject_LT(objId),
	shm(nullptr)
{
	auto conf = uniset_conf();

	logname = myname;
	shm = make_shared<SMInterface>(shmID, UniSetObject_LT::ui, objId, ic);
	build(lfile);

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", ""); // it.getProp("heartbeat_id"));

	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);

		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('HeartBeat') for " << heart;
			dcrit << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time", conf->getHeartBeatTime());

		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", "10", 10);
	}
}

PassiveLProcessor::~PassiveLProcessor()
{

}
// -------------------------------------------------------------------------
void PassiveLProcessor::step()
{
	try
	{
		LProcessor::step();
	}
	catch( const Exception& ex )
	{
		dcrit << myname << "(step): (hb) " << ex << std::endl;
	}

	if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
			ptHeartBeat.reset();
		}
		catch( const Exception& ex )
		{
			dcrit << myname << "(step): (hb) " << ex << std::endl;
		}
	}

}
// -------------------------------------------------------------------------
void PassiveLProcessor::getInputs()
{

}
// -------------------------------------------------------------------------
void PassiveLProcessor::askSensors( UniversalIO::UIOCommand cmd )
{
	try
	{
		for( auto& it : extInputs )
			shm->askSensor(it.sid, cmd);
	}
	catch( const Exception& ex )
	{
		dcrit << myname << "(askSensors): " << ex << endl;
		throw SystemError(myname + "(askSensors): do not ask sensors" );
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sensorInfo( const UniSetTypes::SensorMessage* sm )
{
	for( auto& it : extInputs )
	{
		if( it.sid == sm->id )
			it.state = sm->value ? true : false;
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::timerInfo( const UniSetTypes::TimerMessage* tm )
{
	if( tm->id == tidStep )
		step();
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( !shm->waitSMready(smReadyTimeout) )
			{
				dcrit << myname << "(ERR): SM not ready. Terminated... " << endl;
				raise(SIGTERM);
				return;
			}

			UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
			askSensors(UniversalIO::UIONotify);
			askTimer(tidStep, LProcessor::sleepTime);
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
			// (т.е. RTUExchange  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(RTUExchange)
			if( shm->isLocalwork() )
				break;

			askSensors(UniversalIO::UIONotify);
		}
		break;

		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			ulogany << myname << "(sysCommand): logRotate" << std::endl;
			string fname (ulog()->getLogFile() );

			if( !fname.empty() )
			{
				ulog()->logFile(fname, true);
				ulogany << myname << "(sysCommand): ***************** ulog LOG ROTATE *****************" << std::endl;
			}

			dlogany << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog()->getLogFile();

			if( !fname.empty() )
			{
				dlog()->logFile(fname, true);
				dlogany << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// -------------------------------------------------------------------------
bool PassiveLProcessor::activateObject()
{
	// блокирование обработки Starsp
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		UniSetTypes::uniset_mutex_lock l(mutex_start, 5000);
		UniSetObject_LT::activateObject();
		initIterators();
	}

	return true;
}
// ------------------------------------------------------------------------------------------
void PassiveLProcessor::initIterators()
{
	shm->initIterator(itHeartBeat);
}
// -------------------------------------------------------------------------
void PassiveLProcessor::setOuts()
{
	// выcтавляем выходы
	for( auto& it : extOuts )
	{
		try
		{
			shm->setValue( it.sid, it.lnk->from->getOut() );
		}
		catch( const Exception& ex )
		{
			dcrit << myname << "(setOuts): " << ex << endl;
		}
		catch( const std::exception& ex )
		{
			dcrit << myname << "(setOuts): catch: " << ex.what() << endl;
		}
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sigterm( int signo )
{
	for( auto& it : extOuts )
	{
		try
		{
			shm->setValue(it.sid, 0);
		}
		catch( const Exception& ex )
		{
			dcrit << myname << "(sigterm): " << ex << endl;
		}
		catch( const std::exception& ex )
		{
			dcrit << myname << "(sigterm): catch:" << ex.what() << endl;
		}
	}
}
// -------------------------------------------------------------------------
