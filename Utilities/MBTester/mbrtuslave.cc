// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "MBSlave.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "device", required_argument, 0, 'd' },
    { "verbose", no_argument, 0, 'v' },
    { "myaddr", required_argument, 0, 'a' },
    { "speed", required_argument, 0, 's' },
    { "use485F", no_argument, 0, 'y' },
    { "const-reply", required_argument, 0, 'c' },
    { "random", optional_argument, 0, 'r' },
    { "freeze", required_argument, 0, 'z' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("-h|--help              - this message\n");
    printf("[-v|--verbose]         - Print all messages to stdout\n");
    printf("[-d|--device] dev      - use device dev. Default: /dev/ttyS0\n");
    printf("[-a|--myaddr] addr     - Modbus address for master. Default: 0x01.\n");
    printf("[-s|--speed] speed     - 9600,14400,19200,38400,57600,115200. Default: 38400.\n");
    printf("[-y|--use485F]         - use RS485 Fastwel.\n");
    printf("[-v|--verbose]         - Print all messages to stdout\n");
    printf("[-c|--const-reply] val1 [val2 val3] - Reply val for all queries\n");
    printf("[-r|--random] [min,max] - Reply random value for all queries. Default: [0,65535]\n");
    printf("[-z|--freeze] reg1=val1,reg2=val2,...   - Reply value for some registers.\n");
}
// --------------------------------------------------------------------------
static char* checkArg( int ind, int argc, char* argv[] );
// --------------------------------------------------------------------------
int main( int argc, char** argv )
{
    //  std::ios::sync_with_stdio(false);
    int optindex = 0;
    int opt = 0;
    int verb = 0;
    string dev("/dev/ttyS0");
    string speed("38400");
    ModbusRTU::ModbusAddr myaddr = 0x01;
    auto dlog = make_shared<DebugStream>();
    int use485 = 0;
    int replyVal = -1;
    int replyVal2 = -1;
    int replyVal3 = -1;
    int min = 0;
    int max = 65535;
    bool random = false;
    std::unordered_map<uint16_t, uint16_t> reglist = {};

    try
    {
        while(1)
        {
            opt = getopt_long(argc, argv, "hva:d:s:yc:rz:", longopts, &optindex);

            if( opt == -1 )
                break;

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

                case 'a':
                    myaddr = ModbusRTU::str2mbAddr(optarg);
                    break;

                case 'v':
                    verb = 1;
                    break;

                case 'y':
                    use485 = 1;
                    break;

                case 'c':
                    replyVal = uni_atoi(optarg);

                    if( checkArg(optind, argc, argv) )
                        replyVal2 = uni_atoi(argv[optind]);

                    if( checkArg(optind + 1, argc, argv) )
                        replyVal3 = uni_atoi(argv[optind + 1]);

                    break;

                case 'r':
                    random = true;

                    if( checkArg(optind, argc, argv) )
                    {
                        auto tmp = uniset::explode_str(argv[optind], ',');

                        if( tmp.size() < 2 )
                        {
                            cerr << "Unknown min, max for random. Use -r min,max" << endl;
                            return 1;
                        }

                        min = uni_atoi(tmp[0]);
                        max = uni_atoi(tmp[1]);
                    }

                    break;

                case 'z':
                {
                    auto str = uniset::explode_str(optarg, ',');

                    if( str.size() < 1 )
                    {
                        cerr << "Unknown reg or value. Use -z reg1=value1,reg2=value" << endl;
                        return 1;
                    }

                    for (const auto& s : str)
                    {
                        auto tmp = uniset::explode_str(s, '=');

                        if( tmp.size() < 2 )
                        {
                            cerr << "Error in \"" << s << "\". Use -z reg1=value1,reg2=value" << endl;
                            return 1;
                        }
                        reglist[uni_atoi(tmp[0])] = uni_atoi(tmp[1]);
                    }
                }

                break;

                case '?':
                default:
                    printf("? argument\n");
                    return 0;
            }
        }

        if( verb )
        {
            cout << "(init): dev=" << dev << " speed=" << speed
                 << " myaddr=" << ModbusRTU::addr2str(myaddr)
                 << endl;

            dlog->addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
        }

        /*! \todo Доделать возможность задавать много адресов для ответа */
        std::unordered_set<ModbusRTU::ModbusAddr> vaddr;
        vaddr.emplace(myaddr);

        MBSlave mbs(vaddr, dev, speed, use485);
        mbs.setLog(dlog);
        mbs.setVerbose(verb);

        if( replyVal != -1 )
            mbs.setReply(replyVal);

        if( replyVal2 != -1 )
            mbs.setReply2(replyVal2);

        if( replyVal3 != -1 )
            mbs.setReply3(replyVal3);

        if( !reglist.empty()  )
            mbs.setFreezeReply(reglist);

        if( random )
            mbs.setRandomReply(min, max);

        mbs.execute();
    }
    catch( ModbusRTU::mbException& ex )
    {
        cerr << "(mbtester): " << ex << endl;
    }
    catch( const std::exception& ex )
    {
        cerr << "(mbslave): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(mbslave): catch(...)" << endl;
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
