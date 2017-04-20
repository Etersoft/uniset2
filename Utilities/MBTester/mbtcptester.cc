// --------------------------------------------------------------------------
#include <string>
#include <chrono>
#include <iomanip>
#include <getopt.h>
#include "Debug.h"
#include "modbus/ModbusTCPMaster.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "read01", required_argument, 0, 'c' },
	{ "read02", required_argument, 0, 'b' },
	{ "read03", required_argument, 0, 'r' },
	{ "read04", required_argument, 0, 'x' },
	//    { "read4313", required_argument, 0, 'u' },
	{ "read4314", required_argument, 0, 'e' },
	{ "write05", required_argument, 0, 'f' },
	{ "write06", required_argument, 0, 'z' },
	{ "write0F", required_argument, 0, 'm' },
	{ "write10", required_argument, 0, 'w' },
	{ "diag08", required_argument, 0, 'd' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "verbose", no_argument, 0, 'v' },
	{ "myaddr", required_argument, 0, 'a' },
	{ "port", required_argument, 0, 'p' },
	{ "persistent-connection", no_argument, 0, 'o' },
	{ "num-cycles", required_argument, 0, 'l' },
	{ "sleep-msec", required_argument, 0, 's' },
	{ "check", no_argument, 0, 'n' },
	{ "ignore-errors", no_argument, 0, 'g' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h|--help         - this message\n");
	printf("[--write05] slaveaddr reg val  - write val to reg for slaveaddr\n");
	printf("[--write06] slaveaddr reg val  - write val to reg for slaveaddr\n");
	printf("[--write0F] slaveaddr reg val  - write val to reg for slaveaddr\n");
	printf("[--write10] slaveaddr reg val1...valN  - write values to reg for slaveaddr\n");
	printf("             val: INTEGER   - INT value > 0 (2 byte)(example: 123)\n");
	printf("             val: mINTEGER  - INT value < 0 (2 byte)(example: m2344)\n");
	printf("             val: bVAL      - 8bit value (example: b0000011)\n");
	printf("             val: fVAL      - float value (4 byte)(example: f123.05)\n");
	printf("             val: rVAL      - (revert) float value /backorder(word1,word2)/(4 byte)(example: r123.05)\n");
	printf("[--read01] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--read02] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--read03] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--read04] slaveaddr reg count   - read from reg (from slaveaddr). Default: count=1\n");
	printf("[--diag08] slaveaddr subfunc [dat]  - diagnostics request\n");
	printf("[--read4314] slaveaddr devID objID - (0x2B/0x0E): read device identification (devID=[1...4], objID=[0..254])\n");
	//    printf("[--read43-13] slaveaddr ...     - (0x2B/0x0D):  CANopen General Reference Request and Response PDU \n");
	printf("[-i|--iaddr] ip                 - Modbus server ip. Default: 127.0.0.1\n");
	printf("[-a|--myaddr] addr              - Modbus address for master. Default: 0x01.\n");
	printf("[-p|--port] port                - Modbus server port. Default: 502.\n");
	printf("[-t|--timeout] msec             - Timeout. Default: 2000.\n");
	printf("[-o|--persistent-connection]    - Use persistent-connection.\n");
	printf("[-l|--num-cycles] num           - Number of cycles of exchange. Default: -1 - infinitely.\n");
	printf("[-v|--verbose]                  - Print all messages to stdout\n");
	printf("[-s|--sleep-msec]               - send pause. Default: 200 msec\n");
	printf("[-n|--check]                    - Check connection for (-i)ip (-p)port\n");
	printf("[-g|--ignore-errors]            - Ignore errors\n");
}
// --------------------------------------------------------------------------
enum Command
{
	cmdNOP,
	cmdRead01,
	cmdRead02,
	cmdRead03,
	cmdRead04,
	cmdRead43_13,
	cmdRead43_14,
	cmdWrite05,
	cmdWrite06,
	cmdWrite0F,
	cmdWrite10,
	cmdDiag,
	cmdCheck
};
// --------------------------------------------------------------------------
static char* checkArg( int ind, int argc, char* argv[] );
int main( int argc, char** argv )
{
	//	std::ios::sync_with_stdio(false);
	Command cmd = cmdNOP;
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	bool persist = false;
	string iaddr("127.0.0.1");
	int port = 502;
	ModbusRTU::ModbusData reg = 0;
	int val = 0;
	int sleep_msec = 500;
	bool ignoreErrors = false;

	union DValue
	{
		int v;
		float f;
	};

	struct DataInfo
	{
		DValue d = {0};
		char type = {'i'}; // i - integer, f - float, r - revert float
	};

	vector<DataInfo> data;
	ModbusRTU::ModbusData count = 1;
	ModbusRTU::ModbusAddr myaddr = 0x01;
	ModbusRTU::ModbusAddr slaveaddr = 0x00;
	ModbusRTU::DiagnosticsSubFunction subfunc = ModbusRTU::subEcho;
	ModbusRTU::ModbusData dat = 0;
	int tout = 2000;
	auto dlog = make_shared<DebugStream>();
	int ncycles = -1;
	ModbusRTU::ModbusByte devID = 0;
	ModbusRTU::ModbusByte objID = 0;

	try
	{
		while( 1 )
		{
			opt = getopt_long(argc, argv, "hvgna:w:z:r:x:c:b:d:s:t:p:i:ol:d:e:u:", longopts, &optindex);

			if( opt == -1 )
				break;

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
				{
					if( cmd == cmdNOP )
						cmd = cmdRead04;

					slaveaddr = ModbusRTU::str2mbAddr( optarg );

					if( optind > argc )
					{
						cerr << "read command error: bad or no arguments..." << endl;
						return 1;
					}

					if( checkArg(optind, argc, argv) )
						reg = ModbusRTU::str2mbData(argv[optind]);

					if( checkArg(optind + 1, argc, argv) )
						count = uni_atoi(argv[optind + 1]);
				}
				break;

				case 'n':
					cmd = cmdCheck;
					break;

				case 'g':
					ignoreErrors = true;
				break;

				case 'e':
				{
					if( cmd == cmdNOP )
						cmd = cmdRead43_14;

					slaveaddr = ModbusRTU::str2mbAddr( optarg );

					if( optind > argc )
					{
						cerr << "read command error: bad or no arguments..." << endl;
						return 1;
					}

					if( checkArg(optind, argc, argv) )
						devID = ModbusRTU::str2mbData(argv[optind]);

					if( checkArg(optind + 1, argc, argv) )
						objID = uni_atoi(argv[optind + 1]);
				}
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

					if( !checkArg(optind, argc, argv) )
					{
						cerr << "write command error: bad or no arguments..." << endl;
						return 1;
					}

					reg = ModbusRTU::str2mbData(argv[optind]);

					for( int o = optind + 1; o < argc; o++ )
					{
						DataInfo dval;
						char* arg = checkArg(o, argc, argv);

						if( arg == 0 )
							break;

						if( arg[0] == 'b' )
						{
							dval.type = 'i';
							string v(arg);
							string sb(v, 1);
							ModbusRTU::DataBits d(sb);
							dval.d.v = d.mbyte();
						}
						else if( arg[0] == 'm' )
						{
							dval.type = 'i';
							string v(arg);
							string sb(v, 1);
							dval.d.v = -1 * ModbusRTU::str2mbData(sb);
						}
						else if( arg[0] == 'f' || arg[0] == 'r' )
						{
							dval.type = arg[0];
							string v(arg);
							string sb(v, 1);
							dval.d.f = atof(sb.c_str());
						}
						else
						{
							dval.type = 'i';
							dval.d.v = ModbusRTU::str2mbData(arg);
						}

						data.emplace_back(dval);
						val = dval.d.v;
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

				case 'd':

					cmd = cmdDiag;
					slaveaddr = ModbusRTU::str2mbAddr( optarg );

					if( !checkArg(optind, argc, argv) )
					{
						cerr << "diagnostic command error: bad or no arguments..." << endl;
						return 1;
					}
					else
						subfunc = (ModbusRTU::DiagnosticsSubFunction)uni_atoi(argv[optind]);

					if( checkArg(optind + 1, argc, argv) )
						dat = uni_atoi(argv[optind + 1]);

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
				 << " mbaddr=" << ModbusRTU::addr2str(myaddr)
				 << " timeout=" << tout << " msec "
				 << endl;

			dlog->addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
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

		std::chrono::time_point<std::chrono::system_clock> start, end;

		while( nc )
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

						start = std::chrono::system_clock::now();
						ModbusRTU::ReadCoilRetMessage ret = mb.read01(slaveaddr, reg, count);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						if( verb )
							cout << "(reply): " << ret << endl;

						cout << "(reply): count=" << (int)ret.bcnt
							 << "(" << ModbusRTU::dat2str(ret.bcnt) << ")"
							 << " usec: " << elapsed_usec
							 << endl;

						for( int i = 0; i < ret.bcnt; i++ )
						{
							ModbusRTU::DataBits b(ret.data[i]);

							cout << setw(3) << i << ": "
								 << setw(6) << (reg + 8 * i)
								 << "(" << setw(6) << ModbusRTU::dat2str( reg + 8 * i ) << ") = "
								 << setw(5) << (int)(ret.data[i])
								 << "(" << ModbusRTU::b2str(ret.data[i]) << ") " << b << endl;
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

						start = std::chrono::system_clock::now();
						ModbusRTU::ReadInputStatusRetMessage ret = mb.read02(slaveaddr, reg, count);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						if( verb )
							cout << "(reply): " << ret << endl;

						cout << "(reply): count=" << (int)ret.bcnt
							 << "(" << ModbusRTU::dat2str(ret.bcnt) << ")"
							 << " usec: " << elapsed_usec
							 << endl;

						for( int i = 0; i < ret.bcnt; i++ )
						{
							ModbusRTU::DataBits b(ret.data[i]);

							cout << setw(3) << i << ": "
								 << setw(6) << (reg + 8 * i)
								 << "(" << setw(6) <<  ModbusRTU::dat2str( reg + 8 * i ) << ") = "
								 << setw(5) << (int)(ret.data[i])
								 << "(" << ModbusRTU::b2str(ret.data[i]) << ") " << b << endl;
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

						start = std::chrono::system_clock::now();
						ModbusRTU::ReadOutputRetMessage ret = mb.read03(slaveaddr, reg, count);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						if( verb )
							cout << "(reply): " << ret << endl;

						cout << "(reply): count=" << (int)ret.count
							 << "(" << ModbusRTU::dat2str(ret.count) << ")"
							 << " usec: " << elapsed_usec
							 << endl;

						for( size_t i = 0; i < ret.count; i++ )
						{
							ModbusRTU::DataBits16 b(ret.data[i]);

							cout << setw(3) << i << ": "
								 << setw(6) << ( reg + i )
								 << "(" << setw(6) << ModbusRTU::dat2str( reg + i ) << ") = "
								 << setw(5) << (int)(ret.data[i])
								 << " ("
								 << setw(5)
								 << ModbusRTU::dat2str(ret.data[i])
								 << ")"
								 << b
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

						start = std::chrono::system_clock::now();
						ModbusRTU::ReadInputRetMessage ret = mb.read04(slaveaddr, reg, count);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						if( verb )
							cout << "(reply): " << ret << endl;

						cout << "(reply): count=" << (int)ret.count
							 << "(" << ModbusRTU::dat2str(ret.count) << ")"
							 << " usec: " << elapsed_usec
							 << endl;

						for( size_t i = 0; i < ret.count; i++ )
						{
							ModbusRTU::DataBits16 b(ret.data[i]);

							cout << setw(3) << i << ": "
								 << setw(6) << ( reg + i )
								 << "(" << setw(6) << ModbusRTU::dat2str( reg + i ) << ") = "
								 << setw(5) << (int)(ret.data[i])
								 << " ("
								 << setw(5)
								 << ModbusRTU::dat2str(ret.data[i])
								 << ")"
								 << b
								 << endl;
						}
					}
					break;

					case cmdRead43_14:
					{
						if( verb )
						{
							cout << "read4314: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
								 << " devID=" << ModbusRTU::dat2str(devID)
								 << " objID=" << ModbusRTU::dat2str(objID)
								 << endl;
						}

						ModbusRTU::MEIMessageRetRDI ret = mb.read4314(slaveaddr, devID, objID);

						if( verb )
							cout << "(reply): " << ret << endl;
						else
							cout << "(reply): devID='" << (int)ret.devID << "' objNum='" << (int)ret.objNum << "'" << endl << ret.dlist << endl;
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

						start = std::chrono::system_clock::now();
						ModbusRTU::ForceSingleCoilRetMessage  ret = mb.write05(slaveaddr, reg, (bool)val);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						if( verb )
							cout << "(reply): " << ret
								 << " usec: " << elapsed_usec
								 << endl;
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

						start = std::chrono::system_clock::now();
						ModbusRTU::WriteSingleOutputRetMessage  ret = mb.write06(slaveaddr, reg, val);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						if( verb )
							cout << "(reply): " << ret
								 << " usec: " << elapsed_usec
								 << endl;

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

						ModbusRTU::ForceCoilsMessage msg(slaveaddr, reg);
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
								 << " data[" << data.size() << "]{ ";

							for( const auto& v : data )
							{
								if( v.type == 'f' )
									cout << v.d.f << "f ";
								else
									cout << ModbusRTU::dat2str(v.d.v) << " ";
							}

							cout << "}" << endl;
						}

						start = std::chrono::system_clock::now();
						ModbusRTU::WriteOutputMessage msg(slaveaddr, reg);
						end = std::chrono::system_clock::now();
						int elapsed_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

						for( const auto& v : data )
						{
							if( v.type == 'f' || v.type == 'r' )
							{
								ModbusRTU::ModbusData d[2];
								memcpy(&d, &(v.d.f), std::min(sizeof(d), sizeof(v.d.f)));

								if( v.type == 'r' )
									std::swap(d[0], d[1]);

								msg.addData(d[0]);
								msg.addData(d[1]);
							}
							else
								msg.addData(v.d.v);
						}

						ModbusRTU::WriteOutputRetMessage  ret = mb.write10(msg);

						if( verb )
							cout << "(reply): " << ret
								 << " usec: " << elapsed_usec
								 << endl;

					}
					break;

					case cmdDiag:
					{
						if( verb )
						{
							cout << "diag08: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
								 << " subfunc=" << ModbusRTU::dat2str(subfunc) << "(" << (int)subfunc << ")"
								 << " dat=" << ModbusRTU::dat2str(dat) << "(" << (int)dat << ")"
								 << endl;
						}

						ModbusRTU::DiagnosticRetMessage ret = mb.diag08(slaveaddr, subfunc, dat);

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
					break;

					case cmdCheck:
					{
						bool res = ModbusTCPMaster::checkConnection(iaddr, port, tout);
						cout << iaddr << ":" << port << " connection " << (res ? "OK" : "FAIL") << endl;
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
				if( ex.err == ModbusRTU::erTimeOut )
				{
					cout << "timeout..." << endl;
				}
				else
				{
					if( !ignoreErrors )
						throw;

					cerr << ex << endl;
				}
			}
			catch( std::exception& ex )
			{
				if( !ignoreErrors )
					throw;

				cerr << ex.what() << endl;
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
	catch( const uniset::Exception& ex )
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
void ping( const std::string& iaddr, int port )
{
	cerr << "ping2: check connection " << ModbusTCPMaster::checkConnection(iaddr, port, 1000) << endl;
}
// --------------------------------------------------------------------------
