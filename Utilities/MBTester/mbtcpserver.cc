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
static struct option longopts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "verbose", no_argument, 0, 'v' },
	{ "myaddr", required_argument, 0, 'a' },
	{ "port", required_argument, 0, 'p' },
	{ "ignore-addr", required_argument, 0, 'x' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h|--help 		- this message\n");
	printf("[-t|--timeout] msec               - Timeout. Default: 2000.\n");
	printf("[-v|--verbose]                    - Print all messages to stdout\n");
	printf("[-i|--iaddr] ip                   - Server listen ip. Default 127.0.0.1\n");
	printf("[-a|--myaddr] addr                - Modbus address for master. Default: 0x01.\n");
	printf("[-x|--ignore-addr]           	  - Ignore modbus RTU-address.\n");
	printf("[-p|--port] port                  - Server port. Default: 502.\n");
	printf("[-v|--verbose]                    - Print all messages to stdout\n");
}
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	int port = 502;
	string iaddr("127.0.0.1");
	ModbusRTU::ModbusAddr myaddr = 0x01;
	int tout = 2000;
	DebugStream dlog;
	bool ignoreAddr = false;
	
	ost::Thread::setException(ost::Thread::throwException);

	try
	{
		while( (opt = getopt_long(argc, argv, "ht:va:p:i:bx",longopts,&optindex)) != -1 ) 
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

				case 't':	
					tout = uni_atoi(optarg);
				break;

				case 'a':	
					myaddr = ModbusRTU::str2mbAddr(optarg);
				break;

				case 'v':	
					verb = 1;
				break;

				case 'x':	
					ignoreAddr = true;
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
					<< " timeout=" << tout << " msec "
					<< endl;					
	
			dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
		}		
		
		MBTCPServer mbs(myaddr,iaddr,port,verb);
		mbs.setLog(dlog);
		mbs.setVerbose(verb);
		mbs.setIgnoreAddrMode(ignoreAddr);
		mbs.execute();
	}
	catch( ModbusRTU::mbException& ex )
	{
		cerr << "(mbtcpserver): " << ex << endl;
	}
	catch(SystemError& err)
	{
		cerr << "(mbtcpserver): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(mbtcpserver): " << ex << endl;
	}
	catch( ost::SockException& e ) 
	{
		cerr << e.getString() << ": " << e.getSystemErrorString() << endl;
	}
	catch(...)
	{
		cerr << "(mbtcpserver): catch(...)" << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------
