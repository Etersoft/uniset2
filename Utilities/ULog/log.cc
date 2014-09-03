// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogReader.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] = {
    { "help", no_argument, 0, 'h' },
    { "iaddr", required_argument, 0, 'a' },
    { "port", required_argument, 0, 'p' },
    { "verbose", no_argument, 0, 'v' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("-h|--help           - this message\n");
    printf("[-v|--verbose]      - Print all messages to stdout\n");
    printf("[-a|--iaddr] addr   - Inet address for listen connections.\n");
    printf("[-p|--port] port    - Bind port.\n");
}
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
    int optindex = 0;
    int opt = 0;
    int verb = 0;
    string addr("localhost");
    int port = 3333;
    DebugStream dlog;

    try
    {
        while( (opt = getopt_long(argc, argv, "hva:p:",longopts,&optindex)) != -1 )
        {
            switch (opt)
            {
                case 'h':
                    print_help();
                return 0;

                case 'a':
                    addr = string(optarg);
                break;

                case 'p':
                    port = uni_atoi(optarg);
                break;

                case 'v':
                    verb = 1;
                break;

                case '?':
                default:
                    printf("? argumnet\n");
                    return 0;
            }
        }

        if( verb )
        {
            cout << "(init): read from " << addr << ":" << port << endl;

            dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
        }

        LogReader lr;
        lr.readlogs( addr, port, verb );
    }
    catch( SystemError& err )
    {
        cerr << "(log): " << err << endl;
    }
    catch( Exception& ex )
    {
        cerr << "(log): " << ex << endl;
    }
    catch(...)
    {
        cerr << "(log): catch(...)" << endl;
    }

    return 0;
}
// --------------------------------------------------------------------------
