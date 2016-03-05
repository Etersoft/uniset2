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
extern "C" {
#include <rrd.h>
}
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "RRDServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
RRDServer::RRDServer(UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
					 const string& prefix ):
	UObject_SK(objId, cnode, string(prefix + "-")),
	prefix(prefix)
{
	auto conf = uniset_conf();

	if( ic )
		ic->logAgregator()->add(logAgregator());

	shm = make_shared<SMInterface>(shmId, ui, objId, ic);

	UniXML::iterator it(cnode);

	UniXML::iterator it1(cnode);

	if( !it1.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): empty rrd list...";
		mycrit << err.str() << endl;
		throw NameNotFound(err.str());
	}

	int tmID = 1;

	for( ; it1.getCurrent(); it1++, ++tmID )
		initRRD(it1, tmID);
}
// -----------------------------------------------------------------------------
RRDServer::~RRDServer()
{
}
// -----------------------------------------------------------------------------
void RRDServer::step()
{
}
//--------------------------------------------------------------------------------
void RRDServer::initRRD( xmlNode* cnode, int tmID )
{
	UniXML::iterator it(cnode);

	string fname( it.getProp("filename") );
	string ff( it.getProp("filter_field") );
	string fv( it.getProp("filter_value") );
	string cf( it.getProp("ds_field") );

	string ds(cf + "_dsname");

	if( cf.empty() )
	{
		cf = ff + fv + "_ds";
		ds = ff + fv + "_dsname";
	}

	int rrdstep = it.getPIntProp("step", 5);
	int lastup =  it.getPIntProp("lastup", 0);
	bool overwrite = it.getPIntProp("overwrite", 0);

	myinfo << myname << "(init): add rrd: file='" << fname
		   << " " << ff << "='" << fv
		   << "' create='" << cf << "'"
		   << " step=" << rrdstep
		   << endl;


	std::list<std::string> rralist;
	UniXML::iterator it_rra(cnode);

	if( !it_rra.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): rrd='" << fname << "' Unknown RRA list";
		mycrit << err.str() << endl;
		throw SystemError(err.str());
	}

	for(; it_rra.getCurrent(); it_rra++ )
	{
		string rra( it_rra.getProp("rra") );

		if( rra.empty() )
		{
			ostringstream err;
			err << myname << "(init): rrd='" << fname << "' Unkown RRA item.. <item rra='...'";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		rralist.push_back(rra);
	}

	if( rralist.empty() )
	{
		ostringstream err;
		err << myname << "(init): Not found RRA items...";
		mycrit << err.str() << endl;
		throw SystemError(err.str());
	}

	//    try
	{
		auto conf = uniset_conf();

		xmlNode* snode = conf->getXMLSensorsSection();

		if(!snode)
		{
			ostringstream err;
			err << myname << "(init): Not found section <sensors>";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		UniXML::iterator it1(snode);

		if( !it1.goChildren() )
		{
			ostringstream err;
			err << myname << "(init): section <sensors> empty?!";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		DSList dslist;

		// список параметров входящих в RRD
		std::list<std::string> rrdparamist;

		for(; it1.getCurrent(); it1.goNext() )
		{
			if( !UniSetTypes::check_filter(it1, ff, fv) )
				continue;

			std::string a(it1.getProp(cf));

			if( a.empty() )
			{
				ostringstream err;
				err << myname << "(init): Unknown create parameters ('" << cf << "')";
				mycrit << err.str() << endl;
				throw SystemError(err.str());
			}

			std::string dsname(it1.getProp(ds));

			if( dsname.empty() )
				dsname = it1.getProp("name");

			if( dsname.size() > RRD_MAX_DSNAME_LEN )
			{
				ostringstream err;
				err << myname << "(init): DSNAME=" << dsname << "(" << dsname.size()
					<< ") > RRD_MAX_NAME_SIZE(" << RRD_MAX_DSNAME_LEN << ")";
				mycrit << err.str() << endl;
				throw SystemError(err.str());
			}

			ostringstream nm;
			nm << "DS:" << dsname << ":" << a;
			rrdparamist.push_back(nm.str());

			ObjectId sid = conf->getSensorID( it1.getProp("name") );

			if( sid == DefaultObjectId )
			{
				ostringstream err;
				err << myname << "(init): Unknown SensorID for '" << it1.getProp("name") << "'";
				mycrit << err.str();
				throw SystemError(err.str());
			}

			auto ds = make_shared<DSInfo>(sid, dsname, it1.getIntProp("default"));
			dslist.push_back(ds);
		}

		if( rrdparamist.empty() )
		{
			ostringstream err;
			err << myname << "(init): Not found RRD items...";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		char argc = rrdparamist.size() + rralist.size();
		char** argv = new char* [ argc ];

		int k = 0;

		for( auto& i : rrdparamist )
			argv[k++] = strdup(i.c_str());

		for( auto& i : rralist )
			argv[k++] = strdup(i.c_str());

		//         for( k=0; k<argc; k++ )
		//             cout << "*** argv[" << k << "]='" << argv[k] << "'" << endl;

		// Собственно создаём RRD
		if( !overwrite && file_exist(fname) )
		{
			myinfo << myname << "(init): ignore create file='" << fname
				   << "'. File exist... overwrite=0." << endl;
		}
		else
		{
			rrd_clear_error();

			if( rrd_create_r(fname.c_str(), rrdstep, lastup, argc, (const char**)argv) < 0 )
			{
				ostringstream err;
				err << myname << "(init): Can`t create RRD ('" << fname << "'): err: " << string(rrd_get_error());
				mycrit << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		// Чистим выделенную память
		for( k = 0; k < argc; k++ )
			free( argv[k] );

		delete[] argv;

		rrdlist.emplace_back(fname, tmID, rrdstep, dslist);
	}
}
//--------------------------------------------------------------------------------
void RRDServer::help_print( int argc, const char* const* argv )
{
	cout << " Default prefix='rrd'" << endl;
	cout << "--prefix-name        - ID for rrdstorage. Default: RRDServer1. " << endl;
	cout << "--prefix-confnode    - configuration section name. Default: <NAME name='NAME'...> " << endl;
	cout << "--prefix-heartbeat-id name   - ID for heartbeat sensor." << endl;
	cout << "--prefix-heartbeat-max val   - max value for heartbeat sensor." << endl;
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
std::shared_ptr<RRDServer> RRDServer::init_rrdstorage(int argc, const char* const* argv,
		UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "RRDServer");

	if( name.empty() )
	{
		dcrit << "(RRDServer): Unknown name. Usage: --" <<  prefix << "-name" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == UniSetTypes::DefaultObjectId )
	{
		dcrit << "(RRDServer): Not found ID for '" << name
			  << " in '" << conf->getObjectsSection() << "' section" << endl;
		return 0;
	}

	string confname = conf->getArgParam("--" + prefix + "-confnode", name);
	xmlNode* cnode = conf->getNode(confname);

	if( !cnode )
	{
		dcrit << "(RRDServer): " << name << "(init): Not found <" + confname + ">" << endl;
		return 0;
	}

	dinfo << "(RRDServer): name = " << name << "(" << ID << ")" << endl;
	return make_shared<RRDServer>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void RRDServer::askSensors( UniversalIO::UIOCommand cmd )
{
	UObject_SK::askSensors(cmd);

	for( auto& it : rrdlist )
	{
		for( auto& s : it.dsmap )
		{
			try
			{
				shm->askSensor(s.first, cmd);
			}
			catch( const std::exception& ex )
			{
				mycrit << myname << "(askSensors): " << ex.what() << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void RRDServer::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	UObject_SK::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
	{
		for( auto && it : rrdlist )
		{
			try
			{
				askTimer(it.tid, it.sec * 1000);
			}
			catch( const std::exception& ex )
			{
				mycrit << myname << "(askTimer): " << ex.what() << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void RRDServer::sensorInfo( const UniSetTypes::SensorMessage* sm )
{
	for( auto& it : rrdlist )
	{
		auto s = it.dsmap.find(sm->id);

		if( s != it.dsmap.end() )
			s->second->value = sm->value;

		// продолжаем искать по другим rrd, т.к. датчик может входить в несколько..
	}
}
// -----------------------------------------------------------------------------
void RRDServer::timerInfo( const UniSetTypes::TimerMessage* tm )
{
	for( auto& it : rrdlist )
	{
		if( it.tid == tm->id )
		{
			ostringstream v;
			v << time(0);

			// здесь идём по списку (а не dsmap)
			// т.к. важна последовательность
			for( const auto& s : it.dslist )
				v << ":" << s->value;

			myinfo << myname << "(update): '" << it.filename << "' " << v.str() << endl;

			rrd_clear_error();

			const std::string tmp(v.str()); // надежда на RVO оптимизацию
			const char* argv = tmp.c_str();

			if( rrd_update_r(it.filename.c_str(), NULL, 1, &argv) < 0 )
			{
				ostringstream err;
				err << myname << "(update): Can`t update RRD ('" << it.filename << "'): err: " << string(rrd_get_error());
				mycrit << err.str() << endl;
			}

			break;
		}
	}
}
// -----------------------------------------------------------------------------
RRDServer::RRDInfo::RRDInfo(const string& fname, long tmID, long sec, const RRDServer::DSList& lst):
	filename(fname), tid(tmID), sec(sec), dslist(lst)
{
	// фомируем dsmap
	for( auto && i : dslist )
		dsmap.emplace(i->sid, i);
}
// -----------------------------------------------------------------------------
