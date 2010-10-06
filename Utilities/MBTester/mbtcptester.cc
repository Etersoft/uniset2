// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "modbus/ModbusTCPMaster.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "read01", required_argument, 0, 'c' },
	{ "read02", required_argument, 0, 'b' },
	{ "read03", required_argument, 0, 'r' },
	{ "read04", required_argument, 0, 'x' },
	{ "write05", required_argument, 0, 'f' }, 
	{ "write06", required_argument, 0, 'z' }, 
	{ "write0F", required_argument, 0, 'm' },
	{ "write10", required_argument, 0, 'w' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "verbose", no_argument, 0, 'v' },
	{ "myaddr", required_argument, 0, 'a' },
	{ "port", required_argument, 0, 'p' },
	{ "persistent-connection", no_argument, 0, 'o' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h|--help 		- this message\n");
	printf("[--write05] slaveaddr reg val  - write val to reg for slaveaddr\n");
	printf("[--write06] slaveaddr reg val  - write val to reg for slaveaddr\n");
	printf("[--write0F] slaveaddr reg val  - write val to reg for slaveaddr\n");
	printf("[--write10] slaveaddr reg val count  - write val to reg for slaveaddr\n");
	printf("[--read01] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--read02] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--read03] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--read04] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[-i|--iaddr] ip                 - Modbus server ip. Default: 127.0.0.1\n");
	printf("[-a|--myaddr] addr                - Modbus address for master. Default: 0x01.\n");
	printf("[-p|--port] port                  - Modbus server port. Default: 502.\n");
	printf("[-t|--timeout] msec               - Timeout. Default: 2000.\n");
	printf("[-o|--persistent-connection]      - Use persistent-connection.\n");
	printf("[-v|--verbose]                    - Print all messages to stdout\n");
}
// --------------------------------------------------------------------------
enum Command
{
	cmdNOP,
	cmdRead01,
	cmdRead02,
	cmdRead03,
	cmdRead04,
	cmdWrite05,
	cmdWrite06,
	cmdWrite0F,
	cmdWrite10
};
// --------------------------------------------------------------------------
static char* checkArg( int ind, int argc, char* argv[] );
// --------------------------------------------------------------------------

int main( int argc, char **argv )
{   
	Command cmd = cmdNOP;
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	bool persist = false;
	string iaddr("127.0.0.1");
	int port=502;
	ModbusRTU::ModbusData reg = 0;
	ModbusRTU::ModbusData val = 0;
	ModbusRTU::ModbusData count = 1;
	ModbusRTU::ModbusAddr myaddr = 0x01;
	ModbusRTU::ModbusAddr slaveaddr = 0x00;
	int tout = 2000;
	DebugStream dlog;

	try
	{
		while( (opt = getopt_long(argc, argv, "hva:w:z:r:x:c:b:d:s:t:p:i:o",longopts,&optindex)) != -1 ) 
		{
			switch (opt) 
			{
				case 'h':
					print_help();	
				return 0;

				case 'c':
					if( cmd == cmdNOP )
						cmd = cmdRead01;
				case 'b':
					if( cmd == cmdNOP )
						cmd = cmdRead02;
				case 'r':
					if( cmd == cmdNOP )
						cmd = cmdRead03;
				case 'x':

					if( cmd == cmdNOP )
						cmd = cmdRead04;
					slaveaddr = ModbusRTU::str2mbAddr( optarg );
					if( optind > argc )
					{
						cerr << "read command error: bad or no arguments..." << endl;
						return 1;
					}

					if( checkArg(optind,argc,argv) )
						reg = ModbusRTU::str2mbData(argv[optind]);
					
					if( checkArg(optind+1,argc,argv) )
						count = uni_atoi(argv[optind+1]);
				break;

				case 'f':
					cmd = cmdWrite05;
				case 'z':
					if( cmd == cmdNOP )
						cmd = cmdWrite06;
				case 'm':
					if( cmd == cmdNOP )
						cmd = cmdWrite0F;
				case 'w':
					if( cmd == cmdNOP )
						cmd = cmdWrite10;
					slaveaddr = ModbusRTU::str2mbAddr(optarg);
					
					if( !checkArg(optind,argc,argv) )
					{
						cerr << "write command error: bad or no arguments..." << endl;
						return 1;
					}
					reg = ModbusRTU::str2mbData(argv[optind]);

					if( !checkArg(optind+1,argc,argv) )
					{
						if( (argv[optind+1])[0] == 'b' )
						{
							string v(argv[optind+1]);
							string sb(v,1);
							ModbusRTU::DataBits d(sb);
							val = d.mbyte();
						}
						else
							val = ModbusRTU::str2mbData(argv[optind+1]);
					}
	
					if( cmd == cmdWrite10 && checkArg(optind+2,argc,argv) )
                        count = ModbusRTU::str2mbData(argv[optind+2]);
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

				case 'a':
					myaddr = ModbusRTU::str2mbAddr(optarg);
				break;

				case 'v':	
					verb = 1;
				break;

				case 'o':	
					persist = true;
				break;

				case '?':
				default:
					printf("? argumnet\n");
					return 0;
			}
		}

		if( verb )
		{
			cout << "(init): ip=" << iaddr << ":" << port
					<< ModbusRTU::addr2str(myaddr)
					<< " timeout=" << tout << " msec "
					<< endl;

			dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
		}
		
		ModbusTCPMaster mb;
		mb.setLog(dlog);

//		ost::Thread::setException(ost::Thread::throwException);
		ost::InetAddress ia(iaddr.c_str());
		mb.setTimeout(tout);
		mb.connect(ia,port);
		
		mb.setForceDisconnect(!persist);

		if( verb )
			cout << "connection: " << (mb.isConnection() ? "YES" : "NO") << endl;

		while(1)
		{
			try
			{
			switch(cmd)
			{
				case cmdRead01:
				{
					if( verb )
					{
						cout << "read01: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg) 
							 << " count=" << ModbusRTU::dat2str(count) 
							 << endl;
					}

					ModbusRTU::ReadCoilRetMessage ret = mb.read01(slaveaddr,reg,count);
					if( verb )
						cout << "(reply): " << ret << endl;
					cout << "(reply): count=" << (int)ret.bcnt << endl;
					for( int i=0; i<ret.bcnt; i++ )
					{
						ModbusRTU::DataBits b(ret.data[i]);
					
						cout << i <<": (" << ModbusRTU::dat2str( reg + i ) << ") = (" 
							<< ModbusRTU::b2str(ret.data[i]) << ") " << b << endl;
					}
				}
				break;

				case cmdRead02:
				{
					if( verb )
					{
						cout << "read02: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << " reg=" << ModbusRTU::dat2str(reg) 
						 << " count=" << ModbusRTU::dat2str(count) 
						 << endl;
					}

					ModbusRTU::ReadInputStatusRetMessage ret = mb.read02(slaveaddr,reg,count);
					if( verb )
						cout << "(reply): " << ret << endl;
					cout << "(reply): count=" << (int)ret.bcnt << endl;
					for( int i=0; i<ret.bcnt; i++ )
					{
						ModbusRTU::DataBits b(ret.data[i]);
					
						cout << i <<": (" << ModbusRTU::dat2str( reg + i ) << ") = (" 
							<< ModbusRTU::b2str(ret.data[i]) << ") " << b << endl;
					}
				}
				break;

				case cmdRead03:
				{
					if( verb )
					{
						cout << "read03: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg)
							 << " count=" << ModbusRTU::dat2str(count) 
							 << endl;
					}

					ModbusRTU::ReadOutputRetMessage ret = mb.read03(slaveaddr,reg,count);
					if( verb )
						cout << "(reply): " << ret << endl;
					cout << "(reply): count=" << ModbusRTU::dat2str(ret.count) << endl;
					for( int i=0; i<ret.count; i++ )
					{
						cout << i <<": (" << ModbusRTU::dat2str( reg + i ) << ") = " << (int)(ret.data[i]) 
							<< " (" 
							<< ModbusRTU::dat2str(ret.data[i]) 
							<< ")" 
							<< endl;
					}
				}
				break;
			
				case cmdRead04:
				{
					if( verb )
					{
						cout << "read04: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg) 
							 << " count=" << ModbusRTU::dat2str(count) 
							 << endl;
					}

					ModbusRTU::ReadInputRetMessage ret = mb.read04(slaveaddr,reg,count);
					if( verb )
						cout << "(reply): " << ret << endl;
					cout << "(reply): count=" << ModbusRTU::dat2str(ret.count) << endl;
					for( int i=0; i<ret.count; i++ )
					{
						cout << i <<": (" << ModbusRTU::dat2str( reg + i ) << ") = " << (int)(ret.data[i]) 
							<< " (" 
							<< ModbusRTU::dat2str(ret.data[i]) 
							<< ")" 
							<< endl;
					}
				}
				break;

				case cmdWrite05:
				{
					if( verb )
					{
						cout << "write05: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg) 
							 << " val=" << ModbusRTU::dat2str(val) 
							 << endl;
					}
					
					ModbusRTU::ForceSingleCoilRetMessage  ret = mb.write05(slaveaddr,reg,(bool)val);
					if( verb )
						cout << "(reply): " << ret << endl;
				}
				break;

				case cmdWrite06:
				{
					if( verb )
					{
						cout << "write06: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg) 
							 << " val=" << ModbusRTU::dat2str(val) 
							 << endl;
					}
					
					ModbusRTU::WriteSingleOutputRetMessage  ret = mb.write06(slaveaddr,reg,val);
					if( verb )
						cout << "(reply): " << ret << endl;
					
				}
				break;

				case cmdWrite0F:
				{
					if( verb )
					{
						cout << "write0F: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg) 
							 << " val=" << ModbusRTU::dat2str(val) 
							 << endl;
					}
					
					ModbusRTU::ForceCoilsMessage msg(slaveaddr,reg);
					ModbusRTU::DataBits b(val);
					msg.addData(b);
					ModbusRTU::ForceCoilsRetMessage  ret = mb.write0F(msg);
					if( verb )
						cout << "(reply): " << ret << endl;
				}
				break;

				case cmdWrite10:
				{
					if( verb )
					{
						cout << "write10: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
							 << " reg=" << ModbusRTU::dat2str(reg) 
							 << " val=" << ModbusRTU::dat2str(val) 
							 << " count=" << count
							 << endl;
					}
					
					ModbusRTU::WriteOutputMessage msg(slaveaddr,reg);
					for( int i=0; i<count; i++ )
						msg.addData(val);
					ModbusRTU::WriteOutputRetMessage  ret = mb.write10(msg);
					if( verb )
						cout << "(reply): " << ret << endl;
				}
				break;

				case cmdNOP:
				default:
					cerr << "No command. Use -h for help." << endl;
					return 1;
			}
            }
            catch( ModbusRTU::mbException& ex )
            {
            	if( ex.err != ModbusRTU::erTimeOut )
            		throw ex;
            	
            	cout << "timeout..." << endl;
            }

			msleep(200);
		} // end of while

		mb.disconnect();
	}
	catch( ModbusRTU::mbException& ex )
	{
		cerr << "(mbtester): " << ex << endl;
	}
	catch(SystemError& err)
	{
		cerr << "(mbtester): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(mbtester): " << ex << endl;
	}
	catch( ost::SockException& e ) 
	{
		cerr << e.getString() << ": " << e.getSystemErrorString() << endl;
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
	if( i<argc && (argv[i])[0]!='-' )
		return argv[i];
		
	return 0;
}
// --------------------------------------------------------------------------
