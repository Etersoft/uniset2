// --------------------------------------------------------------------------
#include <string>
#include <vector>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogReader.h"
#include "LogServerTypes.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "verbose", no_argument, 0, 'v' },
	{ "filter", required_argument, 0, 'f' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "port", required_argument, 0, 'p' },
	{ "add", required_argument, 0, 'a' },
	{ "del", required_argument, 0, 'd' },
	{ "set", required_argument, 0, 's' },
	{ "off", required_argument, 0, 'o' },
	{ "on", required_argument, 0, 'e' },
	{ "list", optional_argument, 0, 'l' },
	{ "rotate", optional_argument, 0, 'r' },
	{ "logfilter", required_argument, 0, 'n' },
	{ "command-only", no_argument, 0, 'c' },
	{ "timeout", required_argument, 0, 'w' },
	{ "reconnect-delay", required_argument, 0, 'x' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h, --help                  - this message\n");
	printf("-v, --verbose               - Print all messages to stdout\n");
	printf("[-i|--iaddr] addr           - LogServer ip or hostname.\n");
	printf("[-p|--port] port            - LogServer port.\n");
	printf("[-c|--command-only]         - Send command and break. (No read logs).\n");
	printf("[-w|--timeout] msec         - Timeout for wait data. Default: 0 - endless waiting\n");
	printf("[-x|--reconnect-delay] msec - Pause for repeat connect to LogServer. Default: 5000 msec.\n");

	printf("\n");
	printf("Commands:\n");

	printf("[--add | -a] info,warn,crit,... [logfilter] - Add log levels.\n");
	printf("[--del | -d] info,warn,crit,... [logfilter] - Delete log levels.\n");
	printf("[--set | -s] info,warn,crit,... [logfilter] - Set log levels.\n");
	printf("--off, -o [logfilter]                       - Off the write log file (if enabled).\n");
	printf("--on, -e  [logfilter]                       - On(enable) the write log file (if before disabled).\n");
	printf("--rotate, -r [logfilter]                    - rotate log file.\n");
	printf("--list, -l   [logfilter]                    - List of managed logs.\n");
	printf("--filter, -f logfilter                      - ('filter mode'). View log only from 'logfilter'(regexp)\n");
	printf("\n");
	printf("Note: 'logfilter' -  regexp for name of log. Default: ALL logs.\n");
}
// --------------------------------------------------------------------------
static char* checkArg( int i, int argc, char* argv[] );
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
int main( int argc, char** argv )
{
	//	std::ios::sync_with_stdio(false); // нельзя отключать.. тогда "обмен с сервером" рассинхронизируется

	int optindex = 0;
	int opt = 0;
	int verb = 0;
	string addr("localhost");
	int port = 3333;
	DebugStream dlog;
	vector<LogReader::Command> vcmd;
	string logfilter("");
	LogServerTypes::Command cmd = LogServerTypes::cmdNOP;
	int cmdonly = 0;
	timeout_t tout = 0;
	timeout_t rdelay = 5000;

	try
	{
		while(1)
		{
			opt = getopt_long(argc, argv, "chvlf:a:p:i:d:s:n:eorbx:w:", longopts, &optindex);

			if( opt == -1 )
				break;

			switch (opt)
			{
				case 'h':
					print_help();
					return 0;

				case 'a':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdAddLevel;
					std::string filter("");
					std::string d = string(optarg);
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(cmd, (int)Debug::value(d), filter) );
				}
				break;

				case 'd':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdDelLevel;
					std::string filter("");
					std::string d = string(optarg);
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(cmd, (int)Debug::value(d), filter) );
				}
				break;

				case 's':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdSetLevel;
					std::string filter("");
					std::string d = string(optarg);
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(cmd, (int)Debug::value(d), filter) );
				}
				break;

				case 'l':
				{
					cmdonly = 1;
					std::string filter("");
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(LogServerTypes::cmdList, 0, filter) );
				}
				break;

				case 'o':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdOffLogFile;
					std::string filter("");
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(cmd, 0, filter) );
				}
				break;

				case 'e':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdOnLogFile;
					std::string filter("");
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(cmd, 0, filter) );
				}
				break;

				case 'f':
				{
					cmd = LogServerTypes::cmdFilterMode;
					std::string filter("");
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					logfilter = filter;
				}
				break;

				case 'r':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdRotate;
					std::string filter("");
					char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.push_back( LogReader::Command(cmd, 0, filter) );
				}
				break;

				case 'i':
					addr = string(optarg);
					break;

				case 'c':
					cmdonly = 1;
					break;

				case 'p':
					port = uni_atoi(optarg);
					break;

				case 'x':
					rdelay = uni_atoi(optarg);
					break;

				case 'w':
					tout = uni_atoi(optarg);
					break;

				case 'v':
					verb = 1;
					break;

				case '?':
				default:
					printf("Unknown argumnet\n");
					return 0;
			}
		}

		if( verb )
		{
			cout << "(init): read from " << addr << ":" << port << endl;

			dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
		}

		LogReader lr;
		lr.setCommandOnlyMode(cmdonly);
		lr.setinTimeout(tout);
		lr.setReconnectDelay(rdelay);

		if( !vcmd.empty() )
			lr.sendCommand(addr, port, vcmd, cmdonly, verb);

		if( !cmdonly )
			lr.readlogs( addr, port, cmd, logfilter, verb );
	}
	catch( const SystemError& err )
	{
		cerr << "(log): " << err << endl;
	}
	catch( const Exception& ex )
	{
		cerr << "(log): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(log): catch(...)" << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------
char* checkArg( int i, int argc, char* argv[] )
{
	if( i < argc && (argv[i])[0] != '-' )
		return argv[i];

	return 0;
}
// --------------------------------------------------------------------------
