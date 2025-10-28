#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <limits>
#include <memory>

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/JSON/Parser.h"

#include "UniSetTypes.h"
#include "UInterface.h"
#include "client.h"
#include "OPCUAServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static string addr  = "opc.tcp://127.0.0.1:44999";
static std::shared_ptr<UInterface> ui;
static uint16_t nodeId = 0;
static int pause_msec = 200;
static const string httpAddr = "127.0.0.1";
static const uint16_t httpPort = 9090;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = std::make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    REQUIRE( opcuaCreateClient(addr) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAServer]: read", "[opcuaserver]")
{
    InitTest();

    ui->setValue(2, 100);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI1_S") == 100 );

    ui->setValue(2, -100);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI1_S") == -100 );


    ui->setValue(1, 1);
    msleep(pause_msec);
    REQUIRE( opcuaReadBool(nodeId, "DI1_S") );

    ui->setValue(1, 0);
    msleep(pause_msec);
    REQUIRE_FALSE( opcuaReadBool(nodeId, "DI1_S") );

    // float
    ui->setValue(8, 2022);
    msleep(pause_msec);
    REQUIRE( opcuaReadFloat(nodeId, "AI_Float_S") == 20.22f );

    ui->setValue(8, 2025);
    msleep(pause_msec);
    REQUIRE( opcuaReadFloat(nodeId, "AI_Float_S") == 20.25f );
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAServer]: write", "[opcuaserver]")
{
    InitTest();

    REQUIRE( ui->getValue(3) == 0 );
    REQUIRE( opcuaWrite(nodeId, "AO1_S", 10500) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 10500 );

    REQUIRE( opcuaWriteBool(nodeId, "DO1_S", true) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(4) == 1 );

    REQUIRE( opcuaWriteBool(nodeId, "DO1_S", false) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(4) == 0 );

    // float
    REQUIRE( opcuaWriteFloat(nodeId, "AO_Float_S", 20.22f) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(9) == 2022 );

    // float: round up
    REQUIRE( opcuaWriteFloat(nodeId, "AO_Float_S", 20.225f) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(9) == 2023 );

    // float: round down
    REQUIRE( opcuaWriteFloat(nodeId, "AO_Float_S", 20.223f) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(9) == 2022 );

}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: first bit", "[opcuaserver][firstbit]")
{
    REQUIRE( OPCUAServer::firstBit(4) == 2 );
    REQUIRE( OPCUAServer::firstBit(1) == 0 );
    REQUIRE( OPCUAServer::firstBit(64) == 6 );
    REQUIRE( OPCUAServer::firstBit(12) == 2 );
    REQUIRE( OPCUAServer::firstBit(192) == 6 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAServer: get bits", "[opcuaserver][getbits]")
{
    REQUIRE( OPCUAServer::getBits(1, 1, 0) == 1 );
    REQUIRE( OPCUAServer::getBits(12, 12, 2) == 3 );
    REQUIRE( OPCUAServer::getBits(9, 12, 2) == 2 );
    REQUIRE( OPCUAServer::getBits(9, 3, 0) == 1 );
    REQUIRE( OPCUAServer::getBits(15, 12, 2) == 3 );
    REQUIRE( OPCUAServer::getBits(15, 3, 0) == 3 );
    REQUIRE( OPCUAServer::getBits(15, 0, 0) == 15 );
    REQUIRE( OPCUAServer::getBits(15, 0, 3) == 15 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAServer: set bits", "[opcuaserver][setbits]")
{
    REQUIRE( OPCUAServer::setBits(1, 1, 1, 0) == 1 );
    REQUIRE( OPCUAServer::setBits(0, 3, 12, 2) == 12 );
    REQUIRE( OPCUAServer::setBits(3, 3, 12, 2) == 15 );
    REQUIRE( OPCUAServer::setBits(16, 3, 12, 2) == 28 );
    REQUIRE( OPCUAServer::setBits(28, 1, 3, 0) == 29 );
    REQUIRE( OPCUAServer::setBits(28, 10, 0, 0) == 28 );
    REQUIRE( OPCUAServer::setBits(28, 10, 0, 4) == 28 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAServer: force set bits", "[opcuaserver][forcesetbits]")
{
    REQUIRE( OPCUAServer::forceSetBits(1, 1, 1, 0) == 1 );
    REQUIRE( OPCUAServer::forceSetBits(0, 3, 12, 2) == 12 );
    REQUIRE( OPCUAServer::forceSetBits(28, 10, 0, 0) == 10 );
    REQUIRE( OPCUAServer::forceSetBits(28, 10, 0, 4) == 10 );

    REQUIRE( OPCUAServer::forceSetBits(0, 2, 3, 0) == 2 );
    REQUIRE( OPCUAServer::forceSetBits(2, 2, 12, 2) == 10 );

    REQUIRE( OPCUAServer::forceSetBits(2, 0, 3, 0) == 0 );
    REQUIRE( OPCUAServer::forceSetBits(12, 0, 12, 2) == 0 );

    REQUIRE( OPCUAServer::forceSetBits(3, 5, 3, 0) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAServer]: mask read/write", "[opcuaserver][mask]")
{
    InitTest();

    // read
    ui->setValue(5, 1);
    ui->setValue(6, 5);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI5_S") == 1 );
    REQUIRE( opcuaRead(nodeId, "AI6_S") == 1 );

    ui->setValue(5, 3);
    ui->setValue(6, 15);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI5_S") == 3 );
    REQUIRE( opcuaRead(nodeId, "AI6_S") == 3 );

    ui->setValue(5, 16);
    ui->setValue(6, 16);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI5_S") == 0 );
    REQUIRE( opcuaRead(nodeId, "AI6_S") == 0 );

    // write
    REQUIRE( opcuaWrite(nodeId, "AO7_S", 1) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(7) == 1 );

    REQUIRE( opcuaWrite(nodeId, "AO7_S", 3) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(7) == 3 );

    REQUIRE( opcuaWrite(nodeId, "AO7_S", 5) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(7) == 1 );

    REQUIRE( opcuaWrite(nodeId, "AO7_S", 15) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(7) == 3 );

    REQUIRE( opcuaWrite(nodeId, "AO7_S", 0) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(7) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAServer]: reconnect check", "[opcuaserver][reconnect]")
{
    InitTest();

    REQUIRE( opcuaWrite(nodeId, "AO1_S", 0) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 0 );
    REQUIRE( opcuaWrite(nodeId, "AO1_S", 10500) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 10500 );

    msleep(4000); // Default maxSessionTimeout=5000

    REQUIRE( opcuaWrite(nodeId, "AO1_S", 101) ); // Status = Good
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 101 );

    msleep(6000); // Default maxSessionTimeout=5000

    REQUIRE_FALSE( opcuaWrite(nodeId, "AO1_S", 1) ); // Status = BadSessionClosed or BadSessionIdInvalid

    REQUIRE( opcuaReconnectClient(addr) );

    REQUIRE( opcuaWrite(nodeId, "AO1_S", 1) ); // Status = Good
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 1 );
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
TEST_CASE("OPCUAServer: HTTP /status", "[http][opcua][status]")
{
    InitTest(); // если у тебя есть общий инициализатор окружения

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v01/OPCUAServer/status", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto parsed = parser.parse(ss.str());
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(root);
    REQUIRE(root->get("result").toString() == "OK");

    auto st = root->getObject("status");
    REQUIRE(st);
    REQUIRE(st->has("name"));

    if( st->has("endpoint") ) REQUIRE(st->getObject("endpoint")->has("url"));

    if( st->has("params") )   REQUIRE(st->getObject("params")->has("updateTime_msec"));
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAServer: HTTP getparam/setparam", "[http][opcua][params]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // 1) читаем текущие значения
    int prev_ut = 0;
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v01/OPCUAServer/getparam?name=updateTime_msec",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        auto j = r.extract<Poco::JSON::Object::Ptr>();
        auto p = j->getObject("params");
        prev_ut = (int)p->get("updateTime_msec");
    }

    int new_ut = prev_ut + 60000; // +1 мин

    // 2) пробуем применить
    HTTPRequest reqSet(HTTPRequest::HTTP_GET,
                       std::string("/api/v01/OPCUAServer/setparam?")
                       + "updateTime_msec=" + std::to_string(new_ut),
                       HTTPRequest::HTTP_1_1);
    HTTPResponse resSet;
    cs.sendRequest(reqSet);
    std::istream& rsSet = cs.receiveResponse(resSet);

    std::stringstream ssSet;
    ssSet << rsSet.rdbuf();
    const auto body = ssSet.str();

    if( resSet.getStatus() == HTTPResponse::HTTP_OK )
    {
        auto rSet = parser.parse(body);
        auto jSet = rSet.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(jSet->get("result").toString() == "OK");

        // 3) проверяем
        HTTPRequest req2(HTTPRequest::HTTP_GET,
                         "/api/v01/OPCUAServer/getparam?name=updateTime_msec",
                         HTTPRequest::HTTP_1_1);
        HTTPResponse res2;
        cs.sendRequest(req2);
        std::istream& rs2 = cs.receiveResponse(res2);
        REQUIRE(res2.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss2;
        ss2 << rs2.rdbuf();
        auto r2 = parser.parse(ss2.str());
        auto j2 = r2.extract<Poco::JSON::Object::Ptr>();
        auto p2 = j2->getObject("params");
        REQUIRE((int)p2->get("updateTime_msec") == new_ut);

        // 4) возвращаем
        HTTPRequest reqBack(HTTPRequest::HTTP_GET,
                            std::string("/api/v01/OPCUAServer/setparam?")
                            + "sessionTimeout=" + std::to_string(prev_ut),
                            HTTPRequest::HTTP_1_1);
        HTTPResponse resBack;
        cs.sendRequest(reqBack);
        std::istream& rsBack = cs.receiveResponse(resBack);
        REQUIRE(resBack.getStatus() == HTTPResponse::HTTP_OK);
    }
    else
    {
        // setparam заблокирован флагом
        REQUIRE(resSet.getStatus() >= HTTPResponse::HTTP_BAD_REQUEST);
        REQUIRE(body.find("httpEnabledSetParams") != std::string::npos);
        REQUIRE(body.find("disabled") != std::string::npos);
    }
}
// -----------------------------------------------------------------------------
#endif // DISABLE_REST_API
// -----------------------------------------------------------------------------