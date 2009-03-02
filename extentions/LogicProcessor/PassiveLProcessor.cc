#include <iostream>
#include "Configuration.h"
#include "PassiveLProcessor.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtentions;
// -------------------------------------------------------------------------
PassiveLProcessor::PassiveLProcessor( std::string lfile, UniSetTypes::ObjectId objId, 
										UniSetTypes::ObjectId shmID, SharedMemory* ic ):
	UniSetObject_LT(objId),
	shm(0)
{
	shm = new SMInterface(shmID,&(UniSetObject_LT::ui),objId,ic);
	build(lfile);
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
			UniSetObject::ui.askState(it->sid,cmd);
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
			it->state = sm->state;
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
			if( !shm->waitSMready(10000) )
			{
				cerr << "(ERR): SM not ready. Terminated... " << endl;
				raise(SIGTERM);
				return;
			}

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
				unideb.logFile(fname.c_str());
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}

			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname.c_str());
				dlog << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::setOuts()
{
	// выcтавляем выходы
	for( OUTList::iterator it=extOuts.begin(); it!=extOuts.end(); ++it )
	{
		try
		{
			switch(it->iotype)
			{
				case UniversalIO::DigitalInput:
					UniSetObject::ui.saveState(it->sid,it->lnk->from->getOut(),it->iotype);
				break;

				case UniversalIO::DigitalOutput:
					UniSetObject::ui.setState(it->sid,it->lnk->from->getOut());
				break;
				
				default:
					cerr << "(LProcessor::setOuts): неподдерживаемый тип iotype=" << it->iotype << endl;
					break;
			}
		}
		catch( Exception& ex )
		{
			cerr << "(LProcessor::setOuts): " << ex << endl;
		}
		catch(...)
		{
			cerr << "(LProcessor::setOuts): catch...\n";
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
			switch(it->iotype)
			{
				case UniversalIO::DigitalInput:
					UniSetObject::ui.saveState(it->sid,false,it->iotype);
				break;

				case UniversalIO::DigitalOutput:
					UniSetObject::ui.setState(it->sid,false);
				break;
				
				default:
					cerr << "(LProcessor::sigterm): неподдерживаемый тип iotype=" << it->iotype << endl;
					break;
			}
		}
		catch( Exception& ex )
		{
			cerr << "(LProcessor::sigterm): " << ex << endl;
		}
		catch(...)
		{
			cerr << "(LProcessor::sigterm): catch...\n";
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
		cout  << myname << "(processingMessage): " << ex << endl;
	}
}
// -----------------------------------------------------------------------------
