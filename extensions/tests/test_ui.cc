#include <catch.hpp>

#include <memory>
#include <time.h>
#include "IOController_i.hh"
#include "UInterface.h"
#include "UniSetTypes.h"

using namespace std;
using namespace uniset;

const std::string sidName = "Input1_S";
const std::string aidName = "AI_AS";
ObjectId testOID;
ObjectId sid;
ObjectId aid;
std::shared_ptr<UInterface> ui;

void init()
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    testOID = conf->getObjectID("TestProc");
    CHECK( testOID != DefaultObjectId );

    sid = conf->getSensorID(sidName);
    CHECK( sid != DefaultObjectId );

    aid = conf->getSensorID(aidName);
    CHECK( aid != DefaultObjectId );

    if( ui == nullptr )
        ui = std::make_shared<UInterface>();

    CHECK( ui->getObjectIndex() != nullptr );
    CHECK( ui->getConf() == uniset_conf() );

    REQUIRE( ui->getConfIOType(sid) == UniversalIO::DI );
    REQUIRE( ui->getConfIOType(aid) == UniversalIO::AI );
}

TEST_CASE("UInterface", "[UInterface]")
{
    init();
    auto conf = uniset_conf();

    SECTION( "GET/SET" )
    {
        REQUIRE_THROWS_AS( ui->getValue(DefaultObjectId), uniset::ORepFailed );
        REQUIRE_NOTHROW( ui->setValue(sid, 1) );
        REQUIRE( ui->getValue(sid) == 1 );
        REQUIRE_NOTHROW( ui->setValue(sid, 100) );
        REQUIRE( ui->getValue(sid) == 100 ); // хоть это и дискретный датчик.. функция-то универсальная..

        REQUIRE_THROWS_AS( ui->getValue(sid, DefaultObjectId), uniset::Exception );
        REQUIRE_THROWS_AS( ui->getValue(sid, 100), uniset::Exception );

        REQUIRE_NOTHROW( ui->setValue(aid, 10) );
        REQUIRE( ui->getValue(aid) == 10 );

        IOController_i::SensorInfo si;
        si.id = aid;
        si.node = conf->getLocalNode();
        REQUIRE_NOTHROW( ui->setValue(si, 15, DefaultObjectId) );
        REQUIRE( ui->getRawValue(si) == 15 );

        REQUIRE_NOTHROW( ui->fastSetValue(si, 20, DefaultObjectId) );
        REQUIRE( ui->getValue(aid) == 20 );
        REQUIRE_THROWS_AS( ui->getValue(aid, -2), uniset::Exception );

        si.id = sid;
        REQUIRE_NOTHROW( ui->setValue(si, 15, DefaultObjectId) );
        REQUIRE( ui->getValue(sid) == 15 );

        si.node = -2;
        REQUIRE_THROWS_AS( ui->setValue(si, 20, DefaultObjectId), uniset::Exception );

        REQUIRE_THROWS_AS( ui->getTimeChange(sid, DefaultObjectId), uniset::ORepFailed );
        REQUIRE_NOTHROW( ui->getTimeChange(sid, conf->getLocalNode()) );

        si.id = aid;
        si.node = conf->getLocalNode();
        REQUIRE_NOTHROW( ui->setUndefinedState(si, true, testOID) );
        REQUIRE_NOTHROW( ui->setUndefinedState(si, false, testOID) );

        si.id = sid;
        si.node = conf->getLocalNode();
        REQUIRE_NOTHROW( ui->setUndefinedState(si, true, testOID) );
        REQUIRE_NOTHROW( ui->setUndefinedState(si, false, testOID) );

        REQUIRE( ui->getIOType(sid, conf->getLocalNode()) == UniversalIO::DI );
        REQUIRE( ui->getIOType(aid, conf->getLocalNode()) == UniversalIO::AI );
    }

    SECTION( "resolve" )
    {
        REQUIRE_NOTHROW( ui->resolve(sid) );
        REQUIRE_THROWS_AS( ui->resolve(sid, 10), uniset::ResolveNameError );
        REQUIRE_THROWS_AS( ui->resolve(sid, DefaultObjectId), uniset::ResolveNameError );
    }

    SECTION( "send" )
    {
        TransportMessage tm( SensorMessage(sid, 10).transport_msg() );
        REQUIRE_NOTHROW( ui->send(sid, tm) );
    }

    SECTION( "sendText" )
    {
        TransportMessage tm( SensorMessage(sid, 10).transport_msg() );
        REQUIRE_NOTHROW( ui->send(sid, tm) );
    }


    SECTION( "wait..exist.." )
    {
        CHECK( ui->waitReady(sid, 200, 50) );
        CHECK( ui->waitReady(sid, 200, 50, conf->getLocalNode()) );
        CHECK_FALSE( ui->waitReady(sid, 300, 50, DefaultObjectId) );
        CHECK_FALSE( ui->waitReady(sid, 300, 50, -20) );
        CHECK_FALSE( ui->waitReady(sid, -1, 50) );
        CHECK( ui->waitReady(sid, 300, -1) );

        CHECK( ui->waitWorking(sid, 200, 50) );
        CHECK( ui->waitWorking(sid, 200, 50, conf->getLocalNode()) );
        CHECK_FALSE( ui->waitWorking(sid, 100, 50, DefaultObjectId) );
        CHECK_FALSE( ui->waitWorking(sid, 100, 50, -20) );
        CHECK_FALSE( ui->waitWorking(sid, -1, 50) );
        CHECK( ui->waitWorking(sid, 100, -1) );

        CHECK( ui->isExist(sid) );
        CHECK( ui->isExist(sid, conf->getLocalNode()) );
        CHECK_FALSE( ui->isExist(sid, DefaultObjectId) );
        CHECK_FALSE( ui->isExist(sid, 100) );
    }

    SECTION( "get/set list" )
    {
        uniset::IDList lst;
        lst.add(aid);
        lst.add(sid);
        lst.add(-100); // bad sensor ID

        IOController_i::SensorInfoSeq_var seq = ui->getSensorSeq(lst);
        REQUIRE( seq->length() == 3 );

        IOController_i::OutSeq_var olst = new IOController_i::OutSeq();
        olst->length(2);
        olst[0].si.id = sid;
        olst[0].si.node = conf->getLocalNode();
        olst[0].value = 1;
        olst[1].si.id = aid;
        olst[1].si.node = conf->getLocalNode();
        olst[1].value = 35;

        uniset::IDSeq_var iseq = ui->setOutputSeq(olst, DefaultObjectId);
        REQUIRE( iseq->length() == 0 );

        IOController_i::ShortMapSeq_var slist = ui->getSensors( sid, conf->getLocalNode() );
        REQUIRE( slist->length() >= 2 );
    }

    SECTION( "ask" )
    {
        REQUIRE_THROWS_AS( ui->askSensor(sid, UniversalIO::UIONotify), uniset::IOBadParam );
        REQUIRE_NOTHROW( ui->askSensor(sid, UniversalIO::UIONotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(aid, UniversalIO::UIONotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(aid, UniversalIO::UIODontNotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(sid, UniversalIO::UIODontNotify, testOID) );

        REQUIRE_THROWS_AS( ui->askSensor(-20, UniversalIO::UIONotify), uniset::Exception );

        REQUIRE_NOTHROW( ui->askRemoteSensor(sid, UniversalIO::UIONotify, conf->getLocalNode(), testOID) );
        REQUIRE_NOTHROW( ui->askRemoteSensor(aid, UniversalIO::UIONotify, conf->getLocalNode(), testOID) );
        REQUIRE_THROWS_AS( ui->askRemoteSensor(sid, UniversalIO::UIONotify, -3, testOID), uniset::Exception );

        uniset::IDList lst;
        lst.add(aid);
        lst.add(sid);
        lst.add(-100); // bad sensor ID

        uniset::IDSeq_var rseq = ui->askSensorsSeq(lst, UniversalIO::UIONotify, testOID);
        REQUIRE( rseq->length() == 1 ); // проверяем, что нам вернули один BAD-датчик..(-100)
    }

    SECTION( "Thresholds" )
    {
        REQUIRE_NOTHROW( ui->askThreshold(aid, 10, UniversalIO::UIONotify, 90, 100, false, testOID) );
        REQUIRE_NOTHROW( ui->askThreshold(aid, 11, UniversalIO::UIONotify, 50, 70, false, testOID) );
        REQUIRE_NOTHROW( ui->askThreshold(aid, 12, UniversalIO::UIONotify, 20, 40, false, testOID) );
        REQUIRE_THROWS_AS( ui->askThreshold(aid, 3, UniversalIO::UIONotify, 50, 20, false, testOID), IONotifyController_i::BadRange );

        IONotifyController_i::ThresholdsListSeq_var slist = ui->getThresholdsList(aid);
        REQUIRE( slist->length() == 1 ); // количество датчиков с порогами = 1 (это aid)

        // 3 порога мы создали выше(askThreshold) + 1 который в настроечном файле в секции <thresholds>
        REQUIRE( slist[0].tlist.length() == 4 );

        IONotifyController_i::ThresholdInfo ti1 = ui->getThresholdInfo(aid, 10);
        REQUIRE( ti1.id == 10 );
        REQUIRE( ti1.lowlimit == 90 );
        REQUIRE( ti1.hilimit == 100 );

        IONotifyController_i::ThresholdInfo ti2 = ui->getThresholdInfo(aid, 11);
        REQUIRE( ti2.id == 11 );
        REQUIRE( ti2.lowlimit == 50 );
        REQUIRE( ti2.hilimit == 70 );

        IONotifyController_i::ThresholdInfo ti3 = ui->getThresholdInfo(aid, 12);
        REQUIRE( ti3.id == 12 );
        REQUIRE( ti3.lowlimit == 20 );
        REQUIRE( ti3.hilimit == 40 );

        REQUIRE_THROWS_AS( ui->getThresholdInfo(sid, 10), uniset::NameNotFound );

        // проверяем thresholds который был сформирован из секции <thresholds>
        ui->setValue(10, 378);
        REQUIRE( ui->getValue(13) == 1 );
        ui->setValue(10, 0);
        REQUIRE( ui->getValue(13) == 0 );
    }

    SECTION( "calibration" )
    {
        IOController_i::SensorInfo si;
        si.id = aid;
        si.node = conf->getLocalNode();

        IOController_i::CalibrateInfo ci;
        ci.minRaw = 0;
        ci.maxRaw = 4096;
        ci.minCal = -100;
        ci.maxCal = 100;
        ci.precision = 3;
        REQUIRE_NOTHROW( ui->calibrate(si, ci) );

        IOController_i::CalibrateInfo ci2 = ui->getCalibrateInfo(si);
        CHECK( ci.minRaw == ci2.minRaw );
        CHECK( ci.maxRaw == ci2.maxRaw );
        CHECK( ci.minCal == ci2.minCal );
        CHECK( ci.maxCal == ci2.maxCal );
        CHECK( ci.precision == ci2.precision );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("UInterface::freezeValue", "[UInterface][freezeValue]")
{
    init();
    auto conf = uniset_conf();

    IOController_i::SensorInfo si;
    si.id = aid;
    si.node = conf->getLocalNode();

    REQUIRE_NOTHROW( ui->setValue(aid, 200) );
    REQUIRE( ui->getValue(aid) == 200 );

    REQUIRE_NOTHROW( ui->freezeValue(si, true, 10, testOID) );
    REQUIRE( ui->getValue(aid) == 10 );

    REQUIRE_NOTHROW( ui->setValue(aid, 100) );
    REQUIRE( ui->getValue(aid) == 10 );

    REQUIRE_NOTHROW( ui->freezeValue(si, false, 10, testOID) );
    REQUIRE( ui->getValue(aid) == 100 );

    REQUIRE_NOTHROW( ui->freezeValue(si, true, -1, testOID) );
    REQUIRE( ui->getValue(aid) == -1 );
    REQUIRE_NOTHROW( ui->freezeValue(si, false, 0, testOID) );

    REQUIRE( ui->getValue(aid) == 100 );
    REQUIRE_NOTHROW( ui->setValue(aid, 200) );
    REQUIRE( ui->getValue(aid) == 200 );
}
