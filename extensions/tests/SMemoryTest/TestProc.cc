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
        askTimer(tmCheck,checkTime);
        askTimer(tmCheckWorking,checkWorkingTime);

        // В начальный момент времени блокирующий датчик =0, поэтому d2_check_s должен быть равен depend_off_value (-50).
       dlog.level1() << myname << "(startup): check init depend: " << ( getValue(d2_check_s) == -50 ? "OK" : "FAIL" ) << endl;
    }
}
// -----------------------------------------------------------------------------
void TestProc::sensorInfo( SensorMessage *sm )
{
	dlog.level2() << myname << "(sensorInfo): id=" << sm->id << " val=" << sm->value
			<< "  " << timeToString(sm->sm_tv_sec,":")
			<< "(" << setw(6) << sm->sm_tv_usec << "): "
			<< endl;

	if( sm->id == on_s )
	{
		if( sm->value )
		{
			dlog.level1() << myname << "(sensorInfo): START WORKING.." << endl;
			askTimer(tmChange,changeTime);
		}
	    else
	    {
			askTimer(tmChange,0);
			dlog.level1() << myname << "(sensorInfo): STOP WORKING.." << endl;
		}
	}
	else if( sm->id == check_undef_s )
	{
		dlog.level1() << myname << "(sensorInfo): CHECK UNDEFINED STATE ==> " << (sm->undefined==undef ? "OK" : "FAIL") << endl;
	}
}
// -----------------------------------------------------------------------------
void TestProc::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmChange )
	{
		state^=true;
		out_lamp_c = ( state ? lmpBLINK : lmpOFF );
		dlog.level2() << myname << ": state=" << state << " lmp=" << out_lamp_c << endl;
        askTimer(tmCheckWorking,checkTime); // reset timer
	}
	else if( tm->id == tmCheckWorking )
       dlog.level1() << myname << ": WORKING FAIL!" << endl;
	else if( tm->id == tmCheck )
	{
		dlog.level1() << endl << endl << "--------" << endl;
        test_depend();
        test_undefined_state();
        test_thresholds();
	}
}
// -----------------------------------------------------------------------------
void TestProc::test_depend()
{
    dlog.level1() << myname << ": Check depend..." << endl;

    long test_val = 100;

    // set depend 0...
    setValue(depend_c,0);
    setValue(set_d1_check_s,test_val);
    setValue(set_d2_check_s,test_val);
    dlog.level1() << myname << ": check depend OFF: d1: " << ( getValue(d1_check_s) == 0 ? "OK" : "FAIL" ) << endl;
    dlog.level1() << myname << ": check depend OFF: d2: " << ( getValue(d2_check_s) == -50 ? "OK" : "FAIL" ) << endl;

    // set depend 1
    setValue(depend_c,1);
    dlog.level1() << myname << ": check depend ON: d1: " << ( getValue(d1_check_s) == test_val ? "OK" : "FAIL" ) << endl;
    dlog.level1() << myname << ": check depend ON: d2: " << ( getValue(d2_check_s) == test_val ? "OK" : "FAIL" ) << endl;
}
// -----------------------------------------------------------------------------
void TestProc::test_undefined_state()
{
    // ---------------- Проверка выставления неопределённого состояния ---------------------
    dlog.level1() << myname << ": Check undef state..." << endl;
    undef ^= true;

    si.id = undef_c;
    si.node = conf->getLocalNode();
    dlog.level1() << myname << ": set undefined=" << undef << endl;
    ui.setUndefinedState( si, undef, getId() );
}
// -----------------------------------------------------------------------------
void TestProc::test_thresholds()
{
    // ---------------- Проверка работы порогов ---------------------
    dlog.level1() << myname << ": Check thresholds..." << endl;

    setValue(t_set_c,0);
    dlog.level1() << myname << ": check threshold OFF value: " << ( getValue(t_check_s) == 0 ? "OK" : "FAIL" ) << endl;

    setValue(t_set_c,378);
    dlog.level1() << myname << ": check threshold ON value: " << ( getValue(t_check_s) == 1 ? "OK" : "FAIL" ) << endl;

	dlog.level1() << myname << ": ask threshold and check.. " << endl;

	try
	{
		setValue(t_set_c, 0);
		UniSetTypes::ThresholdId tid = 100;
		ui.askThreshold( t_set_c, tid, UniversalIO::UIONotify, 10, 20 );

		IONotifyController_i::ThresholdInfo ti = ui.getThresholdInfo(t_set_c,tid);
		dlog.level1() << myname << ": ask OFF threshold: " << ( ti.state == IONotifyController_i::NormalThreshold  ? "OK" : "FAIL" ) << endl;
		setValue(t_set_c, 25);
		ti = ui.getThresholdInfo(t_set_c,tid);
		dlog.level1() << myname << ": ask ON threshold: " << ( ti.state == IONotifyController_i::HiThreshold  ? "OK" : "FAIL" ) << endl;
	}
	catch( Exception& ex )
	{
		dlog.level2() << myname << ": CHE 'ask and get threshold' FAILED: " << ex << endl;
	}
}
// -----------------------------------------------------------------------------
