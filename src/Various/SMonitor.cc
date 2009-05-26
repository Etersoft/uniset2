// $Id: SMonitor.cc,v 1.8 2008/02/10 03:21:42 vpashka Exp $
// ------------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "SMonitor.h"
#include "Configuration.h"
#include "ORepHelpers.h"
// ------------------------------------------------------------------------------------------
using namespace UniversalIO;
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
SMonitor::SMonitor():
	script("")
{
}

SMonitor::SMonitor(ObjectId id):
	UniSetObject_LT(id),
	script("")
{
	string sid(conf->getArgParam("--sid"));
	lst = explode(sid);

	if( lst.empty() )
		throw SystemError("Не задан список датчиков (--sid)");

	script = conf->getArgParam("--script");
}


SMonitor::~SMonitor()
{
}
// ------------------------------------------------------------------------------------------

void SMonitor::processingMessage( UniSetTypes::VoidMessage *msg)
{
	try
	{
		switch(msg->type)
		{
			case Message::Alarm:
			case Message::Info:
			case Message::SensorInfo:
			{
//				cout << myname << "(sensorMessage): type="<< msg->type << " prior=" << msg->priority;
//				cout << " sec=" << msg->tm.tv_sec << " usec=" << msg->tm.tv_usec << endl;
				SensorMessage sm(msg);
				sensorInfo(&sm);
				break;
			}

			case Message::SysCommand:
			{
				SystemMessage sm(msg);
				sysCommand(&sm);
				break;
			}
			
			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
				break;
			}		

			default:
				cout << myname << ": неизвестное сообщение  "<<  msg->type << endl;	
			break;

		}
	}
	catch(Exception& ex)
	{
		cerr << myname << ":(processing): " << ex << endl;
	}
	catch(...){}
}

// ------------------------------------------------------------------------------------------
void SMonitor::sigterm( int signo )
{
	cout << myname << "SMonitor: sigterm "<< endl;			
}
// ------------------------------------------------------------------------------------------
void SMonitor::sysCommand( SystemMessage *sm )
{
	switch(sm->command)
	{
		case SystemMessage::StartUp:
		{
			std::list<ObjectId> l = lst.getList();
			for( std::list<ObjectId>::iterator it=l.begin(); it!=l.end(); ++it )
			{
				try
				{
					if( (*it) != DefaultObjectId )
						ui.askSensor((*it),UniversalIO::UIONotify);
				}
				catch(Exception& ex)
				{
					cerr << myname << ":(askSensor): " << ex << endl;
					raise(SIGTERM);
				}
				catch(...)
				{
					cerr << myname << ": НЕ СМОГ ЗАКАЗТЬ датчики "<< endl;
					raise(SIGTERM);
				}
			}
/*
			try
			{
				UniSetTypes::IDSeq_var badlst = ui.askSensorsSeq(lst,UniversalIO::UIONotify);
				int sz = badlst->length();
				if( sz > 0 )
				{
					cerr << myname << "(askSensrosSeq): ******** НЕ УДАЛОСЬ ЗАКАЗАТЬ: ";
					for( int i=0; i<sz; i++ )
						cerr << badlst[i] << " ";
					cerr << endl;
				}			
			}
			catch(Exception& ex)
			{
				cerr << myname << ":(askSensor): " << ex << endl;
				raise(SIGTERM);
			}
			catch(...)
			{
				cerr << myname << ": НЕ СМОГ ЗАКАЗТЬ датчики "<< endl;
				raise(SIGTERM);
			}
*/
		}
		break;
						
		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			break;

		case SystemMessage::WatchDog:
			break;

		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
void SMonitor::sensorInfo( SensorMessage *si )
{
	cout << "(" << setw(6) << si->id << "): " << setw(8) << UniversalInterface::timeToString(si->sm_tv_sec,":") 
		 << "(" << setw(6) << si->sm_tv_usec << "): ";
	cout << setw(45) << conf->oind->getMapName(si->id);
	if( si->sensor_type == UniversalIO::DigitalInput || si->sensor_type == UniversalIO::DigitalOutput )
		cout << "\tstate=" << si->state << endl;
	else if( si->sensor_type == UniversalIO::AnalogInput || si->sensor_type == UniversalIO::AnalogOutput )
		cout << "\tvalue=" << si->value << endl;
	


	if( !script.empty() )
	{
		ostringstream cmd;
		// если задан полный путь или путь начиная с '.'
		// то берём как есть, иначе прибавляем bindir из файла настроек
		if( script[0] == '.' || script[0] == '/' )
			cmd << script;
		else
			cmd << conf->getBinDir() << script;	

		cmd << " " << si->id << " ";
		if( si->sensor_type == UniversalIO::DigitalInput || si->sensor_type == UniversalIO::DigitalOutput )
			cmd << si->state;
		else if( si->sensor_type == UniversalIO::AnalogInput || si->sensor_type == UniversalIO::AnalogOutput )
			cmd << si->value;

		cmd << " " << si->sm_tv_sec << " " << si->sm_tv_usec;

		int ret = system(cmd.str().c_str());
//		if( WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
//		{
//			cout << "finish..." << endl;
//		}
	}
}
// ------------------------------------------------------------------------------------------
void SMonitor::timerInfo( UniSetTypes::TimerMessage *tm )
{
		
}
// ------------------------------------------------------------------------------------------
