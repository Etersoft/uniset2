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
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "SMDBServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
SMDBServer::SMDBServer( uniset::ObjectId objId, uniset::ObjectId shmId, SharedMemory* ic,
						const string& prefix ):
	DBServer_MySQL(objId),
	aiignore(false),
	prefix(prefix)
{
	if( objId == DefaultObjectId )
		throw uniset::SystemError("(SMDBServer): objId=-1?!! Use --" + prefix + "-name" );

	//    xmlNode* cnode = conf->getNode(myname);
	//    if( cnode == NULL )
	//        throw uniset::SystemError("(SMDBServer): Not found conf-node for " + myname );
	xmlNode* cnode = conf->getNode("LocalDBServer");

	if( !cnode )
		throw NameNotFound(string(myname + "(init): <LocalDBServer> not found.."));

	shm = new SMInterface(shmId, &ui, objId, ic);

	UniXML::iterator it(cnode);

	db_locale = conf->getArgParam("--" + prefix + "-locale", it.getProp("locale"));

	if( db_locale.empty() )
		db_locale = "utf8";


	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", it.getProp("heartbeat_id"));

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

		int heartbeatTime = getHeartBeatTime();

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
			dcrit << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	dinfo << myname << "(init): test_id=" << test_id << endl;
}
// -----------------------------------------------------------------------------
SMDBServer::~SMDBServer()
{
	delete shm;
}
// -----------------------------------------------------------------------------
void SMDBServer::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--" + prefix + "-sm-ready-timeout", "120000");

	if( ready_timeout == 0 )
		ready_timeout = 120000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout, 50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): Wait SharedMemory failed. [ " << ready_timeout << " msec ]";
		dcrit << err.str() << endl;
		//throw SystemError(err.str());
		std::terminate();
	}
}
// -----------------------------------------------------------------------------
void SMDBServer::step()
{
	//    DBServer_MySQL::step();

	if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSaveValue(aitHeartBeat, sidHeartBeat, maxHeartBeat, getId());
			ptHeartBeat.reset();
		}
		catch( const uniset::Exception& ex )
		{
			dcrit << myname << "(step): (hb) " << ex << std::endl;
		}
	}
}
//--------------------------------------------------------------------------------
void SMDBServer::initDB( DBInterface* db )
{
	{
		std::ostringstream q;
		q << "SET NAMES " << db_locale;
		db->query(q.str());
	}

	{
		std::ostringstream q;
		q << "SET CHARACTER SET " << db_locale;
		db->query(q.str());
	}

	try
	{
		xmlNode* snode = conf->getXMLSensorsSection();

		if(!snode)
		{
			dcrit << myname << ": section <sensors> not found.." << endl;
			return;
		}

		UniXML::iterator it(snode);

		if( !it.goChildren() )
		{
			dcrit << myname << ": section <sensors> empty?!.." << endl;
			return;
		}

		for(; it.getCurrent(); it.goNext() )
		{
			// ??. DBTABLE ObjectsMap
			std::ostringstream data;
			data << " VALUES('";                            // ???? ???????
			data << it.getProp("textname") << "','";    // name
			data << it.getProp("name") << "','";        // rep_name
			data << it.getProp("id") << "','";            // id (sensorid)
			data << it.getIntProp("msg") << "')";            // msg [0:1]

			if( !writeToBase("INSERT IGNORE INTO ObjectsMap(name,rep_name,id,msg)" + data.str()) )
			{
				dcrit << myname <<  "(insert) ObjectsMap msg error: " << db->error() << std::endl;
				db->freeResult();
			}
		}
	}
	catch( const uniset::Exception& ex )
	{
		dcrit << myname << "(filling ObjectsMap): " << ex << std::endl;
	}
	catch( ...  )
	{
		dcrit << myname << "(filling ObjectsMap): catch ..." << std::endl;
	}
}
//--------------------------------------------------------------------------------
void SMDBServer::help_print( int argc, const char* const* argv )
{
	cout << "--dbserver-name    - ID for dbserver. Default: SMDBServer1. " << endl;
	cout << "--dbserver-locale name   - DB locale. Default: koi8-r. " << endl;
	cout << "--dbserver-heartbeat-id name   - ID for heartbeat sensor." << endl;
	cout << "--dbserver-heartbeat-max val   - max value for heartbeat sensor." << endl;
}
// -----------------------------------------------------------------------------
SMDBServer* SMDBServer::init_smdbserver( int argc, const char* const* argv,
		uniset::ObjectId icID, SharedMemory* ic,
		const std::string& prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name", "DBServer");

	if( name.empty() )
	{
		cerr << "(SMDBServer): Unknown name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getServiceID(name);

	if( ID == uniset::DefaultObjectId )
	{
		cerr << "(SMDBServer): Not found ID for '" << name
			 << " in '" << conf->getServicesSection() << "' section" << endl;
		return 0;
	}

	dinfo << "(SMDBServer): name = " << name << "(" << ID << ")" << endl;
	return new SMDBServer(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
