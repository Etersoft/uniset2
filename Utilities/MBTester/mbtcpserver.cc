// --------------------------------------------------------------------------
#include <string>
#include <cc++/socket.h>
#include <getopt.h>
#include "Debug.h"
#include "MBTCPServer.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "verbose", no_argument, 0, 'v' },
	{ "myaddr", required_argument, 0, 'a' },
	{ "port", required_argument, 0, 'p' },
	{ "reply-all", no_argument, 0, 'r' },
	{ "const-reply", required_argument, 0, 'c' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("Example: uniset-mbtcpserver-echo -i localhost -p 2049 -v \n");
	printf("-h|--help                 - this message\n");
	printf("[-v|--verbose]          - Print all messages to stdout\n");
	printf("[-i|--iaddr] ip         - Server listen ip. Default 127.0.0.1\n");
	printf("[-a|--myaddr] addr      - Modbus address for master. Default: 0x01.\n");
	printf("[-r|--reply-all]        - Reply to all RTU-addresses.\n");
	printf("[-p|--port] port        - Server port. Default: 502.\n");
	printf("[-c|--const-reply] val  - Reply 'val' for all queries\n");
}
// --------------------------------------------------------------------------
int main( int argc, char** argv )
{
	//	std::ios::sync_with_stdio(false);
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	int port = 502;
	string iaddr("127.0.0.1");
	ModbusRTU::ModbusAddr myaddr = 0x01;
	auto dlog = make_shared<DebugStream>();
	bool ignoreAddr = false;
	int replyVal = -1;

	ost::Thread::setException(ost::Thread::throwException);

	try
	{
		while( (opt = getopt_long(argc, argv, "hva:p:i:brc:", longopts, &optindex)) != -1 )
		{
			switch (opt)
			{
				case 'h':
					print_help();
					return 0;

				case 'i':
					iaddr = string(optarg);
					break;

				case 'p':
					port = uni_atoi(optarg);
					break;

				case 'a':
					myaddr = ModbusRTU::str2mbAddr(optarg);
					break;

				case 'v':
					verb = 1;
					break;

				case 'r':
					ignoreAddr = true;
					break;

				case 'c':
					replyVal = uni_atoi(optarg);
					break;

				case '?':
				default:
					printf("? argumnet\n");
					return 0;
			}
		}

		if( verb )
		{
			cout << "(init): iaddr: " << iaddr << ":" << port
				 << " myaddr=" << ModbusRTU::addr2str(myaddr)
				 << endl;

			dlog->addLevel( Debug::ANY );
		}

		MBTCPServer mbs(myaddr, iaddr, port, verb);
		mbs.setLog(dlog);
		mbs.setVerbose(verb);
		mbs.setIgnoreAddrMode(ignoreAddr);

		if( replyVal != -1 )
			mbs.setReply(replyVal);

		mbs.execute();
	}
	catch( const ModbusRTU::mbException& ex )
	{
		cerr << "(mbtcpserver): " << ex << endl;
	}
	catch( const std::exception& e )
	{
		cerr << "(mbtcpserver): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(mbtcpserver): catch(...)" << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------
