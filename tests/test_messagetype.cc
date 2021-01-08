#include <catch.hpp>
// ---------------------------------------------------------------
#include "Configuration.h"
#include "MessageType.h"
#include "UniSetTypes.h"
// ---------------------------------------------------------------
using namespace std;
using namespace uniset;
// ---------------------------------------------------------------
TEST_CASE("Message", "[basic][message types][Message]" )
{
    CHECK( uniset_conf() != nullptr );

    auto conf = uniset_conf();

    Message m;
    CHECK( m.type == Message::Unused );
    CHECK( m.priority == Message::Medium );
    CHECK( m.node == conf->getLocalNode() );
    CHECK( m.supplier == DefaultObjectId );
    CHECK( m.consumer == DefaultObjectId );
}
// ---------------------------------------------------------------
TEST_CASE("VoidMessage", "[basic][message types][VoidMessage]" )
{
    CHECK( uniset_conf() != nullptr );

    auto conf = uniset_conf();

    VoidMessage vm;
    CHECK( vm.type == Message::Unused );
    CHECK( vm.priority == Message::Medium );
    CHECK( vm.node == conf->getLocalNode() );
    CHECK( vm.supplier == DefaultObjectId );
    CHECK( vm.consumer == DefaultObjectId );
}
// ---------------------------------------------------------------
TEST_CASE("SensorMessage", "[basic][message types][SensorMessage]" )
{
    CHECK( uniset_conf() != nullptr );

    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        SensorMessage sm;
        CHECK( sm.type == Message::SensorInfo );
        CHECK( sm.priority == Message::Medium );
        CHECK( sm.node == conf->getLocalNode() );
        CHECK( sm.supplier == DefaultObjectId );
        CHECK( sm.consumer == DefaultObjectId );
        CHECK( sm.id == DefaultObjectId );
        CHECK( sm.value == 0 );
        CHECK( sm.undefined == false );
        //        CHECK( sm.sm_tv_sec == );
        //        CHECK( sm.sm_tv_usec == );

        CHECK( sm.sensor_type == UniversalIO::DI ); // UnknownIOType
        CHECK( sm.ci.precision == 0 );
        CHECK( sm.ci.minRaw == 0 );
        CHECK( sm.ci.maxRaw == 0 );
        CHECK( sm.ci.minCal == 0 );
        CHECK( sm.ci.maxCal == 0 );
        CHECK( sm.threshold == 0 );
        CHECK( sm.tid == uniset::DefaultThresholdId );
    }

    SECTION("Default SensorMessage")
    {
        ObjectId sid = 1;
        long val = 100;
        SensorMessage sm(sid, val);
        REQUIRE( sm.id == sid );
        REQUIRE( sm.value == val );
        REQUIRE( sm.sensor_type == UniversalIO::AI );
    }

    SECTION("Transport SensorMessage")
    {
        ObjectId sid = 1;
        long val = 100;
        SensorMessage sm(sid, val);
        REQUIRE( sm.id == sid );
        REQUIRE( sm.value == val );
        REQUIRE( sm.sensor_type == UniversalIO::AI );

        auto tm = sm.transport_msg();

        VoidMessage vm(tm);
        REQUIRE(vm.type == Message::SensorInfo );

        SensorMessage sm2(&vm);
        REQUIRE( sm2.type == Message::SensorInfo );
        REQUIRE( sm2.id == sid );
        REQUIRE( sm2.value == val );
        REQUIRE( sm2.sensor_type == UniversalIO::AI );
    }
}
// ---------------------------------------------------------------
TEST_CASE("SystemMessage", "[basic][message types][SystemMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        SystemMessage sm;
        CHECK( sm.type == Message::SysCommand );
        CHECK( sm.priority == Message::Medium );
        CHECK( sm.node == conf->getLocalNode() );
        CHECK( sm.supplier == DefaultObjectId );
        CHECK( sm.consumer == DefaultObjectId );
        CHECK( sm.command == SystemMessage::Unknown );
        CHECK( sm.data[0] == 0 );
        CHECK( sm.data[1] == 0 );
    }

    SECTION("Default SystemMessage")
    {
        SystemMessage::Command cmd = SystemMessage::StartUp;
        SystemMessage sm(cmd);
        REQUIRE( sm.command == cmd );
        CHECK( sm.priority == Message::High );
    }

    SECTION("Transport SystemMessage")
    {
        SystemMessage::Command cmd = SystemMessage::StartUp;
        int dat = 100;
        SystemMessage sm(cmd);
        sm.data[0] = dat;
        REQUIRE( sm.command == cmd );

        auto tm = sm.transport_msg();

        VoidMessage vm(tm);
        REQUIRE(vm.type == Message::SysCommand );

        SystemMessage sm2(&vm);
        REQUIRE( sm2.type == Message::SysCommand );
        REQUIRE( sm2.command == cmd );
        REQUIRE( sm2.data[0] == dat );
    }
}
// ---------------------------------------------------------------
TEST_CASE("TimerMessage", "[basic][message types][TimerMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        TimerMessage tm;
        CHECK( tm.type == Message::Timer );
        CHECK( tm.priority == Message::Medium );
        CHECK( tm.node == conf->getLocalNode() );
        CHECK( tm.supplier == DefaultObjectId );
        CHECK( tm.consumer == DefaultObjectId );
        CHECK( tm.id == uniset::DefaultTimerId );
    }

    SECTION("Default TimerMessage")
    {
        int tid = 100;
        TimerMessage tm(tid);
        REQUIRE( tm.id == tid );
    }

    SECTION("Transport TimerMessage")
    {
        int tid = 100;
        TimerMessage tm(tid);
        REQUIRE( tm.id == tid );

        auto m = tm.transport_msg();

        VoidMessage vm(m);
        REQUIRE(vm.type == Message::Timer );

        TimerMessage tm2(&vm);
        REQUIRE( tm2.type == Message::Timer );
        REQUIRE( tm2.id == tid );
    }
}
// ---------------------------------------------------------------
TEST_CASE("ConfirmMessage", "[basic][message types][ConfirmMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    ObjectId sid = 1;
    double val = 100;
    timespec t_event = { 10, 300 };
    timespec t_confirm = { 10, 90 };

    SECTION("Default consturctor")
    {
        ConfirmMessage cm(sid, val, t_event, t_confirm);
        CHECK( cm.type == Message::Confirm );
        CHECK( cm.priority == Message::Medium );
        CHECK( cm.node == conf->getLocalNode() );
        CHECK( cm.supplier == DefaultObjectId );
        CHECK( cm.consumer == DefaultObjectId );
        REQUIRE( cm.sensor_id == sid );
        REQUIRE( cm.sensor_value == val );
        REQUIRE( cm.sensor_time == t_event );
        REQUIRE( cm.confirm_time == t_confirm );
        CHECK( cm.broadcast == false );
        CHECK( cm.forward == false );
    }

    SECTION("Transport ConfirmMessage")
    {
        ConfirmMessage cm(sid, val, t_event, t_confirm);
        REQUIRE( cm.sensor_id == sid );
        REQUIRE( cm.sensor_value == val );
        REQUIRE( cm.sensor_time == t_event );
        REQUIRE( cm.confirm_time == t_confirm );;

        auto tm = cm.transport_msg();

        VoidMessage vm(tm);
        REQUIRE(vm.type == Message::Confirm );

        ConfirmMessage cm2(&vm);
        REQUIRE( cm2.sensor_id == sid );
        REQUIRE( cm2.sensor_value == val );
        REQUIRE( cm.sensor_time == t_event );
        REQUIRE( cm.confirm_time == t_confirm );
    }
}
// ---------------------------------------------------------------
TEST_CASE("TextMessage", "[basic][message types][TextMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        TextMessage tm;
        CHECK( tm.type == Message::TextMessage );
        CHECK( tm.priority == Message::Medium );
        CHECK( tm.node == conf->getLocalNode() );
        CHECK( tm.supplier == DefaultObjectId );
        CHECK( tm.consumer == DefaultObjectId );
        CHECK( tm.txt == "" );
    }

    SECTION("TextMessage from network")
    {
        std::string txt = "Hello world";

        ::uniset::Timespec tspec;
        tspec.sec = 10;
        tspec.nsec = 100;

        ::uniset::ProducerInfo pi;
        pi.id = 30;
        pi.node = conf->getLocalNode();

        ObjectId consumer = 40;

        TextMessage tm(txt.c_str(), 3, tspec, pi, uniset::Message::High, consumer );
        REQUIRE( tm.consumer == consumer );
        REQUIRE( tm.node == pi.node );
        REQUIRE( tm.supplier == pi.id );
        REQUIRE( tm.txt == txt );
        REQUIRE( tm.tm.tv_sec == tspec.sec );
        REQUIRE( tm.tm.tv_nsec == tspec.nsec );
        REQUIRE( tm.mtype == 3 );

        auto vm = tm.toLocalVoidMessage();

        REQUIRE( vm->type == Message::TextMessage );

        TextMessage tm2(vm.get());
        REQUIRE( tm2.consumer == consumer );
        REQUIRE( tm2.node == pi.node );
        REQUIRE( tm2.supplier == pi.id );
        REQUIRE( tm2.txt == txt );
        REQUIRE( tm2.tm.tv_sec == tspec.sec );
        REQUIRE( tm2.tm.tv_nsec == tspec.nsec );
        REQUIRE( tm2.mtype == 3 );
    }
}
// ---------------------------------------------------------------
