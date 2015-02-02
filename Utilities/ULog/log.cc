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
    { "iaddr", required_argument, 0, 'i' },
    { "port", required_argument, 0, 'p' },
    { "add", required_argument, 0, 'a' },
    { "del", required_argument, 0, 'd' },
    { "set", required_argument, 0, 's' },
    { "off", required_argument, 0, 'o' },
    { "on", required_argument, 0, 'n' },
    { "rotate", required_argument, 0, 'r' },
    { "logname", required_argument, 0, 'l' },
    { "command-only", no_argument, 0, 'b' },
    { "timeout", required_argument, 0, 'w' },
    { "reconnect-delay", required_argument, 0, 'x' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("-h, --help                  - this message\n");
    printf("-v, --verbose               - Print all messages to stdout\n");
    printf("[-i|--iaddr] addr           - LogServer ip or hostname.\n");
    printf("[-p|--port] port            - LogServer port.\n");
    printf("[-l|--logname] name         - Send command only for 'logname'.\n");
    printf("[-b|--command-only]         - Send command and break. (No read logs).\n");
    printf("[-w|--timeout] msec         - Timeout for wait data. Default: 0 - endless waiting\n");
    printf("[-x|--reconnect-delay] msec - Pause for repeat connect to LogServer. Default: 5000 msec.\n");

    printf("\n");
    printf("Commands:\n");

    printf("[--add | -a] info,warn,crit,...  - Add log levels.\n");
    printf("[--del | -d] info,warn,crit,...  - Delete log levels.\n");
    printf("[--set | -s] info,warn,crit,...  - Set log levels.\n");
    printf("--off, -o                        - Off the write log file (if enabled).\n");
    printf("--on, -n                         - On the write log file (if before disabled).\n");
    printf("--rotate, -r                     - rotate log file.\n");
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
    int cmdonly = 0;
    string logname("");
    timeout_t tout = 0;
    timeout_t rdelay = 5000;

    try
    {
        while( (opt = getopt_long(argc, argv, "hva:p:i:d:s:l:onrbx:w:",longopts,&optindex)) != -1 )
        {
            switch (opt)
            {
                case 'h':
                    print_help();
                return 0;

                case 'a':
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

                case 'i':
                    addr = string(optarg);
                break;

                case 'l':
                    logname = string(optarg);
                break;

                case 'b':
                    cmdonly = 1;
                break;

                case 'p':
                    port = uni_atoi(optarg);
                break;

                case 'x':
                    rdelay = uni_atoi(optarg);
                break;

                case 'w':
                    tout = uni_atoi(optarg);
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
        lr.setCommandOnlyMode(cmdonly);
        lr.setinTimeout(tout);
        lr.setReconnectDelay(rdelay);

        if( !sdata.empty() )
        {
            data = (int)Debug::value(sdata);

            if( verb )
                cout << "SEND COMMAND: '" << (LogServerTypes::Command)cmd << " data='" << sdata << "'" << endl;
        }

        lr.readlogs( addr, port, (LogServerTypes::Command)cmd, data, logname, verb );
    }
    catch( const SystemError& err )
    {
        cerr << "(log): " << err << endl;
    }
    catch( const Exception& ex )
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
