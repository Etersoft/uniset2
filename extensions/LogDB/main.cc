#include "Configuration.h"
#include "LogDB.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	std::ios::sync_with_stdio(false);

	try
	{
		if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
		{
			cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
			LogDB::help_print();
			return 0;
		}

		uniset_init(argc, argv, "configure.xml");

		auto db = LogDB::init_logdb(argc, argv);
		db->run(false);
	}
	catch( const std::exception& ex )
	{
		cerr << "(LogDB::main): " << ex.what() << endl;
	}
	catch(...)
	{
		cerr << "(LogDB::main): catch ..." << endl;
	}

	return 0;
}
