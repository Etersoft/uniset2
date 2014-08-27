#include <time.h>
#include "Debug.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

int main( int argc, const char **argv )
{
    DebugStream tlog;
    
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
