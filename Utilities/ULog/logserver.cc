// --------------------------------------------------------------------------
#include <string>
#include <getopt.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogServer.h"
#include "LogAgregator.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static struct option longopts[] = {
    { "help", no_argument, 0, 'h' },
    { "iaddr", required_argument, 0, 'i' },
    { "port", required_argument, 0, 'p' },
    { "verbose", no_argument, 0, 'v' },
    { "delay", required_argument, 0, 'd' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
static void print_help()
{
    printf("-h|--help           - this message\n");
//    printf("[-t|--timeout] msec  - Timeout. Default: 2000.\n");
    printf("[-v|--verbose]      - Print all messages to stdout\n");
    printf("[-i|--iaddr] addr   - Inet address for listen connections.\n");
    printf("[-p|--port] port    - Bind port.\n");
    printf("[-d|--delay] msec   - Delay for generate message. Default 5000.\n");
}
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
    int optindex = 0;
    int opt = 0;
    int verb = 0;
    string addr("localhost");
    int port = 3333;
    int tout = 2000;
    timeout_t delay = 5000;

    try
    {
        while( (opt = getopt_long(argc, argv, "hvi:p:d:",longopts,&optindex)) != -1 )
        {
            switch (opt)
            {
                case 'h':
                    print_help();
                return 0;

                case 'i':
                    addr = string(optarg);
                break;

                case 'p':
                    port = uni_atoi(optarg);
                break;

                case 'd':
                    delay = uni_atoi(optarg);
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
            cout << "(init): listen " << addr << ":" << port
//                   << " timeout=" << tout << " msec "
                    << endl;

//            dlog.addLevel( Debug::type(Debug::CRIT | Debug::WARN | Debug::INFO) );
        }


	    DebugStream dlog;
	    dlog.setLogName("dlog");
        DebugStream dlog2;
	    dlog2.setLogName("dlog2");

		LogAgregator la;
		la.add(dlog);
		la.add(dlog2);

        LogServer ls(la);
//		LogServer ls(cout);
        dlog.addLevel(Debug::ANY);
        dlog2.addLevel(Debug::ANY);
        
        ls.run( addr, port, true );
        
        unsigned int i=0;
        while( true )
        {
        	dlog << "[" << ++i << "] Test message for log" << endl;
        	dlog.info() << ": dlog : INFO message" << endl;
        	dlog.warn() << ": dlog : WARN message" << endl;
        	dlog.crit() << ": dlog : CRIT message" << endl;

        	dlog2.info() << ": dlog2: INFO message" << endl;
        	dlog2.warn() << ": dlog2: WARN message" << endl;
        	dlog2.crit() << ": dlog2: CRIT message" << endl;
        	
        	msleep(delay);
        }
    }
    catch( SystemError& err )
    {
        cerr << "(logserver): " << err << endl;
    }
    catch( Exception& ex )
    {
        cerr << "(logserver): " << ex << endl;
    }
    catch(...)
    {
        cerr << "(logserver): catch(...)" << endl;
    }

    return 0;
}
// --------------------------------------------------------------------------
