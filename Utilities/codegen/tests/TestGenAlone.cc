#include "Exceptions.h"
#include "TestGenAlone.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TestGenAlone::TestGenAlone( uniset::ObjectId id, xmlNode* confnode ):
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
void TestGenAlone::sensorInfo( const SensorMessage* sm )
{
	if( sm->id == input1_s )
		out_output1_c = in_input1_s; // sm->state
}
// -----------------------------------------------------------------------------
void TestGenAlone::timerInfo( const TimerMessage* tm )
{
}
// -----------------------------------------------------------------------------
