#include "Exceptions.h"
#include "TestGen.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TestGen::TestGen( uniset::ObjectId id, xmlNode* confnode ):
    TestGen_SK( id, confnode )
{
    vmonit(int_var);
    vmonit(bool_var);

    const long* i = valptr(input2_s);
    long* k = outptr(output1_c);

    if( !k )
        cerr << "output1_c NOT FOUND!!!" << endl;


    ObjectId d = idval(in_input2_s);
    ObjectId d2 = idval(&in_input2_s);
    ObjectId d3 = idval(i);
    ObjectId d4 = idval(&out_output1_c);

    if( !i )
        cerr << "input2_s NOT FOUND!!!" << endl;
    else
        cerr << "input2_s=" << (*i)  << " d=" << d << " d2=" << d2 << " d3=" << d3 << " input2_s=" << input2_s << endl;

    vmonit(t_val);
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
#if 0
    cout << "strval: " << strval(input2_s) << endl;
    cout << "str: " << str(input2_s) << endl;
    cout << "===========" << endl;
    cout << dumpIO() << endl;

    myinfo << str(input2_s) << endl;
    ulog()->info() << "ulog: " << str(input2_s) << endl;

    int_var++;
    bool_var ^= true;
    cout << vmon << endl;
#endif
    //  cout << vmon.pretty_str() << endl;
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
    if( tm->id == 1 )
    {
	cout << "**** timer 1 ****" << endl;
        askTimer(1, 0);
        askTimer(2, 3000);
    }
    else if( tm->id == 2 )
    {
    	cout << "**** timer 2 ****" << endl;
        askTimer(1, 2000);
        askTimer(2, 0);
    }
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
void TestGen::httpGetUserData( Poco::JSON::Object::Ptr& jdata )
{
    jdata->set("myMode", "RUNNING");
    jdata->set("myVar", 42);
    jdata->set("myFloatVar", 42.42);
    jdata->set("myMessage", "This is text fot test httpGetUserData");
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
void TestGen::sysCommand( const uniset::SystemMessage* sm )
{
    if( sm->command == SystemMessage::StartUp )
        askTimer(1, 2000);
}
// -----------------------------------------------------------------------------
