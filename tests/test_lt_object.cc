#include <catch.hpp>
// --------------------------------------------------------------------------
#include "UniSetObject.h"
#include "MessageType.h"
#include "Configuration.h"
#include "UHelpers.h"
#include "LT_Object.h"
#include "TestUObject.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
shared_ptr<TestUObject> lt_uobj;
// --------------------------------------------------------------------------
static void initTest()
{
    REQUIRE( uniset_conf() != nullptr );

    if( !lt_uobj )
    {
        lt_uobj = make_object<TestUObject>("TestUObject1", "TestUObject");
        REQUIRE( lt_uobj != nullptr );
    }
}
// --------------------------------------------------------------------------
TEST_CASE( "LT_Object: local timer1", "[lt_object]" )
{
    initTest();

    auto lt = LT_Object();

    lt.checkTimers(lt_uobj.get());
    REQUIRE( lt_uobj->mqEmpty() == true );

    // askTimer
    lt.askTimer(1, 20, -1);
    REQUIRE( lt.checkTimers(lt_uobj.get()) == 20 );
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 20 );
    REQUIRE( lt.getTimeLeft(1) == 20 );

    msleep(50);
    lt.checkTimers(lt_uobj.get());
    REQUIRE( lt_uobj->mqEmpty() == false );

    auto msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    auto tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 1 );
    REQUIRE( tmsg->interval_msec == 20 );
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 20 );
    REQUIRE( lt.getTimeLeft(1) == 20 );
    msleep(50);

    REQUIRE( lt.checkTimers(lt_uobj.get()) == 20 );
    REQUIRE( lt_uobj->mqEmpty() == false );
    msg = lt_uobj->getOneMessage();
    REQUIRE( lt_uobj->mqEmpty() == true );

    // reaskTimer
    lt.askTimer(1, 150, -1);
    REQUIRE( lt.getTimeInterval(1) == 150 );
    msleep(30);
    lt.askTimer(1, 20, -1);
    REQUIRE( lt.getTimeInterval(1) == 20 );
    REQUIRE( lt.getTimeLeft(1) == 20 );

    // remove timer
    REQUIRE( (int)lt.askTimer(1, 0) == (int)UniSetTimer::WaitUpTime );
    REQUIRE( lt.getTimeInterval(1) == 0 );
    REQUIRE( lt.getTimeLeft(1) == 0 );
    REQUIRE( (int)lt.checkTimers(lt_uobj.get()) == (int)UniSetTimer::WaitUpTime );
    REQUIRE( lt_uobj->mqEmpty() == true );
}
// --------------------------------------------------------------------------
TEST_CASE( "LT_Object: local timer1, timer2", "[lt_object]" )
{
    initTest();

    auto lt = LT_Object();

    // askTimer
    lt.askTimer(1, 20, -1);
    lt.askTimer(2, 30, -1);
    REQUIRE( lt.checkTimers(lt_uobj.get()) == 20 );
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 20 );
    REQUIRE( lt.getTimeLeft(1) == 20 );
    REQUIRE( lt.getTimeInterval(2) == 30 );
    REQUIRE( lt.getTimeLeft(2) == 30 );
    msleep(50);

    // msg timer1
    REQUIRE( lt.checkTimers(lt_uobj.get()) == 20 );
    auto msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    auto tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 1 );
    REQUIRE( tmsg->interval_msec == 20 );
    REQUIRE( lt_uobj->mqEmpty() == false );

    // msg timer2
    msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 2 );
    REQUIRE( tmsg->interval_msec == 30 );
    REQUIRE( lt_uobj->mqEmpty() == true );

    // remove timer1
    lt.askTimer(1, 0);
    REQUIRE( lt.getTimeInterval(1) == 0 );
    REQUIRE( lt.getTimeLeft(1) == 0 );
    REQUIRE( lt.getTimeInterval(2) == 30 );
    REQUIRE( lt.getTimeLeft(2) == 30 );
    msleep(40);
    REQUIRE( lt.checkTimers(lt_uobj.get()) == 30 );
    msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 2 );
    REQUIRE( tmsg->interval_msec == 30 );
    REQUIRE( lt_uobj->mqEmpty() == true );
}
// --------------------------------------------------------------------------
TEST_CASE( "LT_Object: timer ticks", "[lt_object]" )
{
    initTest();

    auto lt = LT_Object();
    // askTimer
    lt.askTimer(1, 20, 2);
    REQUIRE( lt.checkTimers(lt_uobj.get()) == 20 );
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 20 );
    REQUIRE( lt.getTimeLeft(1) == 20 );

    // first event
    msleep(30);
    REQUIRE( lt.checkTimers(lt_uobj.get()) == 20 );
    auto msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    auto tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 1 );
    REQUIRE( tmsg->interval_msec == 20 );

    // second event
    msleep(30);
    REQUIRE( (int)lt.checkTimers(lt_uobj.get()) == (int)UniSetTimer::WaitUpTime );
    msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 1 );
    REQUIRE( tmsg->interval_msec == 20 );

    msleep(30);

    // no event
    REQUIRE( (int)lt.checkTimers(lt_uobj.get()) == (int)UniSetTimer::WaitUpTime );
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 0 );
    REQUIRE( lt.getTimeLeft(1) == 0 );

    // reask
    lt.askTimer(1, 20, 1);
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 20 );
    REQUIRE( lt.getTimeLeft(1) == 20 );

    // first event
    msleep(30);
    lt.checkTimers(lt_uobj.get());
    msg = lt_uobj->getOneMessage();
    REQUIRE( msg->type == Message::Timer );
    tmsg = reinterpret_cast<const TimerMessage*>(msg.get());
    REQUIRE( tmsg->id == 1 );
    REQUIRE( tmsg->interval_msec == 20 );

    msleep(30);

    // no event
    REQUIRE( (int)lt.checkTimers(lt_uobj.get()) == (int)UniSetTimer::WaitUpTime );
    REQUIRE( lt_uobj->mqEmpty() == true );
    REQUIRE( lt.getTimeInterval(1) == 0 );
    REQUIRE( lt.getTimeLeft(1) == 0 );
}
// --------------------------------------------------------------------------
