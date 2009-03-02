// --------------------------------------------------------------------------
//!  \version $Id: mbrtuslave.cc,v 1.2 2009/02/24 20:27:24 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "MBSlave.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "device", required_argument, 0, 'd' },
	{ "verbose", no_argument, 0, 'v' },
	{ "myaddr", required_argument, 0, 'a' },
	{ "speed", required_argument, 0, 's' },
	{ "use485F", no_argument, 0, 'y' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h|--help 		- this message\n");
	printf("[-t|--timeout] msec               - Timeout. Default: 2000.\n");
	printf("[-v|--verbose]                    - Print all messages to stdout\n");
	printf("[-d|--device] dev                 - use device dev. Default: /dev/ttyS0\n");
	printf("[-a|--myaddr] addr                - Modbus address for master. Default: 0x01.\n");
	printf("[-s|--speed] speed                - 9600,14400,19200,38400,57600,115200. Default: 38400.\n");
	printf("[-y|--use485F]                    - use RS485 Fastwel.\n");
	printf("[-v|--verbose]                    - Print all messages to stdout\n");
}
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	string dev("/dev/ttyS0");
	string speed("38400");
	ModbusRTU::ModbusAddr myaddr = 0x01;
	int tout = 2000;
	DebugStream dlog;
	int use485 = 0;

	try
	{
		while( (opt = getopt_long(argc, argv, "hva:d:s:y",longopts,&optindex)) != -1 ) 
		{
			switch (opt) 
			{
				case 'h':
					print_help();
				return 0;

				case 'd':
					dev = string(optarg);
				break;

				case 's':
					speed = string(optarg);
				break;

				case 't':
					tout = atoi(optarg);
				break;

				case 'a':
					myaddr = ModbusRTU::str2mbAddr(optarg);
				break;

				case 'v':
					verb = 1;
				break;

				case 'y':
					use485 = 1;
				break;

				case '?':
				default:
					printf("? argumnet\n");
					return 0;
			}
		}

		if( verb )
		{
			cout << "(init): dev=" << dev << " speed=" << speed
					<< " myaddr=" << ModbusRTU::addr2str(myaddr)
					<< " timeout=" << tout << " msec "
					<< endl;					
	
			dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
		}		
		
		MBSlave mbs(myaddr,dev,speed,use485);
		mbs.setLog(dlog);
		mbs.setVerbose(verb);
		mbs.execute();
	}
	catch( ModbusRTU::mbException& ex )
	{
		cerr << "(mbtester): " << ex << endl;
	}
	catch(SystemError& err)
	{
		cerr << "(mbslave): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(mbslave): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(mbslave): catch(...)" << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------
