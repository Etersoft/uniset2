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

    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("-h|--help         - this message\n");
    printf("[--write05] slaveaddr reg val  - write val to reg for slaveaddr\n");
    printf("[--write06] slaveaddr reg val  - write val to reg for slaveaddr\n");
    printf("[--write0F] slaveaddr reg val  - write val to reg for slaveaddr\n");
    printf("[--write10] slaveaddr reg val count  - write val to reg for slaveaddr\n");
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
    cmdDiag
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
    int val = 0;
    ModbusRTU::ModbusData count = 1;
    ModbusRTU::ModbusAddr myaddr = 0x01;
    ModbusRTU::ModbusAddr slaveaddr = 0x00;
    ModbusRTU::DiagnosticsSubFunction subfunc = ModbusRTU::subEcho;
    ModbusRTU::ModbusData dat = 0;
    int tout = 2000;
    DebugStream dlog;
    int ncycles = -1;
    ModbusRTU::ModbusByte devID = 0;
    ModbusRTU::ModbusByte objID = 0;

    try
    {
        while( (opt = getopt_long(argc, argv, "hva:w:z:r:x:c:b:d:s:t:p:i:ol:d:e:u:",longopts,&optindex)) != -1 ) 
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
                {
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
                }
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

                    if( checkArg(optind,argc,argv) )
                        devID = ModbusRTU::str2mbData(argv[optind]);
                    
                    if( checkArg(optind+1,argc,argv) )
                        objID = uni_atoi(argv[optind+1]);
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
                    
                    if( !checkArg(optind,argc,argv) )
                    {
                        cerr << "write command error: bad or no arguments..." << endl;
                        return 1;
                    }
                    reg = ModbusRTU::str2mbData(argv[optind]);

                    if( checkArg(optind+1,argc,argv) )
                    {
                        if( (argv[optind+1])[0] == 'b' )
                        {
                            string v(argv[optind+1]);
                            string sb(v,1);
                            ModbusRTU::DataBits d(sb);
                            val = d.mbyte();
                        }
                        else if( (argv[optind+1])[0] == 'm' )
                        {
                            string v(argv[optind+1]);
                            string sb(v,1);
                            val = -1 * ModbusRTU::str2mbData(sb);
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

                case 'l':
                    ncycles = uni_atoi(optarg);
                break;

                case 'd':

                    cmd = cmdDiag;
                    slaveaddr = ModbusRTU::str2mbAddr( optarg );
                    
                    if( !checkArg(optind,argc,argv) )
                    {
                        cerr << "diagnostic command error: bad or no arguments..." << endl;
                        return 1;
                    }
                    else
                        subfunc = (ModbusRTU::DiagnosticsSubFunction)uni_atoi(argv[optind]);
                        
                    if( checkArg(optind+1,argc,argv) )
                        dat = uni_atoi(argv[optind+1]);
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

            dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
        }
        
        ModbusTCPMaster mb;
        mb.setLog(dlog);

//        ost::Thread::setException(ost::Thread::throwException);
        ost::InetAddress ia(iaddr.c_str());
        mb.setTimeout(tout);
        mb.connect(ia,port);
        
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
                    
                        cout << i <<": (" << ModbusRTU::dat2str( reg + 8*i ) << ") = (" 
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
                    
                        cout << i <<": (" << ModbusRTU::dat2str( reg + 8*i ) << ") = (" 
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

                case cmdRead43_14:
                {
                    if( verb )
                    {
                        cout << "read4314: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
                             << " devID=" << ModbusRTU::dat2str(devID) 
                             << " objID=" << ModbusRTU::dat2str(objID) 
                             << endl;
                    }

                    ModbusRTU::MEIMessageRetRDI ret = mb.read4314(slaveaddr,devID,objID);
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

                case cmdDiag:
                {
                    if( verb )
                    {
                        cout << "diag08: slaveaddr=" << ModbusRTU::addr2str(slaveaddr)
                             << " subfunc=" << ModbusRTU::dat2str(subfunc) << "(" << (int)subfunc << ")"
                             << " dat=" << ModbusRTU::dat2str(dat) << "(" << (int)dat << ")"
                             << endl;
                    }

                    ModbusRTU::DiagnosticRetMessage ret = mb.diag08(slaveaddr,subfunc,dat);
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

                case cmdNOP:
                default:
                    cerr << "No command. Use -h for help." << endl;
                    return 1;
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
                if( nc <=0 )
                    break;
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
    catch( std::exception& e )
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
    if( i<argc && (argv[i])[0]!='-' )
        return argv[i];
        
    return 0;
}
// --------------------------------------------------------------------------
