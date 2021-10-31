#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include "Configuration.h"
#include "Exceptions.h"

int main( int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        uniset::uniset_init(argc, argv);

        int returnCode = session.applyCommandLine( argc, argv );

        if( returnCode != 0 ) // Indicates a command line error
            return returnCode;

        return session.run();
    }
    catch( const uniset::Exception& ex )
    {
        std::cerr << ex << std::endl;
    }
    catch( const std::exception& ex )
    {
        std::cerr << ex.what() << std::endl;
    }

    return 1;
}
