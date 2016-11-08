// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "MBTCPServer.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "verbose", no_argument, 0, 'v' },
	{ "myaddr", required_argument, 0, 'a' },
	{ "port", required_argument, 0, 'p' },
	{ "const-reply", required_argument, 0, 'c' },
	{ "after-send-pause", required_argument, 0, 's' },
	{ "max-sessions", required_argument, 0, 'm' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("Example: uniset-mbtcpserver-echo -i localhost -p 2049 -v \n");
	printf("-h|--help                      - this message\n");
	printf("[-v|--verbose]                 - Print all messages to stdout\n");
	printf("[-i|--iaddr] ip                - Server listen ip. Default 127.0.0.1\n");
	printf("[-a|--myaddr] addr1,addr2,...  - Modbus address for master. Default: 0x01.\n");
	printf("                    myaddr=255 - Reply to all RTU-addresses.\n");
	printf("[-p|--port] port               - Server port. Default: 502.\n");
	printf("[-c|--const-reply] val         - Reply 'val' for all queries\n");
	printf("[-s|--after-send-pause] msec   - Pause after send request. Default: 0\n");
	printf("[-m|--max-sessions] num        - Set the maximum number of sessions. Default: 10\n");
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
	string myaddr("0x01");
	auto dlog = make_shared<DebugStream>();
	int replyVal = -1;
	timeout_t afterpause = 0;
	size_t maxSessions = 10;

	try
	{
		while(1)
		{
			opt = getopt_long(argc, argv, "hva:p:i:c:s:m:", longopts, &optindex);

			if( opt == -1 )
				break;

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
					myaddr = string(optarg);
					break;

				case 'v':
					verb = 1;
					break;

				case 'c':
					replyVal = uni_atoi(optarg);
					break;

				case 's':
					afterpause = uni_atoi(optarg);
					break;

				case 'm':
					maxSessions = uni_atoi(optarg);
					break;

				case '?':
				default:
					printf("? argumnet\n");
					return 0;
			}
		}

		auto avec = uniset::explode_str(myaddr, ',');
		std::unordered_set<ModbusRTU::ModbusAddr> vaddr;

		for( const auto& a : avec )
			vaddr.emplace( ModbusRTU::str2mbAddr(a) );

		if( verb )
		{
			cout << "(init): iaddr: " << iaddr << ":" << port
				 << " myaddr=" << ModbusServer::vaddr2str(vaddr)
				 << endl;

			dlog->addLevel( Debug::ANY );
		}

		MBTCPServer mbs(vaddr, iaddr, port, verb);
		mbs.setLog(dlog);
		mbs.setVerbose(verb);
		mbs.setAfterSendPause(afterpause);
		mbs.setMaxSessions(maxSessions);

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
