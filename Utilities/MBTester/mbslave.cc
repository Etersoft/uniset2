// --------------------------------------------------------------------------
//!  \version $Id: mbslave.cc,v 1.5 2009/02/24 20:27:24 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "MBSlave.h"
#include "ComPort485F.h"
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
	{ "f485", no_argument, 0, 'g' },
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
	printf("[-v|--verbose]                    - Print all messages to stdout\n");
	printf("[-g|--f485]                       - Use 485 Fastwel\n");
}
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	int f485 = 0;
	string dev("/dev/ttyS0");
	string speed("38400");
	ModbusRTU::ModbusAddr myaddr = 0x01;
	int tout = 2000;
	DebugStream dlog;


	try
	{
		while( (opt = getopt_long(argc, argv, "hva:d:s:c:",longopts,&optindex)) != -1 ) 
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

				case 'g':	
					f485 = 1;
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
		
		if( f485 )
		{
			ComPort485F* cp;
			if( dev == "/dev/ttyS2" )
				cp = new ComPort485F(dev,5);
			else if( dev == "/dev/ttyS3" )
				cp = new ComPort485F(dev,6);
			else
			{
				cerr << "dev must be /dev/ttyS2 or /dev/tytS3" << endl;
				return 1;
			}
		
			MBSlave mbs(cp,myaddr,speed);
			mbs.setLog(dlog);
			mbs.setVerbose(verb);
			mbs.execute();
		}
		else
		{
			MBSlave mbs(myaddr,dev,speed);
			mbs.setLog(dlog);
			mbs.setVerbose(verb);
			mbs.execute();
		}
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
