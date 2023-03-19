#include <cstdlib>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include "UniSetTypes.h"
#include "PassiveTimer.h"
#include "OPCUAClient.h"
// --------------------------------------------------------------------------
static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "host", required_argument, 0, 'x' },
    { "read", required_argument, 0, 'r' },
    { "write", required_argument, 0, 'w' },
    { "timeout", required_argument, 0, 't' },
    { "verbode", no_argument, 0, 'v' },
    { "num-cycles", required_argument, 0, 'z' },
    { "read-pause", required_argument, 0, 's' },
    { "user", required_argument, 0, 'u' },
    { "pass", required_argument, 0, 'p' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace std::chrono;
// --------------------------------------------------------------------------
enum Command
{
    cmdNOP,
    cmdRead,
    cmdWrite
};
// --------------------------------------------------------------------------
char* checkArg( int i, int argc, char* argv[] )
{
    if( i < argc && (argv[i])[0] != '-' )
        return argv[i];

    return 0;
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int optindex = 0;
    int opt = 0;
    Command cmd = cmdNOP;
    int verb = 0;
    std::string addr = "localhost:4840";
    std::string user = "";
    std::string pass = "";
    int msecpause = 200;
    timeout_t tout = UniSetTimer::WaitUpTime;
    size_t ncycles = 0;
    std::vector<UA_ReadValueId> rvalues;
    std::vector<UA_WriteValue> wvalues;
    std::vector<std::string> attrs;

    while(1)
    {
        opt = getopt_long(argc, argv, "hx:r:w:t:vz:i:p:u:s:", longopts, &optindex);

        if( opt == -1 )
            break;

        switch (opt)
        {
            case 'h':
                cout << "-h|--help                - this message" << endl;
                cout << "[-x|--host] host:port    - OPC UA server address. Default: localhost:4840" << endl;
                cout << "[-r|--read] nodeid       - Read variable" << endl;
                cout << "[-w|--write] nodeid val  - Write variable" << endl;
                cout << "[-t|--timeout] msec      - timeout for receive. Default: 0 msec (waitup)." << endl;
                cout << "[-v|--verbose]           - verbose mode." << endl;
                cout << "[-z|--num-cycles] num    - Number of cycles of exchange. Default: -1 - infinitely." << endl;
                cout << "[-s|--read-pause] msec   - Pause between read" << endl;
                cout << "[-u|--user] name         - Auth: user" << endl;
                cout << "[-p|--pass] pass         - Auth: password" << endl;
                cout << endl;
                cout << "Atribute NodeId examples:" << endl;
                cout << "* i=13" << endl;
                cout << "* ns=10;i=1" << endl;
                cout << "* ns=10;s=Hello:World" << endl;
                cout << "* g=09087e75-8e5e-499b-954f-f2a9603db28a" << endl;
                cout << "* ns=1;b=b3BlbjYyNTQxIQ==    // base64" << endl;
                cout << endl;
                cout << "Examples:" << endl;
                cout << "uniset2-opcua-tester -x opc.tcp://localhost:53530/OPCUA/SimulationServer -r \"ns=3;i=1001\" -v" << endl;
                cout << "uniset2-opcua-tester -x opc.tcp://localhost:53530/OPCUA/SimulationServer -w \"ns=3;i=1007\" 42 -v" << endl;
                return 0;

            case 'x':
                addr = string(optarg);
                break;

            case 'u':
                user = string(optarg);
                break;

            case 'p':
                pass = string(optarg);
                break;

            case 'r':
                cmd = cmdRead;
                rvalues.emplace_back(OPCUAClient::makeReadValue32(string(optarg)));
                attrs.push_back(string(optarg));
                break;

            case 'w':
            {
                cmd = cmdWrite;

                if( !checkArg(optind, argc, argv) )
                {
                    cerr << "write must have to parameters 'name value'" << endl;
                    return 1;
                }

                string name = string(optarg);
                int value = uni_atoi(argv[optind]);
                wvalues.emplace_back(OPCUAClient::makeWriteValue32(name, value));
                attrs.push_back(name);
            }
            break;

            case 't':
                tout = atoi(optarg);
                break;

            case 's':
                msecpause = atoi(optarg);
                break;

            case 'v':
                verb = 1;
                break;

            case 'z':
                ncycles = atoi(optarg);
                break;

            case '?':
            default:
                cerr << "? argumnet" << endl;
                return 0;
        }
    }

    if( cmd == cmdNOP )
    {
        cerr << "No command... Use -h for help" << endl;
        return -1;
    }

    try
    {
        const string prefix = "opc.tcp://";

        if( !addr.compare(0, prefix.size() - 1, prefix) )
            addr = "opc.tcp://" + addr;

        if( verb )
        {
            cout << addr
                 << " timeout=";

            if( tout == UniSetTimer::WaitUpTime )
                cout << "Waitup";
            else
                cout << tout;

            cout << " msecpause=" << msecpause << endl;
        }

        auto client = std::make_shared<OPCUAClient>();

        if( user.empty() )
        {
            if (!client->connect(addr))
            {
                cerr << "Can't connect to " << addr << endl;
                return 1;
            }
        }
        else
        {
            if (!client->connect(addr, user, pass))
            {
                cerr << "Can't connect to " << addr  << " user: " << user << endl;
                return 1;
            }
        }

        switch( cmd )
        {
            case cmdRead:
            {
                int nc = 1;

                if( ncycles > 0 )
                    nc = ncycles;

                std::vector<OPCUAClient::ResultVar> result;

                for( size_t i = 0; i < rvalues.size(); i++ )
                    result.push_back(OPCUAClient::ResultVar());

                while( nc )
                {
                    try
                    {
                        auto t_start = steady_clock::now();
                        auto ret = client->read(rvalues, result);
                        auto t_end = steady_clock::now();

                        if( ret == 0 )
                        {
                            for( size_t i = 0; i < result.size(); i++ )
                                cout << attrs[i] << ": value=" << result[i].get()
                                     << " status[" << UA_StatusCode_name(result[i].status) << "]"
                                     << " update: " << setw(10) << setprecision(7) << std::fixed
                                     << duration_cast<duration<float>>(t_end - t_start).count() << " sec"
                                     << endl;
                        }
                        else
                        {
                            cout << "read: error=" << ret << endl;
                        }
                    }
                    catch( std::exception& ex )
                    {
                        cerr << "(recv): " << ex.what() << endl;
                    }
                    catch( ... )
                    {
                        cerr << "(recv): catch ..." << endl;
                    }

                    if( ncycles > 0 )
                    {
                        nc--;

                        if( nc <= 0 )
                            break;
                    }

                    msleep(msecpause);
                }
            }
            break;

            case cmdWrite:
            {
                if( verb )
                    cout << "write:" << endl;

                for( size_t i = 0; i < attrs.size(); i++ )
                    cout << attrs[i]  << " = " << (*(int32_t*)wvalues[i].value.value.data) << endl;

                if( verb )
                    cout << endl;

                try
                {
                    auto ret = client->write32(wvalues);

                    if( ret != 0 )
                    {
                        cerr << "write error code " << ret << endl;

                        for( size_t i = 0; i < wvalues.size(); i++ )
                            cerr << attrs[i] << ": status=" << UA_StatusCode_name(wvalues[i].value.status) << endl;
                    }
                    else if( verb )
                    {
                        cout << "write ok" << endl;
                    }

                }
                catch( std::exception& ex )
                {
                    cerr << "(write): " << ex.what() << endl;
                }
                catch( ... )
                {
                    cerr << "(write): catch ..." << endl;
                }
            }

            break;

            default:
                cerr << endl << "Unknown command: '" << cmd << "'. Use -h for help" << endl;
                return -1;
                break;
        }
    }
    catch( const std::exception& e )
    {
        cerr << "(main): " << e.what() << endl;
    }
    catch( ... )
    {
        cerr << "(main): catch ..." << endl;
        return 1;
    }

    return 0;
}
// --------------------------------------------------------------------------
