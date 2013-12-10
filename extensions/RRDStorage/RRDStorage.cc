// -----------------------------------------------------------------------------
extern "C" {
#include <rrd.h>
}
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "extensions/Extensions.h"
#include "RRDStorage.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
RRDStorage::RRDStorage( UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmId, SharedMemory* ic,
			const string prefix ):
UObject_SK(objId,cnode),
shm( new SMInterface(shmId,&ui,objId,ic) ),
prefix(prefix)
{
	UniXML::iterator it(cnode);

	UniXML::iterator it1(cnode);
	if( !it1.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): empty rrd list...";
		RRDStorage::dlog[Debug::CRIT] << err.str() << endl;
		throw NameNotFound(err.str());
	}

	int tmID=1;
	for( ;it1.getCurrent(); it1++,tmID++ )
		initRRD(it1,tmID);
}
// -----------------------------------------------------------------------------
RRDStorage::~RRDStorage()
{
}
// -----------------------------------------------------------------------------
void RRDStorage::step()
{
}
//--------------------------------------------------------------------------------
void RRDStorage::initRRD( xmlNode* cnode, int tmID )
{
	UniXML::iterator it(cnode);

	string fname( it.getProp("filename") );
	string ff( it.getProp("filter_field") );
	string fv( it.getProp("filter_value") );
	string cf( it.getProp("ds_field") );
	if( cf.empty() )
		cf = ff+fv+"_ds";

	int rrdstep = it.getPIntProp("step",5);
	int lastup =  it.getPIntProp("lastup",0);

	RRDStorage::dlog[Debug::INFO] << myname << "(init): add rrd: file='" << fname 
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
		RRDStorage::dlog[Debug::CRIT] << err.str();
		throw SystemError(err.str());
	}

	for(;it_rra.getCurrent(); it_rra++ )
	{
		string rra( it_rra.getProp("rra") );
		if( rra.empty() )
		{
			ostringstream err;
			err << myname << "(init): rrd='" << fname << "' Unkown RRA item.. <item rra='...'";
			RRDStorage::dlog[Debug::CRIT] << err.str();
			throw SystemError(err.str());
		}

		rralist.push_back(rra);
	}

	if( rralist.empty() )
	{
		ostringstream err;
		err << myname << "(init): Not found RRA items...";
		RRDStorage::dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}

//	try
	{

		xmlNode* snode = conf->getXMLSensorsSection();
		if(!snode)
		{
			ostringstream err;
			err << myname << "(init): Not found section <sensors>";
			RRDStorage::dlog[Debug::CRIT] << err.str();
			throw SystemError(err.str());
		}

		UniXML_iterator it1(snode);
		if( !it1.goChildren() )
		{
			ostringstream err;
			err << myname << "(init): section <sensors> empty?!";
			RRDStorage::dlog[Debug::CRIT] << err.str();
			throw SystemError(err.str());
		}

		DSMap dsmap;

		// список параметров входящих в RRD
		std::list<std::string> dslist;

		for(;it1.getCurrent(); it1.goNext() )
		{
			if( !UniSetTypes::check_filter(it1,ff,fv) )
				continue;

			std::string a(it1.getProp(cf));
			if( a.empty() )
			{
				ostringstream err;
				err << myname << "(init): Unknown create parameters ('" << cf << "')";
				RRDStorage::dlog[Debug::CRIT] << err.str();
				throw SystemError(err.str());
			}

			std::string dsname(it1.getProp("name"));
			ostringstream nm;
			nm << "DS:" << dsname << ":" << a;
			dslist.push_back(nm.str());

			ObjectId sid = conf->getSensorID( dsname );
			if( sid == DefaultObjectId )
			{
				ostringstream err;
				err << myname << "(init): Unknown SensorID for '" << dsname << "'";
				RRDStorage::dlog[Debug::CRIT] << err.str();
				throw SystemError(err.str());
			}

			DSInfo ds(dsname,it1.getIntProp("default"));
			dsmap.insert( DSMap::value_type(sid,ds) );
		}

		if( dslist.empty() )
		{
			ostringstream err;
			err << myname << "(init): Not found RRD items...";
			RRDStorage::dlog[Debug::CRIT] << err.str() << endl;
			throw SystemError(err.str());
		}

		char argc = dslist.size() + rralist.size();
		char** argv = new char*[ argc ];

		int k=0;
		for( std::list<std::string>::iterator i=dslist.begin(); i!=dslist.end(); ++i,k++ )
			argv[k] = strdup(i->c_str());

		for( std::list<std::string>::iterator i=rralist.begin(); i!=rralist.end(); ++i,k++ )
			argv[k] = strdup(i->c_str());

// 		for( k=0; k<argc; k++ )
// 			cout << "*** argv[" << k << "]='" << argv[k] << "'" << endl;

		// Собственно создаём RRD
 		rrd_clear_error();
		if( rrd_create_r(fname.c_str(),rrdstep,lastup,argc,(const char**)argv) < 0 )
		{
			ostringstream err;
			err << myname << "(init): Can`t create RRD ('" << fname << "'): err: " << string(rrd_get_error());
			RRDStorage::dlog[Debug::CRIT] << err.str() << endl;
			throw SystemError(err.str());
		}

		// Чистим выделенную память
		for( k=0; k<argc; k++ )
			free( argv[k] );

		delete[] argv;

		RRDInfo rrd(fname,tmID,rrdstep,dsmap);
		rrdlist.push_back(rrd);
	}
/*	catch( Exception& ex )
	{	
		RRDStorage::dlog[Debug::CRIT] << myname << "(init) " << ex << std::endl;
	}
	catch( ...  )
	{	
		RRDStorage::dlog[Debug::CRIT] << myname << "(init): catch ..." << std::endl;
	}
*/
}
//--------------------------------------------------------------------------------
void RRDStorage::help_print( int argc, const char* const* argv )
{
	cout << "--rrdstorage-name    - ID for rrdstorage. Default: RRDStorage1. " << endl;
	cout << "--rrdstorage-locale name   - DB locale. Default: koi8-r. " << endl;
	cout << "--rrdstorage-heartbeat-id name   - ID for heartbeat sensor." << endl;
	cout << "--rrdstorage-heartbeat-max val   - max value for heartbeat sensor." << endl;
}
// -----------------------------------------------------------------------------
RRDStorage* RRDStorage::init_rrdstorage( int argc, const char* const* argv, 
											UniSetTypes::ObjectId icID, SharedMemory* ic, 
											const std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","RRDStorage");
	if( name.empty() )
	{
		UniSetExtensions::dlog[Debug::CRIT] << "(RRDStorage): Unknown name. Usage: --" <<  prefix << "-name" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		UniSetExtensions::dlog[Debug::CRIT] << "(RRDStorage): Not found ID for '" << name 
			<< " in '" << conf->getObjectsSection() << "' section" << endl;
		return 0;
	}

	string confname = conf->getArgParam("--" + prefix + "-confnode",name);
	xmlNode* cnode = conf->getNode(confname);
	if( !cnode )
	{
		UniSetExtensions::dlog[Debug::CRIT] << "(RRDStorage): " << name << "(init): Not found <" + confname + ">" << endl;
		return 0;
	}

	UniSetExtensions::dlog[Debug::INFO] << "(RRDStorage): name = " << name << "(" << ID << ")" << endl;
	return new RRDStorage(ID,cnode,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
void RRDStorage::askSensors( UniversalIO::UIOCommand cmd )
{
	UObject_SK::askSensors(cmd);

	for( RRDList::iterator it=rrdlist.begin(); it!=rrdlist.end(); ++it )
	{
		for( DSMap::iterator s=it->dsmap.begin(); s!=it->dsmap.end(); ++s )
		{
			try
			{
				shm->askSensor(s->first,cmd);
			}	
			catch( std::exception& ex )
			{
				RRDStorage::dlog[Debug::CRIT] << myname << "(askSensors): " << ex.what() << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void RRDStorage::sysCommand( UniSetTypes::SystemMessage* sm )
{
	UObject_SK::sysCommand(sm);
	if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
	{
		for( RRDList::iterator it=rrdlist.begin(); it!=rrdlist.end(); ++it )
		{
			try
			{
				askTimer(it->tid,it->sec*1000);
			}	
			catch( std::exception& ex )
			{
				RRDStorage::dlog[Debug::CRIT] << myname << "(askTimer): " << ex.what() << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void RRDStorage::sensorInfo( UniSetTypes::SensorMessage* sm )
{

}
// -----------------------------------------------------------------------------
void RRDStorage::timerInfo( UniSetTypes::TimerMessage* tm )
{
	for( RRDList::iterator it=rrdlist.begin(); it!=rrdlist.end(); ++it )
	{
		if( it->tid == tm->id )
		{
			ostringstream v;
			v << time(0);

			for( DSMap::iterator s=it->dsmap.begin(); s!=it->dsmap.end(); ++s )
				v << ":" << s->second.value;

			if( RRDStorage::dlog.debugging(Debug::INFO) )
				RRDStorage::dlog[Debug::INFO] << myname << "(update): '" << it->filename << "' " << v.str() << endl;

 			rrd_clear_error();
			const char* argv = v.str().c_str();
			if( rrd_update_r(it->filename.c_str(),NULL,1,&argv) < 0 )
			{
				ostringstream err;
				err << myname << "(update): Can`t update RRD ('" << it->filename << "'): err: " << string(rrd_get_error());
				RRDStorage::dlog[Debug::CRIT] << err.str() << endl;
			}

			break;
		}
	}
}
// -----------------------------------------------------------------------------

