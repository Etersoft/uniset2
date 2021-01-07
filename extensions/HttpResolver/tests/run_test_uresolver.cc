#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "UHelpers.h"
#include "TestObject.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            session.showHelp("tests_httpresolver");
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

        if( returnCode != 0 ) // Indicates a command line error
            return returnCode;

        auto conf = uniset_init(argc, argv);

        auto to = make_object<TestObject>("TestProc", "TestProc");

        if( !to )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(to);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);
        return session.run();
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(tests_httpresolver): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_httpresolver): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_httpresolver): catch(...)" << endl;
    }

    return 1;
}
