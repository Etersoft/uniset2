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
    { "read-pause", required_argument, 0, 'p' },
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
    std::string varname = "";
    int msecpause = 200;
    timeout_t tout = UniSetTimer::WaitUpTime;
    size_t ncycles = 0;
    std::vector<UA_ReadValueId> rvalues;
    std::vector<UA_WriteValue> wvalues;
    std::vector<std::string> attrs;

    while(1)
    {
        opt = getopt_long(argc, argv, "hx:r:w:t:vz:i:p:", longopts, &optindex);

        if( opt == -1 )
            break;

        switch (opt)
        {
            case 'h':
                cout << "-h|--help                - this message" << endl;
                cout << "[-x|--host] host:port    - OPC UA server address. Default: localhost:4840" << endl;
                cout << "[-r|--read] varname      - Read variable" << endl;
                cout << "[-w|--write] varname=val - Write variable" << endl;
                cout << "[-t|--timeout] msec      - timeout for receive. Default: 0 msec (waitup)." << endl;
                cout << "[-v|--verbose]           - verbose mode." << endl;
                cout << "[-z|--num-cycles] num    - Number of cycles of exchange. Default: -1 - infinitely." << endl;
                cout << "[-p|--read-pause] msec   - Pause between read" << endl;
                cout << endl;
                return 0;

            case 'x':
                addr = string(optarg);
                break;

            case 'r':
                cmd = cmdRead;
                rvalues.emplace_back(OPCUAClient::makeReadValue32(string(optarg)));
                attrs.push_back(string(optarg));
                break;

            case 'w':
            {
                cmd = cmdWrite;
                auto arg = string(optarg);
                auto val = uniset::explode_str(arg, '=');

                if (val.size() < 2)
                {
                    cerr << "write variable format msut be 'varname=value'" << endl;
                    return 1;
                }

                wvalues.emplace_back(OPCUAClient::makeWriteValue32(val[0], uni_atoi(val[1])) );
                attrs.push_back(val[0]);
            }
            break;

            case 't':
                tout = atoi(optarg);
                break;

            case 'p':
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

        if( !client->connect(addr))
        {
            cerr << "Can't connect to " << addr << endl;
            return 1;
        }

        switch( cmd )
        {
            case cmdRead:
            {
                int nc = 1;

                if( ncycles > 0 )
                    nc = ncycles;

                auto t_start = steady_clock::now();

                std::vector<OPCUAClient::Result32> result;

                for( auto&& v : rvalues )
                    result.push_back(OPCUAClient::Result32{});

                while( nc )
                {
                    try
                    {
                        auto ret = client->read32(rvalues, result);

                        if( ret == 0 )
                        {
                            for( size_t i = 0; i < result.size(); i++ )
                                cout << attrs[i] << ": value=" << result[i].value << " status: " << UA_StatusCode_name(result[i].status) << endl;
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
                    cout << "write:";

                if( verb )
                    cout << endl;

                try
                {
                    auto ret = client->write32(wvalues);

                    if( ret != 0 )
                    {
                        cerr << "write error code " << ret << endl;

                        for(size_t i = 0; i < wvalues.size(); i++ )
                            cerr << attrs[i] << ": status=" << UA_StatusCode_name(wvalues[i].value.status) << endl;
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
