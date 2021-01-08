#include <iomanip>
#include "Exceptions.h"
#include "TestProc.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TestProc::TestProc( uniset::ObjectId id, xmlNode* confnode ):
    TestProc_SK( id, confnode )
{
}
// -----------------------------------------------------------------------------
TestProc::~TestProc()
{
}
// -----------------------------------------------------------------------------
bool TestProc::isFullQueue()
{
    return ( getCountOfLostMessages() > 0 );
}
// -----------------------------------------------------------------------------
TestProc::TestProc()
{
    cerr << ": init failed!!!!!!!!!!!!!!!" << endl;
    throw Exception();
}
// -----------------------------------------------------------------------------
