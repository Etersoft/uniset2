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
#include <map>
#include <list>
#include <fstream>
#include <iomanip>
#include <getopt.h>
#include <math.h>
#include "Debug.h"
#include "modbus/ModbusRTUMaster.h"
#include "modbus/ModbusTCPMaster.h"
#include "modbus/ModbusHelpers.h"
#include "extensions/MTR.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "read", required_argument, 0, 'r' },
	{ "save", required_argument, 0, 'w' },
	{ "timeout", required_argument, 0, 't' },
	{ "autodetect-slave", required_argument, 0, 'l' },
	{ "autodetect-speed", required_argument, 0, 'n' },
	{ "device", required_argument, 0, 'd' },
	{ "verbose", no_argument, 0, 'v' },
	{ "speed", required_argument, 0, 's' },
	{ "stop-bits", required_argument, 0, 'o' },
	{ "parity", required_argument, 0, 'a' },
	{ "use485F", no_argument, 0, 'y' },
	{ "min-addr", required_argument, 0, 'b' },
	{ "max-addr", required_argument, 0, 'e' },
	{ "model", required_argument, 0, 'x' },
	{ "serial", required_argument, 0, 'z' },
	{ "iaddr", required_argument, 0, 'i' },
	{ "port", required_argument, 0, 'p' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
	printf("-h|--help                 - this message\n");
	printf("[--read] mtraddr          - read configuration from MTR\n");
	printf("[--save] mtraddr confile  - save configureation to MTR\n");
	printf("             mtraddr=0x00 - autodetect addr\n");
	printf("[-t|--timeout] msec       - Timeout. Default: 2000.\n");
	printf("[-v|--verbose]            - Print all messages to stdout\n");
	printf("[--autodetect-slave] [reg fn]  - find slave\n");
	printf("          reg - register of test. Default: 0\n");
	printf("          fn - function of test [0x01,0x02,0x03,0x04]. Default: 0x04\n");
	printf("[--min-addr] - start addres for autodetect. Default: 0\n");
	printf("[--max-addr] - end addres for autodetect. Default: 254\n");
	printf("\nRTU prameters:\n");
	printf("[-y|--use485F]            - use RS485 Fastwel.\n");
	printf("[-d|--device] dev         - use device dev. Default: /dev/ttyS0\n");
	printf("[-s|--speed] speed        - 9600,14400,19200,38400,57600,115200. Default: 38400.\n");
	printf("[--parity] par            - parity [odd,even,no]. Default: no\n");
	printf("[--stop-bits] n           - stop bits [1,2]. Default: 1\n");
	printf("[--autodetect-speed] slaveaddr [reg fn]  - detect speed\n");
	printf("          reg - register of test. Default: 0\n");
	printf("          fn - function of test [0x01,0x02,0x03,0x04]. Default: 0x04\n");
	printf("\nTCP prameters:\n");
	printf("[-i|--iaddr] ip                 - Modbus server ip. Default: 127.0.0.1\n");
	printf("[-p|--port] port                - Modbus server port. Default: 502.\n");

	printf("\n");
}
// --------------------------------------------------------------------------
enum Command
{
	cmdNOP,
	cmdRead,
	cmdSave,
	cmdDetectSpeed,
	cmdDetectSlave,
	cmdGetModel,
	cmdGetSerial
};
// --------------------------------------------------------------------------
static char* checkArg( int ind, int argc, char* argv[] );
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
	string mtrconfile("");
	string par("");
	ModbusRTU::ModbusData reg = 0;
	ModbusRTU::ModbusAddr slaveaddr = 0x00;
	ModbusRTU::SlaveFunctionCode fn = ModbusRTU::fnReadInputRegisters;
	ModbusRTU::ModbusAddr beg = 0;
	ModbusRTU::ModbusAddr end = 254;
	int tout = 20;
	auto dlog = make_shared<DebugStream>();
	//string tofile("");
	int use485 = 0;
	ComPort::StopBits sbits = ComPort::OneBit;
	ComPort::Parity parity = ComPort::NoParity;
	string iaddr("127.0.0.1");
	int port = 502;

	try
	{
		while(1)
		{
			opt = getopt_long(argc, argv, "hvw:r:x:d:s:t:l:n:yb:e:x:z:i:p:o:a:", longopts, &optindex);

			if( opt == -1 )
				break;

			switch (opt)
			{
				case 'h':
					print_help();
					return 0;

				case 'r':
					cmd = cmdRead;
					slaveaddr = ModbusRTU::str2mbAddr(optarg);
					break;

				case 'w':
					cmd = cmdSave;
					slaveaddr = ModbusRTU::str2mbAddr( optarg );

					if( !checkArg(optind, argc, argv) )
					{
						cerr << "read command error: bad or no arguments..." << endl;
						return 1;
					}
					else
						mtrconfile = string(argv[optind]);

					break;

				case 'x':
					cmd = cmdGetModel;
					slaveaddr = ModbusRTU::str2mbAddr(optarg);
					break;

				case 'z':
					cmd = cmdGetSerial;
					slaveaddr = ModbusRTU::str2mbAddr(optarg);
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

				case 'a':
					par = string(optarg);

					if( !par.compare("odd") )
						parity = ComPort::Odd;
					else if( !par.compare("even") )
						parity = ComPort::Even;

					break;

#undef atoi

				case 't':
					tout = atoi(optarg);
					break;

				case 'o':
					if( atoi(optarg) == 2 )
						sbits = ComPort::TwoBits;

					break;

				case 'b':
					beg = atoi(optarg);
					break;

				case 'e':
					end = atoi(optarg);
					break;

				//                case 'a':
				//                    myaddr = ModbusRTU::str2mbAddr(optarg);
				//                break;

				case 'v':
					verb = 1;
					break;

				case 'l':
				{
					if( cmd == cmdNOP )
						cmd = cmdDetectSlave;

					if( !checkArg(optind, argc, argv) )
						break;

					reg = ModbusRTU::str2mbData(argv[optind + 2]);

					if( !checkArg(optind + 1, argc, argv) )
						break;

					fn = (ModbusRTU::SlaveFunctionCode)uniset::uni_atoi(argv[optind + 3]);
				}
				break;

				case 'n':
				{
					if( cmd == cmdNOP )
						cmd = cmdDetectSpeed;

					slaveaddr = ModbusRTU::str2mbAddr(optarg);

					if( !checkArg(optind, argc, argv) )
						break;

					reg = ModbusRTU::str2mbData(argv[optind]);

					if( !checkArg(optind + 1, argc, argv) )
						break;

					fn = (ModbusRTU::SlaveFunctionCode)uniset::uni_atoi(argv[optind + 1]);
				}
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
			mbrtu->setParity(parity);
			mbrtu->setStopBits(sbits);
			mb = mbrtu;
		}

		if( verb )
			dlog->addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );

		mb->setTimeout(tout);
		mb->setLog(dlog);

		switch(cmd)
		{
			case cmdRead:
			{
				if( verb )
					cout << "(mtr-setup): read: slaveaddr=" << ModbusRTU::addr2str(slaveaddr) << endl;
			}
			break;

			case cmdSave:
			{
				if( slaveaddr == 0x00 )
				{
					if( verb )
						cout << "(mtr-setup): save: autodetect slave addr... (speed=" << speed <<  ")" << endl;

					mb->setTimeout(50);
					slaveaddr = ModbusHelpers::autodetectSlave(mb, beg, end, MTR::regModelNumber, ModbusRTU::fnReadInputRegisters);
					mb->setTimeout(tout);
				}

				if( speed.empty() )
				{
					if( verb )
						cout << "(mtr-setup): save: autodetect speed... (addr=" << ModbusRTU::addr2str(slaveaddr) << ")" << endl;

					auto mbrtu = dynamic_cast<ModbusRTUMaster*>(mb);

					if( mbrtu )
					{
						mb->setTimeout(50);
						ComPort::Speed s = ModbusHelpers::autodetectSpeed(mbrtu, slaveaddr, MTR::regModelNumber, ModbusRTU::fnReadInputRegisters);
						mbrtu->setSpeed(s);
						mbrtu->setTimeout(tout);
					}
				}

				if( verb )
					cout << "(mtr-setup): save: "
						 << " slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << " confile=" << mtrconfile
						 << " speed=" << speed
						 << endl;

				return  MTR::update_configuration(mb, slaveaddr, mtrconfile, verb) ? 0 : 1;
			}
			break;

			case cmdDetectSlave:
			{
				if( verb )
				{
					cout << "(mtr-setup): autodetect slave: "
						 << " beg=" << ModbusRTU::addr2str(beg)
						 << " end=" << ModbusRTU::addr2str(end)
						 << " reg=" << ModbusRTU::dat2str(reg)
						 << " fn=" << ModbusRTU::b2str(fn)
						 << endl;
				}

				try
				{
					ModbusRTU::ModbusAddr a = ModbusHelpers::autodetectSlave(mb, beg, end, reg, fn);
					cout << "(mtr-setup): autodetect modbus slave: " << ModbusRTU::addr2str(a) << endl;
				}
				catch( const uniset::TimeOut& ex )
				{
					cout << "(mtr-setup): slave not autodetect..." << endl;
				}

				break;
			}

			case cmdDetectSpeed:
			{
				auto mbrtu = dynamic_cast<ModbusRTUMaster*>(mb);

				if( !mbrtu )
				{
					cerr << "autodetect speed only for RTU interface.." << endl;
					return 1;
				}

				if( verb )
				{
					cout << "(mtr-setup): autodetect speed: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << " reg=" << ModbusRTU::dat2str(reg)
						 << " fn=" << ModbusRTU::b2str(fn)
						 << endl;
				}

				try
				{
					ComPort::Speed s = ModbusHelpers::autodetectSpeed(mbrtu, slaveaddr, reg, fn);
					cout << "(mtr-setup): autodetect: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << " speed=" << ComPort::getSpeed(s) << endl;
				}
				catch( const uniset::TimeOut& ex )
				{
					cout << "(mtr-setup): speed not autodetect for slaveaddr="
						 << ModbusRTU::addr2str(slaveaddr) << endl;
				}
			}
			break;

			case cmdGetModel:
			{
				if( verb )
				{
					cout << "(mtr-setup): model: "
						 << " slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << endl;
				}

				cout << "model: " << MTR::getModelNumber(mb, slaveaddr) << endl;
			}
			break;

			case cmdGetSerial:
			{
				if( verb )
				{
					cout << "(mtr-setup): serial: "
						 << " slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
						 << endl;
				}

				cout << "serial: " << MTR::getSerialNumber(mb, slaveaddr) << endl;
			}
			break;

			case cmdNOP:
			default:
				cerr << "No command. Use -h for help." << endl;
				return 1;
		}

		return 0;
	}
	catch( const ModbusRTU::mbException& ex )
	{
		cerr << "(mtr-setup): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(mtr-setup): " << ex.what() << endl;
	}
	catch(...)
	{
		cerr << "(mtr-setup): catch(...)" << endl;
	}

	return 1;
}
// --------------------------------------------------------------------------
char* checkArg( int i, int argc, char* argv[] )
{
	if( i < argc && (argv[i])[0] != '-' )
		return argv[i];

	return 0;
}
// --------------------------------------------------------------------------
