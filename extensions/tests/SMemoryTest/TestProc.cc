#include "Exceptions.h"
#include "TestProc.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TestProc::TestProc( UniSetTypes::ObjectId id, xmlNode* confnode ):
	TestProc_SK( id, confnode ),
	state(false)
{
}
// -----------------------------------------------------------------------------
TestProc::~TestProc()
{
}
// -----------------------------------------------------------------------------
TestProc::TestProc():
	state(false)
{
	cerr << ": init failed!!!!!!!!!!!!!!!"<< endl;
	throw Exception();
}
// -----------------------------------------------------------------------------
void TestProc::step()
{
}
// -----------------------------------------------------------------------------
void TestProc::sensorInfo( SensorMessage *sm )
{
	if( sm->id == on_s )
	{
		if( sm->value )
			askTimer(tmChange,changeTime);
	}
}
// -----------------------------------------------------------------------------
void TestProc::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmChange )
	{
		state^=true;
		out_lamp_c = ( state ? lmpBLINK : lmpOFF );
		dlog[Debug::INFO] << myname << "(timerInfo): state=" << state << " lmp=" << out_lamp_c << endl;
	}

}
// -----------------------------------------------------------------------------
