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
    loglevels.push_back(Debug::INFO);
    loglevels.push_back(Debug::WARN);
    loglevels.push_back(Debug::CRIT);
    loglevels.push_back(Debug::LEVEL1);
    loglevels.push_back(Debug::LEVEL2);
    loglevels.push_back( Debug::type(Debug::LEVEL2 | Debug::INFO) );
    loglevels.push_back( Debug::type(Debug::LEVEL2 | Debug::INFO | Debug::WARN | Debug::CRIT) );

    lit = loglevels.begin();
    out_log_c = (*lit);
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
        askTimer(tmLogControl,checkLogTime);

        // В начальный момент времени блокирующий датчик =0, поэтому d2_check_s должен быть равен depend_off_value (-50).
       cerr << myname << "(startup): check init depend: " << ( getValue(d2_check_s) == -50 ? "OK" : "FAIL" ) << endl;
    }
}
// -----------------------------------------------------------------------------
void TestProc::sensorInfo( SensorMessage *sm )
{
/*
    dlog.level2() << myname << "(sensorInfo): id=" << sm->id << " val=" << sm->value
            << "  " << timeToString(sm->sm_tv_sec,":")
            << "(" << setw(6) << sm->sm_tv_usec << "): "
            << endl;
*/
    if( sm->id == on_s )
    {
        if( sm->value )
        {
            cerr << myname << "(sensorInfo): START WORKING.." << endl;
            askTimer(tmChange,changeTime);
        }
        else
        {
            askTimer(tmChange,0);
            cerr << myname << "(sensorInfo): STOP WORKING.." << endl;
        }
    }
    else if( sm->id == check_undef_s )
    {
        cerr << myname << "(sensorInfo): CHECK UNDEFINED STATE ==> " << (sm->undefined==undef ? "OK" : "FAIL") << endl;
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
       cerr << myname << ": WORKING FAIL!" << endl;
    else if( tm->id == tmCheck )
    {
        cerr << endl << endl << "--------" << endl;
        test_depend();
        test_undefined_state();
        test_thresholds();
        test_loglevel();
    }
    else if( tm->id == tmLogControl )
    {
        cerr << endl;
        cerr << "======= TEST LOG PRINT ======" << endl;
        cerr << "LOGLEVEL: [" << (int)(*lit) << "] " << (*lit) << endl;
        for( std::vector<Debug::type>::iterator it=loglevels.begin(); it!=loglevels.end(); ++it )
            dlog[*it] << myname << ": test log print..." << endl;

        cerr << "======= END LOG PRINT ======" << endl;
    }
}
// -----------------------------------------------------------------------------
void TestProc::test_depend()
{
    cerr << myname << ": Check depend..." << endl;

    long test_val = 100;

    // set depend 0...
    setValue(depend_c,0);
    setValue(set_d1_check_s,test_val);
    setValue(set_d2_check_s,test_val);
    cerr << myname << ": check depend OFF: d1: " << ( getValue(d1_check_s) == 0 ? "OK" : "FAIL" ) << endl;
    cerr << myname << ": check depend OFF: d2: " << ( getValue(d2_check_s) == -50 ? "OK" : "FAIL" ) << endl;

    // set depend 1
    setValue(depend_c,1);
    cerr << myname << ": check depend ON: d1: " << ( getValue(d1_check_s) == test_val ? "OK" : "FAIL" ) << endl;
    cerr << myname << ": check depend ON: d2: " << ( getValue(d2_check_s) == test_val ? "OK" : "FAIL" ) << endl;
}
// -----------------------------------------------------------------------------
void TestProc::test_undefined_state()
{
    // ---------------- Проверка выставления неопределённого состояния ---------------------
    cerr << myname << ": Check undef state..." << endl;
    undef ^= true;

    si.id = undef_c;
    si.node = conf->getLocalNode();
    cerr << myname << ": set undefined=" << undef << endl;
    ui.setUndefinedState( si, undef, getId() );
}
// -----------------------------------------------------------------------------
void TestProc::test_thresholds()
{
    // ---------------- Проверка работы порогов ---------------------
    cerr << myname << ": Check thresholds..." << endl;

    setValue(t_set_c,0);
    cerr << myname << ": check threshold OFF value: " << ( getValue(t_check_s) == 0 ? "OK" : "FAIL" ) << endl;

    setValue(t_set_c,378);
    cerr << myname << ": check threshold ON value: " << ( getValue(t_check_s) == 1 ? "OK" : "FAIL" ) << endl;

    cerr << myname << ": ask threshold and check.. " << endl;

    try
    {
        setValue(t_set_c, 0);
        UniSetTypes::ThresholdId tid = 100;
        ui.askThreshold( t_set_c, tid, UniversalIO::UIONotify, 10, 20 );

        IONotifyController_i::ThresholdInfo ti = ui.getThresholdInfo(t_set_c,tid);
        cerr << myname << ": ask OFF threshold: " << ( ti.state == IONotifyController_i::NormalThreshold  ? "OK" : "FAIL" ) << endl;
        setValue(t_set_c, 25);
        ti = ui.getThresholdInfo(t_set_c,tid);
        cerr << myname << ": ask ON threshold: " << ( ti.state == IONotifyController_i::HiThreshold  ? "OK" : "FAIL" ) << endl;
    }
    catch( Exception& ex )
    {
        dlog.level2() << myname << ": CHECK 'ask and get threshold' FAILED: " << ex << endl;
    }
}
// -----------------------------------------------------------------------------
void TestProc::test_loglevel()
{
    lit++;
    if( lit == loglevels.end() )
        lit = loglevels.begin();

    cerr << "SET LOGLEVEL: [" << (int)(*lit) << "] " << (*lit) << endl;
    setValue(log_c, (*lit));
    askTimer(tmLogControl,checkLogTime);
}
// -----------------------------------------------------------------------------
