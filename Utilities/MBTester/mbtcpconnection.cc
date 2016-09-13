// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "modbus/ModbusTCPMaster.h"
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
	{ "persistent-connection", no_argument, 0, 'o' },
	{ "num-cycles", required_argument, 0, 'l' },
	{ "sleep-msec", required_argument, 0, 's' },
	{ "timeout", required_argument, 0, 't' },
	{ "read", required_argument, 0, 'r' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("Example: mbtcp-test-connection -i localhost -p 502 -r 12\n");
	printf("-h|--help         - this message\n");
	printf("[-i|--iaddr] ip                 - Modbus server ip. Default: 127.0.0.1\n");
	printf("[-a|--myaddr] addr              - Modbus address for master. Default: 0x01.\n");
	printf("[-p|--port] port                - Modbus server port. Default: 502.\n");
	printf("[-o|--persistent-connection]    - Use persistent-connection.\n");
	printf("[-l|--num-cycles] num           - Number of cycles of exchange. Default: -1 - infinitely.\n");
	printf("[-v|--verbose]                  - Print all messages to stdout\n");
	printf("[-s|--sleep-msec]               - send pause. Default: 200 msec\n");
	printf("[-t|--timeout]                  - timeout for connection. Default: 2000\n");
	printf("[-r|--read addr reg cnt]         - read regster, cnt - count (defaulr: 1)\n");
}
// --------------------------------------------------------------------------
static char* checkArg( int ind, int argc, char* argv[] );
int main( int argc, char** argv )
{
	//	std::ios::sync_with_stdio(false);
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	bool persist = false;
	string iaddr("127.0.0.1");
	int port = 502;
	int sleep_msec = 500;

	ModbusRTU::ModbusData reg = 0;
	ModbusRTU::ModbusData count = 1;
	ModbusRTU::ModbusAddr myaddr = 0x01;
	ModbusRTU::ModbusAddr slaveaddr = 0x00;
	int tout = 2000;
	auto dlog = make_shared<DebugStream>();
	int ncycles = -1;

	try
	{
		while( (opt = getopt_long(argc, argv, "hva:p:i:ol:s:r:t:", longopts, &optindex)) != -1 )
		{
			switch (opt)
			{
				case 'h':
					print_help();
					return 0;

				case 'r':
				{
					slaveaddr = ModbusRTU::str2mbAddr( optarg );

					if( !checkArg(optind, argc, argv) )
					{
						cerr << "read command error: bad or no arguments..." << endl;
						return 1;
					}
					else
						reg = ModbusRTU::str2mbData(argv[optind]);

					if( checkArg(optind + 1, argc, argv) )
						count = ModbusRTU::str2mbData(argv[optind + 1]);
				}
				break;

				case 'i':
					iaddr = string(optarg);
					break;

				case 'p':
					port = uni_atoi(optarg);
					break;

				case 't':
					tout = uni_atoi(optarg);
					break;

				case 's':
					sleep_msec = uni_atoi(optarg);
					break;

				case 'a':
					myaddr = ModbusRTU::str2mbAddr(optarg);
					break;

				case 'v':
					verb = 1;
					break;

				case 'o':
					persist = true;
					break;

				case 'l':
					ncycles = uni_atoi(optarg);
					break;

				case '?':
				default:
					printf("? argumnet\n");
					return 0;
			}
		}

		if( reg == 0 )
		{
			print_help();
			return 0;
		}

		if( verb )
		{
			cout << "(init): ip=" << iaddr << ":" << port
				 << " mbaddr=" << ModbusRTU::addr2str(myaddr)
				 << " timeout=" << tout << " msec "
				 << endl;

			dlog->level(Debug::ANY);
		}

		ModbusTCPMaster mb;
		mb.setLog(dlog);

		mb.setTimeout(tout);
		mb.connect(iaddr, port);
		mb.setForceDisconnect(!persist);

		if( verb )
			cout << "connection: " << (mb.isConnection() ? "YES" : "NO") << endl;

		if( count > ModbusRTU::MAXDATALEN && verb )
			cout << "Too long packet! Max count=" << ModbusRTU::MAXDATALEN << " (ignore...)" << endl;

		int nc = 1;

		if( ncycles > 0 )
			nc = ncycles;

		while( nc )
		{
			try
			{
				if( verb )
				{
					cout << "read03: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << " reg=" << ModbusRTU::dat2str(reg)
						 << " count=" << ModbusRTU::dat2str(count)
						 << endl;
				}

				ModbusRTU::ReadOutputRetMessage ret = mb.read03(slaveaddr, reg, count);

				if( verb )
					cout << "(reply): " << ret << endl;

				cout << "(reply): count=" << ModbusRTU::dat2str(ret.count) << endl;

				for( size_t i = 0; i < ret.count; i++ )
				{
					cout << i << ": (" << ModbusRTU::dat2str( reg + i ) << ") = " << (int)(ret.data[i])
						 << " ("
						 << ModbusRTU::dat2str(ret.data[i])
						 << ")"
						 << endl;
				}
			}
			catch( ModbusRTU::mbException& ex )
			{
				if( ex.err != ModbusRTU::erTimeOut )
					throw;

				cout << "timeout..." << endl;
			}

			if( ncycles > 0 )
			{
				nc--;

				if( nc <= 0 )
					break;
			}

			msleep(sleep_msec);

		} // end of while

		mb.disconnect();
	}
	catch( const ModbusRTU::mbException& ex )
	{
		cerr << "(mbtester): " << ex << endl;
	}
	catch( const SystemError& err )
	{
		cerr << "(mbtester): " << err << endl;
	}
	catch( const Exception& ex )
	{
		cerr << "(mbtester): " << ex << endl;
	}
	catch( const std::exception& e )
	{
		cerr << "(mbtester): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(mbtester): catch(...)" << endl;
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
