#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include "Configuration.h"

int main( int argc, char* argv[] )
{
	try
	{
		Catch::Session session;

		UniSetTypes::uniset_init(argc, argv);

		int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

		if( returnCode != 0 ) // Indicates a command line error
			return returnCode;

		return session.run();
	}
	catch( UniSetTypes::Exception& ex )
	{
		std::cerr << ex << std::endl;
	}

	return 1;
}
