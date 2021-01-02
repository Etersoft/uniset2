// --------------------------------------------------------------------------
#include "Debug.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main( int argc, char** argv )
{
    if( argc < 2 || (argc > 1 && ( !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) ) // -V560
    {
        cout << "Usage: lo2gval [ info,warn,crit,level1...level9,init,repository,system,exception | any ]" << endl;
        return 0;
    }

    const string s(argv[1]);

    cout << (int)Debug::value(s) << endl;

    return 0;
}
// --------------------------------------------------------------------------
