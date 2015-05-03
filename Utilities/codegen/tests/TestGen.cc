#include "Exceptions.h"
#include "TestGen.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TestGen::TestGen( UniSetTypes::ObjectId id, xmlNode* confnode ):
	TestGen_SK( id, confnode )
{
}
// -----------------------------------------------------------------------------
TestGen::~TestGen()
{
}
// -----------------------------------------------------------------------------
TestGen::TestGen()
{
	cerr << ": init failed!!!!!!!!!!!!!!!" << endl;
	throw Exception();
}
// -----------------------------------------------------------------------------
void TestGen::step()
{
	cout << "strval: " << strval(input2_s) << endl;
	cout << "str: " << str(input2_s) << endl;
	cout << "===========" << endl;
	cout << dumpIO() << endl;
}
// -----------------------------------------------------------------------------
void TestGen::sensorInfo( const SensorMessage* sm )
{
	if( sm->id == input1_s )
		out_output1_c = in_input1_s; // sm->state
}
// -----------------------------------------------------------------------------
void TestGen::timerInfo( const TimerMessage* tm )
{
}
// -----------------------------------------------------------------------------
void TestGen::sigterm( int signo )
{
	TestGen_SK::sigterm(signo);
}
// -----------------------------------------------------------------------------
