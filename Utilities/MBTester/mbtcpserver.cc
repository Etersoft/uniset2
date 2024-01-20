// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "MBTCPServer.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "iaddr", required_argument, 0, 'i' },
    { "verbose", no_argument, 0, 'v' },
    { "myaddr", required_argument, 0, 'a' },
    { "port", required_argument, 0, 'p' },
    { "const-reply", required_argument, 0, 'c' },
    { "after-send-pause", required_argument, 0, 's' },
    { "max-sessions", required_argument, 0, 'm' },
    { "random", optional_argument, 0, 'r' },
    { "freeze", required_argument, 0, 'z' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("Example: uniset-mbtcpserver-echo -i localhost -p 2049 -v \n");
    printf("-h|--help                               - this message\n");
    printf("[-v|--verbose]                          - Print all messages to stdout\n");
    printf("[-i|--iaddr] ip                         - Server listen ip. Default 127.0.0.1\n");
    printf("[-a|--myaddr] addr1,addr2,...           - Modbus address for master. Default: 0x01.\n");
    printf("                    myaddr=0            - Reply to all RTU-addresses (broadcast).\n");
    printf("[-p|--port] port                        - Server port. Default: 502.\n");
    printf("[-c|--const-reply] val                  - Reply 'val' for all queries\n");
    printf("[-s|--after-send-pause] msec            - Pause after send request. Default: 0\n");
    printf("[-m|--max-sessions] num                 - Set the maximum number of sessions. Default: 10\n");
    printf("[-r|--random] [min,max]                 - Reply random value for all queries. Default: [0,65535]\n");
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
    int port = 502;
    string iaddr("127.0.0.1");
    string myaddr("0x01");
    auto dlog = make_shared<DebugStream>();
    int replyVal = -1;
    timeout_t afterpause = 0;
    size_t maxSessions = 10;
    int min = 0;
    int max = 65535;
    bool random = false;
    std::unordered_map<uint16_t, uint16_t> reglist = {};

    try
    {
        while(1)
        {
            opt = getopt_long(argc, argv, "hva:p:i:c:s:m:rz:", longopts, &optindex);

            if( opt == -1 )
                break;

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

                case 'a':
                    myaddr = string(optarg);
                    break;

                case 'v':
                    verb = 1;
                    break;

                case 'c':
                    replyVal = uni_atoi(optarg);
                    break;

                case 's':
                    afterpause = uni_atoi(optarg);
                    break;

                case 'm':
                    maxSessions = uni_atoi(optarg);
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

        auto avec = uniset::explode_str(myaddr, ',');
        std::unordered_set<ModbusRTU::ModbusAddr> vaddr;

        for( const auto& a : avec )
            vaddr.emplace( ModbusRTU::str2mbAddr(a) );

        if( verb )
        {
            cout << "(init): iaddr: " << iaddr << ":" << port
                 << " myaddr=" << ModbusServer::vaddr2str(vaddr)
                 << endl;

            dlog->addLevel( Debug::ANY );
        }

        MBTCPServer mbs(vaddr, iaddr, port, verb);
        mbs.setLog(dlog);
        mbs.setVerbose(verb);
        mbs.setAfterSendPause(afterpause);
        mbs.setMaxSessions(maxSessions);

        if( replyVal != -1 )
            mbs.setReply(replyVal);

        if( random )
            mbs.setRandomReply(min, max);

        if( !reglist.empty()  )
            mbs.setFreezeReply(reglist);

        mbs.execute();
    }
    catch( const ModbusRTU::mbException& ex )
    {
        cerr << "(mbtcpserver): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(mbtcpserver): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(mbtcpserver): catch(...)" << endl;
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
