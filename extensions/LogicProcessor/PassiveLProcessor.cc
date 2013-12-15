#include <iostream>
#include "Configuration.h"
#include "PassiveLProcessor.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
PassiveLProcessor::PassiveLProcessor( std::string lfile, UniSetTypes::ObjectId objId, 
										UniSetTypes::ObjectId shmID, SharedMemory* ic, const std::string& prefix ):
	UniSetObject_LT(objId),
	shm(0)
{
	logname = myname;
	shm = new SMInterface(shmID,&(UniSetObject_LT::ui),objId,ic);
	build(lfile);

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id",""); // it.getProp("heartbeat_id"));
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

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max","10", 10);
	}
}

PassiveLProcessor::~PassiveLProcessor()
{
	delete shm;
}
// -------------------------------------------------------------------------
void PassiveLProcessor::step()
{
	try
	{
		LProcessor::step();
	}
	catch(Exception& ex )
	{
		dlog[Debug::CRIT] << myname
			<< "(step): (hb) " << ex << std::endl;
	}
	
	if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSetValue(itHeartBeat,sidHeartBeat,maxHeartBeat,getId());
			ptHeartBeat.reset();
		}
		catch(Exception& ex)
		{
			dlog[Debug::CRIT] << myname
				<< "(step): (hb) " << ex << std::endl;
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
		for( EXTList::iterator it=extInputs.begin(); it!=extInputs.end(); ++it )
			shm->askSensor(it->sid,cmd);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << myname << "(askSensors): " << ex << endl;
		throw SystemError(myname +"(askSensors): do not ask sensors" );
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sensorInfo( UniSetTypes::SensorMessage*sm )
{
	for( EXTList::iterator it=extInputs.begin(); it!=extInputs.end(); ++it )
	{
		if( it->sid == sm->id )
			it->state = (bool)sm->value;
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::timerInfo( UniSetTypes::TimerMessage *tm )
{
	if( tm->id == tidStep )
		step();
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sysCommand( UniSetTypes::SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( !shm->waitSMready(smReadyTimeout) )
			{
				dlog[Debug::CRIT] << myname << "(ERR): SM not ready. Terminated... " << endl;
				raise(SIGTERM);
				return;
			}

			UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
			askSensors(UniversalIO::UIONotify);
			askTimer(tidStep,LProcessor::sleepTime);
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
			unideb << myname << "(sysCommand): logRotate" << std::endl;
			string fname = unideb.getLogFile();
			if( !fname.empty() )
			{
				unideb.logFile(fname);
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}

			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname);
				dlog << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
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
	for( OUTList::iterator it=extOuts.begin(); it!=extOuts.end(); ++it )
	{
		try
		{
			shm->setValue( it->sid,it->lnk->from->getOut() );
		}
		catch( Exception& ex )
		{
			dlog[Debug::CRIT] << myname << "(setOuts): " << ex << endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(setOuts): catch...\n";
		}
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sigterm( int signo )
{
	for( OUTList::iterator it=extOuts.begin(); it!=extOuts.end(); ++it )
	{
		try
		{
			shm->setValue(it->sid,0);
		}
		catch( Exception& ex )
		{
			dlog[Debug::CRIT] << myname << "(sigterm): " << ex << endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(sigterm): catch...\n";
		}
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::processingMessage( UniSetTypes::VoidMessage* msg )
{
	try
	{
		switch( msg->type )
		{
			case Message::SensorInfo:
			{
				SensorMessage sm( msg );
				sensorInfo( &sm );
				break;
			}

			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
				break;
			}

			case Message::SysCommand:
			{
				SystemMessage sm( msg );
				sysCommand( &sm );
				break;
			}

			default:
				break;
		}	
	}
	catch(Exception& ex)
	{
		dlog[Debug::CRIT]  << myname << "(processingMessage): " << ex << endl;
	}
}
// -----------------------------------------------------------------------------
