// -----------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "SMDBServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
SMDBServer::SMDBServer( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic,
			const string prefix ):
DBServer_MySQL(objId),
prefix(prefix)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(SMDBServer): objId=-1?!! Use --" + prefix + "-name" );

//	xmlNode* cnode = conf->getNode(myname);
//	if( cnode == NULL )
//		throw UniSetTypes::SystemError("(SMDBServer): Not found conf-node for " + myname );
	xmlNode* cnode = conf->getNode("LocalDBServer");
	if( !cnode )
		throw NameNotFound(string(myname+"(init): <LocalDBServer> not found.."));

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);
	
	db_locale = conf->getArgParam("--" + prefix + "-locale",it.getProp("locale"));
	if( db_locale.empty() )
	    db_locale = "utf8";
	
	
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

		int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time",it.getProp("heartbeatTime"),conf->getHeartBeatTime());
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
	int ready_timeout = conf->getArgInt("--" + prefix + "-sm-ready-timeout","15000");
	if( ready_timeout == 0 )
		ready_timeout = 15000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout, 50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): Wait SharedMemory failed. [ " << ready_timeout << " msec ]";
		dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}
}
// -----------------------------------------------------------------------------
void SMDBServer::step()
{
//	DBServer_MySQL::step();
	
	if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSaveValue(aitHeartBeat,sidHeartBeat,maxHeartBeat,getId());
			ptHeartBeat.reset();
		}
		catch(Exception& ex)
		{
			dlog[Debug::CRIT] << myname << "(step): (hb) " << ex << std::endl;
		}
	}
}
//--------------------------------------------------------------------------------
void SMDBServer::initDB( DBInterface *db )
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
			dlog[Debug::CRIT] << myname << ": section <sensors> not found.." << endl;
			return;
		}

			UniXML_iterator it(snode);
			if( !it.goChildren() )
			{
				dlog[Debug::CRIT] << myname << ": section <sensors> empty?!.." << endl;
				return;
				
			}

			for(;it.getCurrent(); it.goNext() )
			{
				// ??. DBTABLE ObjectsMap 	
				std::ostringstream data;
				data << " VALUES('";							// ???? ???????
				data << it.getProp("textname") << "','";	// name 
				data << it.getProp("name") << "','";		// rep_name 
				data << it.getProp("id") << "','";			// id (sensorid)
				data << it.getIntProp("msg") << "')";			// msg [0:1]

				if( !writeToBase("INSERT IGNORE INTO ObjectsMap(name,rep_name,id,msg)"+data.str()) )
				{
					dlog[Debug::CRIT] << myname <<  "(insert) ObjectsMap msg error: "<< db->error() << std::endl;
					db->freeResult();
				}
			}
	}
	catch( Exception& ex )
	{	
		dlog[Debug::CRIT] << myname << "(filling ObjectsMap): " << ex << std::endl;
	}
	catch( ...  )
	{	
		dlog[Debug::CRIT] << myname << "(filling ObjectsMap): catch ..." << std::endl;
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
											UniSetTypes::ObjectId icID, SharedMemory* ic, 
											const std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","DBServer");
	if( name.empty() )
	{
		cerr << "(SMDBServer): Unknown name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getServiceID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(SMDBServer): Not found ID for '" << name 
			<< " in '" << conf->getServicesSection() << "' section" << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(SMDBServer): name = " << name << "(" << ID << ")" << endl;
	return new SMDBServer(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
