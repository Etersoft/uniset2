// ------------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <cmath>
#include "SMonitor.h"
#include "Configuration.h"
#include "ORepHelpers.h"
#include "UniSetTypes.h"
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
	UniSetObject(id),
	script("")
{
	string sid(uniset_conf()->getArgParam("--sid"));

	lst = UniSetTypes::getSInfoList(sid, uniset_conf());

	if( lst.empty() )
		throw SystemError("Не задан список датчиков (--sid)");

	script = uniset_conf()->getArgParam("--script");
}


SMonitor::~SMonitor()
{
}
// ------------------------------------------------------------------------------------------
void SMonitor::sigterm( int signo )
{
	cout << myname << "SMonitor: sigterm " << endl;
}
// ------------------------------------------------------------------------------------------
void SMonitor::sysCommand( const SystemMessage* sm )
{
	switch(sm->command)
	{
		case SystemMessage::StartUp:
		{
			for( auto& it : lst )
			{
				if( it.si.node == DefaultObjectId )
					it.si.node = uniset_conf()->getLocalNode();

				try
				{
					if( it.si.id != DefaultObjectId )
						ui->askRemoteSensor(it.si.id, UniversalIO::UIONotify, it.si.node);
				}
				catch( const Exception& ex )
				{
					cerr << myname << ":(askSensor): " << ex << endl;
					raise(SIGTERM);
				}
				catch(...)
				{
					cerr << myname << ": НЕ СМОГ ЗАКАЗТЬ датчики " << endl;
					raise(SIGTERM);
				}
			}
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
void SMonitor::sensorInfo( const SensorMessage* si )
{
	auto conf = uniset_conf();

	string s_sup("");

	if( si->supplier == UniSetTypes::AdminID )
		s_sup = "uniset-admin";
	else
		s_sup = ORepHelpers::getShortName(conf->oind->getMapName(si->supplier));

	cout << "(" << setw(6) << si->id << "):"
		 << "[(" << std::right << setw(5) << si->supplier << ")"
		 << std::left << setw(20) << s_sup <<  "] "
		 << std::right << setw(8) << timeToString(si->sm_tv_sec, ":")
		 << "(" << setw(6) << si->sm_tv_usec << "): "
		 << std::right << setw(45) << conf->oind->getMapName(si->id)
		 << "    value:" << std::right << setw(9) << si->value
		 << "    fvalue:" << std::right << setw(12) << ( (float)si->value / pow(10.0, si->ci.precision) ) << endl;

	if( !script.empty() )
	{
		ostringstream cmd;

		// если задан полный путь или путь начиная с '.'
		// то берём как есть, иначе прибавляем bindir из файла настроек
		if( script[0] == '.' || script[0] == '/' )
			cmd << script;
		else
			cmd << conf->getBinDir() << script;

		cmd << " " << si->id << " " << si->value << " " << si->sm_tv_sec << " " << si->sm_tv_usec;

		int ret = system(cmd.str().c_str());
		int res = WEXITSTATUS(ret);
		if( res != 0 )
			cerr << "run script '" <<cmd.str() << "' failed.." << endl;

		//        if( WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
		//        {
		//            cout << "finish..." << endl;
		//        }
	}
}
// ------------------------------------------------------------------------------------------
void SMonitor::timerInfo( const UniSetTypes::TimerMessage* tm )
{

}
// ------------------------------------------------------------------------------------------
