/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <string>
#include <sstream>
#include <iomanip>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "modbus/ModbusRTUMaster.h"
#include "modbus/ModbusHelpers.h"
#include "modbus/ModbusTCPMaster.h"
#include "MTR.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "read03", required_argument, 0, 'r' },
	{ "read04", required_argument, 0, 'x' },
	{ "read-model", required_argument, 0, 'm' },
	{ "read-serial", required_argument, 0, 'n' },
	{ "device", required_argument, 0, 'd' },
	{ "verbose", no_argument, 0, 'v' },
	{ "speed", required_argument, 0, 's' },
	{ "use485F", no_argument, 0, 'y' },
	{ "num-cycles", required_argument, 0, 'l' },
	{ "timeout", required_argument, 0, 't' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "port", required_argument, 0, 'p' },
	{ "ignore-errors", no_argument, 0, 'g' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h|--help         - this message\n");
	printf("[--read03] slaveaddr reg mtrtype  - read from MTR (mtrtype: T1...T10,T16,T17,F1)\n");
	printf("[--read04] slaveaddr reg mtrtype  - read from MTR (mtrtype: T1...T10,T16,T17,F1)\n");
	printf("[-m|--read-model] slaveaddr     - read model name from MTR\n");
	printf("[-n|--read-serial] slaveaddr    - read serial number from MTR\n");
	printf("[-t|--timeout] msec             - Timeout. Default: 2000.\n");
	printf("[-l|--num-cycles] num           - Number of cycles of exchange. Default: -1 infinitely.\n");
	printf("[-g|--ignore-errors]            - Ignore errors\n");
	printf("[-v|--verbose]                  - Print all messages to stdout\n");
	printf("\nRTU prameters:\n");
	printf("[-y|--use485F]                  - use RS485 Fastwel.\n");
	printf("[-d|--device] dev               - use device dev. Default: /dev/ttyS0\n");
	printf("[-s|--speed] speed              - 9600,14400,19200,38400,57600,115200. Default: 38400.\n");
	printf("\nTCP prameters:\n");
	printf("[-i|--iaddr] ip                 - Modbus server ip. Default: 127.0.0.1\n");
	printf("[-p|--port] port                - Modbus server port. Default: 502.\n");
}
// --------------------------------------------------------------------------
enum Command
{
	cmdNOP,
	cmdRead03,
	cmdRead04,
	cmdModel,
	cmdSerial
};
// --------------------------------------------------------------------------
static char* checkArg( int ind, int argc, char* argv[] );
static void readMTR( ModbusClient* mb, ModbusRTU::ModbusAddr addr,
					 ModbusRTU::ModbusData reg, MTR::MTRType t, Command cmd );
// --------------------------------------------------------------------------

int main( int argc, char** argv )
{
	//	std::ios::sync_with_stdio(false);
	Command cmd = cmdNOP;
	int optindex = 0;
	int opt = 0;
	int verb = 0;
	string dev("/dev/ttyS0");
	string speed("38400");
	ModbusRTU::ModbusData reg = 0;
	ModbusRTU::ModbusAddr slaveaddr = 0x00;
	int tout = 2000;
	auto dlog = make_shared<DebugStream>();
	//    string tofile("");
	int use485 = 0;
	int ncycles = -1;
	MTR::MTRType mtrtype = MTR::mtUnknown;
	std::string iaddr;
	int port = 502;
	bool ignoreErrors = false;

	try
	{
		while(1)
		{
			opt = getopt_long(argc, argv, "hvygq:r:d:s:t:x:m:n:i:p:", longopts, &optindex);

			if( opt == -1 )
				break;

			switch (opt)
			{
				case 'h':
					print_help();
					return 0;

				case 'r':
				case 'x':
				{
					if( opt == 'r' )
						cmd = cmdRead03;
					else
						cmd = cmdRead04;

					slaveaddr = ModbusRTU::str2mbAddr( optarg );

					if( !checkArg(optind, argc, argv) )
					{
						cerr << "no argument is given: 'reg'.." << endl;
						return 1;
					}

					reg = ModbusRTU::str2mbData(argv[optind]);

					if( !checkArg(optind + 1, argc, argv) )
					{
						cerr << "no argument is given: 'mtrtype'.." << endl;
						return 1;
					}

					mtrtype = MTR::str2type(argv[optind + 1]);

					if( mtrtype == MTR::mtUnknown )
					{
						cerr << "command error: Unknown mtr type: '" << string(argv[optind + 1]) << "'" << endl;
						return 1;
					}
				}
				break;

				case 'm':
				{
					cmd = cmdModel;
					slaveaddr = ModbusRTU::str2mbAddr( optarg );
				}
				break;

				case 'g':
					ignoreErrors = true;
					break;

				case 'n':
				{
					cmd = cmdSerial;
					slaveaddr = ModbusRTU::str2mbAddr( optarg );
				}
				break;

				case 'y':
					use485 = 1;
					break;

				case 'd':
					dev = string(optarg);
					break;

				case 's':
					speed = string(optarg);
					break;

				case 't':
					tout = uni_atoi(optarg);
					break;

				case 'v':
					verb = 1;
					break;

				case 'l':
					ncycles = uni_atoi(optarg);
					break;

				case 'i':
					iaddr = string(optarg);
					break;

				case 'p':
					port = uni_atoi(optarg);
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
				 << " timeout=" << tout << " msec "
				 << endl;
		}

		ModbusClient* mb = nullptr;

		if( !iaddr.empty() )
		{
			auto mbtcp = new ModbusTCPMaster();
			mbtcp->connect(iaddr, port);
			//			mbtcp->setForceDisconnect(!persist);
			mb = mbtcp;
		}
		else
		{
			auto mbrtu = new ModbusRTUMaster(dev, use485);
			mbrtu->setSpeed(speed);
			mb = mbrtu;
		}

		if( verb )
			dlog->addLevel(Debug::ANY);

		mb->setTimeout(tout);
		mb->setLog(dlog);

		int nc = 1;

		if( ncycles > 0 )
			nc = ncycles;

		while( nc )
		{
			try
			{

				switch(cmd)
				{
					case cmdRead03:
					case cmdRead04:
					{
						if( verb )
						{
							cout << " slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
								 << " reg=" << ModbusRTU::dat2str(reg) << "(" << (int)reg << ")"
								 << " mtrType=" << MTR::type2str(mtrtype)
								 << endl;
						}

						readMTR( mb, slaveaddr, reg, mtrtype, cmd );
					}
					break;

					case cmdModel:
					{
						if( verb )
						{
							cout << " read model name: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
								 << endl;
						}

						string s = MTR::getModelNumber(mb, slaveaddr);
						cout << (s.empty() ? "Don`t read model name." : s) << endl;
						return 0;
					}
					break;

					case cmdSerial:
					{
						if( verb )
						{
							cout << " read serial number: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
								 << endl;
						}

						string s(MTR::getSerialNumber(mb, slaveaddr));
						cout << (s.empty() ? "Don`t read serial number." : s) << endl;
						return 0;
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

			msleep(500);
		}
	}
	catch( const ModbusRTU::mbException& ex )
	{
		cerr << "(mtr-read): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(mtr-read): " << ex.what() << endl;
	}
	catch(...)
	{
		cerr << "(mtr-read): catch(...)" << endl;
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
void readMTR( ModbusClient* mb, ModbusRTU::ModbusAddr addr,
			  ModbusRTU::ModbusData reg, MTR::MTRType mtrType, Command cmd )
{
	int count = MTR::wsize(mtrType);
	ModbusRTU::ModbusData dat[ModbusRTU::MAXLENPACKET / sizeof(ModbusRTU::ModbusData)];
	memset(dat, 0, sizeof(dat));

	if( cmd == cmdRead03 )
	{
		ModbusRTU::ReadOutputRetMessage ret = mb->read03(addr, reg, count);
		memcpy(dat, ret.data, sizeof(ModbusRTU::ModbusData)*ret.count);
	}
	else if( cmd == cmdRead04 )
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04(addr, reg, count);
		memcpy(dat, ret.data, sizeof(ModbusRTU::ModbusData)*ret.count);
	}
	else
	{
		cerr << "Unknown command..." << endl;
		return;
	}

	if( mtrType == MTR::mtT1 )
	{
		MTR::T1 t(dat[0]);
		cout << "(T1): v1=" << dat[0] << " --> (unsigned) " << t.val << endl;
		return;
	}

	if( mtrType == MTR::mtT2 )
	{
		MTR::T2 t(dat[0]);
		cout << "(T2): v1=" << dat[0] << " --> (signed) " << t.val << endl;
		return;
	}

	if( mtrType == MTR::mtT3 )
	{
		MTR::T3 t(dat, MTR::T3::wsize());
		cout << "(T3): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " --> " << t << endl;
		return;
	}

	if( mtrType == MTR::mtT4 )
	{
		MTR::T4 t(dat[0]);
		cout << "(T4): v1=" << t.raw
			 << " --> " << t << endl;
		return;
	}

	if( mtrType == MTR::mtT5 )
	{
		MTR::T5 t(dat, MTR::T5::wsize());
		cout << "(T5): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " --> " << t << endl;
		return;
	}

	if( mtrType == MTR::mtT6 )
	{
		MTR::T6 t(dat, MTR::T6::wsize());
		cout << "(T6): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " --> " << t << endl;
		return;
	}

	if( mtrType == MTR::mtT7 )
	{
		MTR::T7 t(dat, MTR::T7::wsize());
		cout << "(T7): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " ===> " << t << endl;
		return;
	}

	if( mtrType == MTR::mtT16 )
	{
		MTR::T16 t(dat[0]);
		cout << "(T16): v1=" << t.val << " float=" << t.fval << endl;
		return;
	}

	if( mtrType == MTR::mtT17 )
	{
		MTR::T17 t(dat[0]);
		cout << "(T17): v1=" << t.val << " float=" << t.fval << endl;
		return;
	}

	if( mtrType == MTR::mtF1 )
	{
		MTR::F1 f(dat, MTR::F1::wsize());
		cout << "(F1): v1=" << f.raw.v[0] << " v2=" << f.raw.v[1]
			 << " ===> " << f << endl;
		return;
	}

	if( mtrType == MTR::mtT8 )
	{
		MTR::T8 t(dat, MTR::T8::wsize());
		cout << "(T8): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " ===> " << t << endl;
		return;
	}

	if( mtrType == MTR::mtT9 )
	{
		MTR::T9 t(dat, MTR::T9::wsize());
		cout << "(T9): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " ===> " << t << endl;

		return;
	}

	if( mtrType == MTR::mtT10 )
	{
		MTR::T10 t(dat, MTR::T10::wsize());
		cout << "(T10): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			 << " ===> " << t << endl;

		return;
	}

	cerr << "Unsupported mtrtype='" << MTR::type2str(mtrType) << "'" << endl;
}
