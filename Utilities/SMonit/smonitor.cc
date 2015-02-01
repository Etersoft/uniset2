#include <iostream>
#include <string>
#include "UniSetActivator.h"
#include "Configuration.h"
#include "SMonitor.h"
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, const char **argv )
{
    std::ios::sync_with_stdio(false);
    try
    {
        if( argc>1 && ( !strcmp(argv[1],"--help") || !strcmp(argv[1],"-h") ) )
        {
            cout << "Usage: uniset-smonit [ args ] --sid id1@node1,Sensor2@node2,id2,sensorname3,... "
//                 << " --script scriptname \n"
                 << " --confile configure.xml \n";
            return 0;
        }

        auto conf = uniset_init(argc,argv,"configure.xml");

        ObjectId ID(DefaultObjectId);
        string name = conf->getArgParam("--name", "TestProc");

        ID = conf->getObjectID(name);
        if( ID == UniSetTypes::DefaultObjectId )
        {
            cerr << "(main): идентификатор '" << name
                << "' не найден в конф. файле!"
                << " в секции " << conf->getObjectsSection() << endl;
            return 0;
        }

        auto act = UniSetActivator::Instance();
        auto smon = make_shared<SMonitor>(ID);
        act->add(smon);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( Exception& ex )
    {
        cout << "(main):" << ex << endl;
    }
    catch( std::exception& ex)
    {
        cout << "(main): exception: " << ex.what() << endl;
    }
    catch(...)
    {
        cout << "(main): Unknown exception!!"<< endl;
    }

    return 1;
}
// ------------------------------------------------------------------------------------------
