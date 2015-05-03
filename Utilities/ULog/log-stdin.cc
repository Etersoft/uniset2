// --------------------------------------------------------------------------
#include <getopt.h>
#include <iostream>
#include <string>
#include "DebugStream.h"
#include "LogServer.h"
#include "Exceptions.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "port", required_argument, 0, 'p' },
	{ "verbose", no_argument, 0, 'v' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h       - this message\n");
	printf("-v       - Print all messages to stdout\n");
	printf("-i addr  - LogServer ip or hostname. Default: localhost.\n");
	printf("-p port  - LogServer port. Default: 3333.\n");
}
// --------------------------------------------------------------------------
int main( int argc, char* argv[], char* envp[] )
{
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	string addr("localhost");
	int port = 3333;

	try
	{
		while( (opt = getopt_long(argc, argv, "hvi:p:", longopts, &optindex)) != -1 )
		{
			switch (opt)
			{
				case 'h':
					print_help();
					return 0;

				case 'i':
					addr = string(optarg);
					break;

				case 'p':
					port = atoi(optarg);
					break;

				case 'v':
					verb = 1;
					break;

				case '?':
				default:
					printf("? argumnet\n");
					return 0;
			}
		}

		if( verb )
			cout << "(init): listen " << addr << ":" << port << endl;

		auto log = make_shared<DebugStream>();
		LogServer ls(log);

		ls.run(addr, port, true);

		char buf[10000];

		while( true )
		{
			size_t r = read(fileno(stdin), buf, sizeof(buf) - 1);

			if( r > 0 )
			{
				buf[r] = '\0';
				(*(log.get())) << buf;
			}
		}
	}
	catch( const SystemError& err )
	{
		cerr << "(log-stdin): " << err << endl;
		return 1;
	}
	catch( const Exception& ex )
	{
		cerr << "(log-stdin): " << ex << endl;
		return 1;
	}
	catch(...)
	{
		cerr << "(log-stdin): catch(...)" << endl;
		return 1;
	}

	return 0;
}
// --------------------------------------------------------------------------
