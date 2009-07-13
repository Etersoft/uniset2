#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "ObjectsActivator.h"
#include "PassiveLProcessor.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main(int argc, char **argv)
{
	try
	{
		string confile=UniSetTypes::getArgParam("--confile",argc,argv,"configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--logicproc-logfile"));
		if( logfilename.empty() )
			logfilename = "logicproc.log";
	
		conf->initDebug(dlog,"dlog");
	
		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		unideb.logFile( logname.str().c_str() );
		dlog.logFile( logname.str().c_str() );

		string schema = conf->getArgParam("--schema");
		if( schema.empty() )
		{
			cerr << "schema-file not defined. Use --schema" << endl;
			return 1;
		}

		ObjectId shmID = DefaultObjectId;
		string sID = conf->getArgParam("--smemory-id");
		if( !sID.empty() )
			shmID = conf->getControllerID(sID);
		else
			shmID = getSharedMemoryID();

		if( shmID == DefaultObjectId )
		{
			cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
			return 1;
		}

		cout << "init smemory: " << sID << " ID: " << shmID << endl; 
		
		string name = conf->getArgParam("--name","LProcessor");
		if( name.empty() )
		{
			cerr << "(plogicproc): Не задан name'" << endl;
			return 1;
		}

		ObjectId ID = conf->getObjectID(name);
		if( ID == UniSetTypes::DefaultObjectId )
		{
			cerr << "(plogicproc): идентификатор '" << name 
				<< "' не найден в конф. файле!"
				<< " в секции " << conf->getObjectsSection() << endl;
			return 1;
		}

		cout << "init name: " << name << " ID: " << ID << endl; 

		PassiveLProcessor plc(schema,ID,shmID);

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(&plc));

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- IOControl START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- IOControl START -------------------------\n\n";
		act.run(false);
		pause();
	}
	catch( LogicException& ex )
	{
		cerr << ex << endl;
	}
	catch( Exception& ex )
	{
		cerr << ex << endl;
	}
	catch( ... )
	{
		cerr << " catch ... " << endl;
	}
	
	return 0;
}
// -----------------------------------------------------------------------------
