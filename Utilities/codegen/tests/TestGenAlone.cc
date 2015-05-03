#include "Exceptions.h"
#include "TestGenAlone.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TestGenAlone::TestGenAlone( UniSetTypes::ObjectId id, xmlNode* confnode ):
	TestGenAlone_SK( id, confnode )
{
}
// -----------------------------------------------------------------------------
TestGenAlone::~TestGenAlone()
{
}
// -----------------------------------------------------------------------------
void TestGenAlone::step()
{
	cout << strval(in_input2_s) << endl;
}
// -----------------------------------------------------------------------------
void TestGenAlone::sensorInfo( SensorMessage* sm )
{
	if( sm->id == input1_s )
		out_output1_c = in_input1_s; // sm->state
}
// -----------------------------------------------------------------------------
void TestGenAlone::timerInfo( TimerMessage* tm )
{
}
// -----------------------------------------------------------------------------
void TestGenAlone::sigterm( int signo )
{
	TestGenAlone_SK::sigterm(signo);
}
// -----------------------------------------------------------------------------
