#include "Configuration.h"
#include "DBServer_PostgreSQL.h"
#include "UniSetActivator.h"
#include "Debug.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
	try
	{
		if( argc > 1 && !strcmp(argv[1], "--help") )
		{
			DBServer_PostgreSQL::help_print(argc, argv);
			return 0;
		}

		auto conf = uniset_init(argc, argv, "configure.xml");

		auto dbs = DBServer_PostgreSQL::init_dbserver(argc, argv);
		auto act = UniSetActivator::Instance();
		act->add(dbs);
		act->run(false);
	}
	catch( const Exception& ex )
	{
		cerr << "(DBServer_PosgreSQL::main): " << ex << endl;
	}
	catch( std::exception& ex )
	{
		cerr << "(DBServer_PosgreSQL::main): " << ex.what() << endl;
	}
	catch(...)
	{
		cerr << "(DBServer_PosgreSQL::main): catch ..." << endl;
	}

	return 0;
}
