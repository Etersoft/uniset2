// --------------------------------------------------------------------------
#include <string>
#include <vector>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "PassiveTimer.h"
#include "Exceptions.h"
#include "LogReader.h"
#include "LogServerTypes.h"
#include "LogAgregator.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "verbose", no_argument, 0, 'v' },
    { "filter", required_argument, 0, 'f' },
    { "iaddr", required_argument, 0, 'i' },
    { "port", required_argument, 0, 'p' },
    { "add", required_argument, 0, 'a' },
    { "del", required_argument, 0, 'd' },
    { "set", required_argument, 0, 's' },
    { "off", required_argument, 0, 'o' },
    { "on", required_argument, 0, 'e' },
    { "save-loglevels", required_argument, 0, 'u' },
    { "restore-loglevels", required_argument, 0, 'y' },
    { "view-default-loglevels", required_argument, 0, 'b' },
    { "list", optional_argument, 0, 'l' },
    { "rotate", optional_argument, 0, 'r' },
    { "logfilter", required_argument, 0, 'n' },
    { "command-only", no_argument, 0, 'c' },
    { "timeout", required_argument, 0, 't' },
    { "reconnect-delay", required_argument, 0, 'x' },
    { "logfile", required_argument, 0, 'w' },
    { "logfile-truncate", required_argument, 0, 'z' },
    { "grep", required_argument, 0, 'g' },
    { "timezone", required_argument, 0, 'm' },
    { "set-verbosity", required_argument, 0, 'q' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("Configs:\n");
    printf("-h, --help                  - this message\n");
    printf("-v, --verbose               - Print all messages to stdout\n");
    printf("[-i|--iaddr] addr           - LogServer ip or hostname.\n");
    printf("[-p|--port] port            - LogServer port.\n");
    printf("[-c|--command-only]         - Send command and break. (No read logs).\n");
    printf("[-t|--timeout] msec         - Timeout for wait data. Default: WaitUpTime - endless waiting\n");
    printf("[-x|--reconnect-delay] msec - Pause for repeat connect to LogServer. Default: 5000 msec.\n");
    printf("[-w|--logfile] logfile      - Save log to 'logfile'.\n");
    printf("[-z|--logfile-truncate]     - Truncate log file before write. Use with -w|--logfile \n");

    printf("\n");
    printf("Commands:\n");

    printf("[-l | --list] [objName]                   - Show logs hierarchy from logname. Default: ALL\n");
    printf("[-b | --view-default-loglevels] [objName] - Show current log levels for logname.\n");

    printf("[-a | --add] info,warn,crit,... [objName] - Add log levels.\n");
    printf("[-d | --del] info,warn,crit,... [objName] - Delete log levels.\n");
    printf("[-s | --set] info,warn,crit,... [objName] - Set log levels.\n");
    printf("[-q | --set-verbosity] level [objName]    - Set verbose level.\n");

    printf("[-f | --filter] logname                   - ('filter mode'). View log only from 'logname'(regexp)\n");
    printf("[-g | --grep pattern                      - Print lines matching a pattern (c++ regexp)\n");

    printf("\n");
    printf("Note: 'objName' - regexp for name of log. Default: ALL logs.\n");
    printf("\n");
    printf("Special commands:\n");
    printf("[-o | --off] [objName]                    - Off the write log file (if enabled).\n");
    printf("[-e | --on] [objName]                     - On(enable) the write log file (if before disabled).\n");
    printf("[-r | --rotate] [objName]                 - rotate log file.\n");
    printf("[-u | --save-loglevels] [objName]         - save log levels (disable restore after disconnected).\n");
    printf("[-y | --restore-loglevels] [objName]      - restore default log levels.\n");
    printf("[-m | --timezone] [local|utc]             - set time zone for log, local or UTC.\n");

    printf("\n");
    printf("Examples:\n");
    printf("=========\n");
    printf("log hierarchy:\n");
    printf("SESControl1/TV1\n");
    printf("SESControl1/TV1/HeatExchanger\n");
    printf("SESControl1/TV1/MyCustomLog\n");
    printf("\n");
    printf("* Show all logs for SESControl1 (only for SESControl1 and it's childrens)\n");
    printf("uniset2-log -i host -p 30202 --del any --set any SESControl1\n");
    printf("* Show all logs for TV1\n");
    printf("uniset2-log -i host -p 30201 --del any --set any TV1.*\n");
    printf("* Show all logs for MyCustomLog\n");
    printf("uniset2-log -i host -p 30201 --del any --set any MyCustomLog\n");
    printf("* Show info logs with special text for TV1\n");
    printf("uniset2-log -i host -p 30202 --del any --set info TV1 --grep [Tt]ransient\n");
}
// --------------------------------------------------------------------------
static char* checkArg( int i, int argc, char* argv[] );
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
int main( int argc, char** argv )
{
    //  std::ios::sync_with_stdio(false); // нельзя отключать.. тогда "обмен с сервером" рассинхронизируется

    int optindex = 0;
    int opt = 0;
    int verb = 0;
    string addr("localhost");
    int port = 3333;
    DebugStream dlog;
    vector<LogReader::Command> vcmd;
    string logfilter("");
    LogServerTypes::Command cmd = LogServerTypes::cmdNOP;
    int cmdonly = 0;
    timeout_t tout = UniSetTimer::WaitUpTime;
    timeout_t rdelay = 8000;
    string logfile("");
    bool logtruncate = false;
    std::string textfilter("");

    try
    {
        while(1)
        {
            opt = getopt_long(argc, argv, "chvlf:a:p:i:d:s:n:eorbx:w:zt:g:uby:m:q:", longopts, &optindex);

            if( opt == -1 )
                break;

            switch (opt)
            {
                case 'h':
                    print_help();
                    return 0;

                case 'a':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdAddLevel;
                    std::string filter("");
                    std::string d = string(optarg);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, (int)Debug::value(d), filter);
                }
                break;

                case 'd':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdDelLevel;
                    std::string filter("");
                    std::string d = string(optarg);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, (int)Debug::value(d), filter );
                }
                break;

                case 's':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdSetLevel;
                    std::string filter("");
                    std::string d = string(optarg);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, (int)Debug::value(d), filter );
                }
                break;

                case 'q':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdSetVerbosity;
                    std::string filter("");
                    int d = uni_atoi(optarg);
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, d, filter);
                }
                break;

                case 'l':
                {
                    cmdonly = 1;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(LogServerTypes::cmdList, 0, filter);
                }
                break;

                case 'o':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdOffLogFile;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, 0, filter);
                }
                break;

                case 'u':  // --save-loglevels
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdSaveLogLevel;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, 0, filter);
                }
                break;

                case 'y':  // --restore-loglevels
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdRestoreLogLevel;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, 0, filter);
                }
                break;

                case 'b':  // --view-default-loglevels
                {
                    cmdonly = 1;
                    LogServerTypes::Command cmd = LogServerTypes::cmdViewDefaultLogLevel;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, 0, filter);
                }
                break;

                case 'e':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdOnLogFile;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, 0, filter);
                }
                break;

                case 'f':
                {
                    cmd = LogServerTypes::cmdFilterMode;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    logfilter = filter;
                }
                break;

                case 'r':
                {
                    LogServerTypes::Command cmd = LogServerTypes::cmdRotate;
                    std::string filter("");
                    char* arg2 = checkArg(optind, argc, argv);

                    if( arg2 )
                        filter = string(arg2);

                    vcmd.emplace_back(cmd, 0, filter);
                }
                break;

                case 'm':
                {
                    LogServerTypes::Command cmd;
                    std::string tz(optarg);

                    if( tz == "local" )
                        cmd = LogServerTypes::cmdShowLocalTime;
                    else if( tz == "utc" )
                        cmd = LogServerTypes::cmdShowUTCTime;
                    else
                    {
                        cerr << "Error: Unknown timezone '" << tz << "'. Must be 'local' or 'utc'" << endl;
                        return 1;
                    }

                    vcmd.emplace_back(cmd, 0, "");
                }
                break;

                case 'i':
                    addr = string(optarg);
                    break;

                case 'c':
                    cmdonly = 1;
                    break;

                case 'p':
                    port = uni_atoi(optarg);
                    break;

                case 'x':
                    rdelay = uni_atoi(optarg);
                    break;

                case 't':
                    tout = uni_atoi(optarg);
                    break;

                case 'w':
                    logfile = string(optarg);
                    break;

                case 'g':
                    textfilter = string(optarg);
                    break;

                case 'z':
                    logtruncate = true;
                    break;

                case 'v':
                    verb = 1;
                    break;

                case '?':
                default:
                    printf("Unknown argumnet\n");
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
        lr.setTextFilter(textfilter);

        if( !logfile.empty() )
            lr.log()->logFile(logfile, logtruncate);

        if( !vcmd.empty() )
            lr.sendCommand(addr, port, vcmd, cmdonly, verb);

        if( !cmdonly )
            lr.readlogs( addr, port, cmd, logfilter, verb );
    }
    catch( const SystemError& err )
    {
        cerr << "(log): " << err << endl;
    }
    catch( const uniset::Exception& ex )
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
char* checkArg( int i, int argc, char* argv[] )
{
    if( i < argc && (argv[i])[0] != '-' )
        return argv[i];

    return 0;
}
// --------------------------------------------------------------------------
