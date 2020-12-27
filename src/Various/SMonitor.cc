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
using namespace uniset;
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

	lst = uniset::getSInfoList(sid, uniset_conf());

	if( lst.empty() )
		throw SystemError("Не задан список датчиков (--sid)");

	script = uniset_conf()->getArgParam("--script");
}


SMonitor::~SMonitor()
{
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
				catch( const uniset::Exception& ex )
				{
					cerr << myname << ":(askSensor): " << ex << endl;
					//					raise(SIGTERM);
					//std::terminate();
					uterminate();
				}
				catch(...)
				{
					std::exception_ptr p = std::current_exception();
					cerr << myname << ": " << (p ? p.__cxa_exception_type()->name() : "FAIL ask sensors..") << std::endl;
					uterminate();
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
std::string SMonitor::printEvent( const uniset::SensorMessage* sm )
{
	auto conf = uniset_conf();
	ostringstream s;

	string s_sup("");

	if( sm->supplier == uniset::AdminID )
		s_sup = "uniset-admin";
	else
		s_sup = ORepHelpers::getShortName(conf->oind->getMapName(sm->supplier));

	s << "(" << setw(6) << sm->id << "):"
	  << "[(" << std::right << setw(5) << sm->supplier << ")"
	  << std::left << setw(20) << s_sup <<  "] "
	  << std::right << setw(8) << timeToString(sm->sm_tv.tv_sec, ":")
	  << "(" << setw(6) << sm->sm_tv.tv_nsec << "): "
	  << std::right << setw(45) << conf->oind->getMapName(sm->id)
	  << "    value:" << std::right << setw(9) << sm->value
	  << "    fvalue:" << std::right << setw(12) << ( (float)sm->value / pow(10.0, sm->ci.precision) ) << endl;

	return s.str();
}
// ------------------------------------------------------------------------------------------
void SMonitor::sensorInfo( const SensorMessage* si )
{
	cout << printEvent(si) << endl;

	if( !script.empty() )
	{
		ostringstream cmd;

		// если задан полный путь или путь начиная с '.'
		// то берём как есть, иначе прибавляем bindir из файла настроек
		if( script[0] == '.' || script[0] == '/' )
			cmd << script;
		else
			cmd << uniset_conf()->getBinDir() << script;

		cmd << " " << si->id << " " << si->value << " " << si->sm_tv.tv_sec << " " << si->sm_tv.tv_nsec;

		int ret = system(cmd.str().c_str());
		int res = WEXITSTATUS(ret);

		if( res != 0 )
			cerr << "run script '" << cmd.str() << "' failed.." << endl;

		//        if( WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
		//        {
		//            cout << "finish..." << endl;
		//        }
	}
}
// ------------------------------------------------------------------------------------------
