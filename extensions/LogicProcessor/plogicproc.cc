#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "UniSetActivator.h"
#include "PassiveLProcessor.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main(int argc, const char **argv)
{
    std::ios::sync_with_stdio(false);
    try
    {
        auto conf = uniset_init( argc, argv );

        string logfilename(conf->getArgParam("--logicproc-logfile"));
        if( logfilename.empty() )
            logfilename = "logicproc.log";

        conf->initDebug(dlog(),"dlog");

        std::ostringstream logname;
        string dir(conf->getLogDir());
        logname << dir << logfilename;
        ulog()->logFile( logname.str() );
        dlog()->logFile( logname.str() );

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

        auto act = UniSetActivator::Instance();
        act->add(plc.get_ptr());

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(main): -------------- IOControl START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- IOControl START -------------------------\n\n";
        act->run(false);
        return 0;
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

    return 1;
}
// -----------------------------------------------------------------------------
