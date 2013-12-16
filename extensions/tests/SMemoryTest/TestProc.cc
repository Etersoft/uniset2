#include <iomanip>
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
    {
        askTimer(tmCheckDepend,checkDependTime);
        askTimer(tmCheckUndefState,checkUndefTime);
    }
}
// -----------------------------------------------------------------------------
void TestProc::sensorInfo( SensorMessage *sm )
{
	dlog[Debug::LEVEL1] << myname << "(sensorInfo): id=" << sm->id << " val=" << sm->value
			<< "  " << UniversalInterface::timeToString(sm->sm_tv_sec,":")
			<< "(" << setw(6) << sm->sm_tv_usec << "): "
			<< endl;

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
	else if( sm->id == check_undef_s )
	{
		dlog[Debug::LEVEL1] << myname << "(sensorInfo): CHECK UNDEFINED STATE ==> " << (sm->undefined==undef ? "OK" : "FAIL") << endl;
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
       setValue(set_d1_check_s,test_val);
       setValue(set_d2_check_s,test_val);
       dlog[Debug::LEVEL1] << myname << "(timerInfo): check depend OFF: d1: " << ( getValue(d1_check_s) == 0 ? "OK" : "FAIL" ) << endl;
       dlog[Debug::LEVEL1] << myname << "(timerInfo): check depend OFF: d2: " << ( getValue(d2_check_s) == -50 ? "OK" : "FAIL" ) << endl;

		// set depend 1
       setValue(depend_c,1);
       dlog[Debug::LEVEL1] << myname << "(timerInfo): check depend ON: d1: " << ( getValue(d1_check_s) == test_val ? "OK" : "FAIL" ) << endl;
       dlog[Debug::LEVEL1] << myname << "(timerInfo): check depend ON: d2: " << ( getValue(d2_check_s) == test_val ? "OK" : "FAIL" ) << endl;
    }
    else if( tm->id == tmCheckUndefState )
    {
       dlog[Debug::LEVEL1] << myname << "(timerInfo): Check undef state..." << endl;
	   undef ^= true;

	   si.id = undef_c;
	   si.node = conf->getLocalNode();
	   dlog[Debug::LEVEL1] << myname << "(timerInfo): set undefined=" << undef << endl;
	   ui.setUndefinedState( si, undef, getId() );
	}
}
// -----------------------------------------------------------------------------
