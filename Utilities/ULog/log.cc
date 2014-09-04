// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogReader.h"
#include "LogServerTypes.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] = {
    { "help", no_argument, 0, 'h' },
    { "verbose", no_argument, 0, 'v' },
    { "iaddr", required_argument, 0, 'a' },
    { "port", required_argument, 0, 'p' },
    { "add", required_argument, 0, 'l' },
    { "del", required_argument, 0, 'd' },
    { "set", required_argument, 0, 's' },
    { "off", required_argument, 0, 'o' },
    { "on", required_argument, 0, 'n' },
    { "rotate", required_argument, 0, 'r' },
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
    int cmd = LogServerTypes::cmdNOP;
    int data = 0;
    string sdata("");

    try
    {
        while( (opt = getopt_long(argc, argv, "hva:p:l:d:s:onr",longopts,&optindex)) != -1 )
        {
            switch (opt)
            {
                case 'h':
                    print_help();
                return 0;

                case 'l':
                {
                    cmd = LogServerTypes::cmdAddLevel;
                    sdata = string(optarg);
                }
                break;
                case 'd':
                {
                    cmd = LogServerTypes::cmdDelLevel;
                    sdata = string(optarg);
                }
                break;
                case 's':
                {
                    cmd = LogServerTypes::cmdSetLevel;
                    sdata = string(optarg);
                }
                break;
                case 'o':
                    cmd = LogServerTypes::cmdOffLogFile;
                break;
                case 'n':
                    cmd = LogServerTypes::cmdOnLogFile;
                break;
                case 'r':
                    cmd = LogServerTypes::cmdRotate;
                break;

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

        if( !sdata.empty() )
        {
            data = (int)Debug::value(sdata);
            
            if( verb )
            	cout << "SEND COMMAND: '" << (LogServerTypes::Command)cmd << " data='" << sdata << "'" << endl;
        }

        lr.readlogs( addr, port, (LogServerTypes::Command)cmd, data, verb );
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
