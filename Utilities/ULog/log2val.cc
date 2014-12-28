// --------------------------------------------------------------------------
#include "Debug.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{
	if( argc < 2 || (argc > 1 && ( !strcmp(argv[1],"--help") || !strcmp(argv[1],"-h"))) )
	{
	    cout << "Usage: lo2gval info,warn,crit,.... " << endl;
        return 0;
    }

	const string s(argv[1]);

	cout << (int)Debug::value(s) << endl;

    return 0;
}
// --------------------------------------------------------------------------
