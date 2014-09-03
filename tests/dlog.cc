#include <time.h>
#include "Debug.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

void check_log_signal( const string& s )
{
//	cout << "log signal: ||| " << s  << "|||";
	cout << "*ENDL*[" << s << flush;
}

int main( int argc, const char **argv )
{
    DebugStream tlog;
    
	tlog.signal_stream_event().connect(&check_log_signal);    
    
    
    tlog.addLevel(Debug::ANY);
    
    tlog[Debug::INFO] << ": [info] ..." << endl;
    tlog(Debug::INFO) << ": (info) ..." << endl;
    cout << endl;
    tlog.level1() << ": [level1] ..." << endl;
    tlog.info() << ": [info] ..." << endl;

    if( tlog.is_level1() )
        tlog.level1() << ": is level1..." << endl;

    return 0;
}
