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
void TestProc::sysCommand( UniSetTypes::SystemMessage* sm )
{
	TestProc_SK::sysCommand(sm);
    if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
        askTimer(tmCheckDepend,checkDependTime);
}
// -----------------------------------------------------------------------------
void TestProc::sensorInfo( SensorMessage *sm )
{
	dlog[Debug::INFO] << myname << "(sensorInfo): id=" << sm->id << " val=" << sm->value << endl;

	if( sm->id == on_s )
	{
		if( sm->value )
		{
			dlog[Debug::LEVEL1] << myname << "(sensorInfo): START WORKING.." << endl;
			askTimer(tmChange,changeTime);
		}
	    else
	    {
			askTimer(tmChange,0);
			dlog[Debug::LEVEL1] << myname << "(sensorInfo): STOP WORKING.." << endl;
		}
	}
}
// -----------------------------------------------------------------------------
void TestProc::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmChange )
	{
		state^=true;
		out_lamp_c = ( state ? lmpBLINK : lmpOFF );
		dlog[Debug::LEVEL1] << myname << "(timerInfo): state=" << state << " lmp=" << out_lamp_c << endl;
        askTimer(tmCheckWorking,checkTime); // reset timer
	}
	else if( tm->id == tmCheckWorking )
       dlog[Debug::LEVEL1] << myname << "(timerInfo): WORKING FAIL!" << endl;
	else if( tm->id == tmCheckDepend )
	{
       dlog[Debug::LEVEL1] << myname << "(timerInfo): Check depend..." << endl;

	   long test_val = 100;

       // set depend 0...
       setValue(depend_c,0);
       setValue(set_d_check_s,test_val);
       dlog[Debug::LEVEL1] << myname << "(timerInfo): check depend OFF: " << ( getValue(d_check_s) == 0 ? "OK" : "FAIL" ) << endl;

		// set depend 1
       setValue(depend_c,1);
       dlog[Debug::LEVEL1] << myname << "(timerInfo): check depend ON: " << ( getValue(d_check_s) == test_val ? "OK" : "FAIL" ) << endl;
    }
}
// -----------------------------------------------------------------------------
