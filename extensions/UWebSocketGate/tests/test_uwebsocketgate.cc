#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include <memory>
#include <iostream>
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/JSON/Parser.h"
#include "UniSetTypes.h"
#include "UInterface.h"
// -----------------------------------------------------------------------------
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::WebSocket;
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static int port = 8081;
static string addr("127.0.0.1");
static std::shared_ptr<UInterface> ui;
// -----------------------------------------------------------------------------
static Poco::Dynamic::Var httpGetJson(const std::string& path, HTTPResponse::HTTPStatus expectStatus = HTTPResponse::HTTP_OK)
{
    HTTPClientSession cs(addr, port);
    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPRequest::HTTP_1_1);
    HTTPResponse response;
    cs.sendRequest(request);
    std::istream& rs = cs.receiveResponse(response);
    REQUIRE(response.getStatus() == expectStatus);

    std::ostringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    return parser.parse(ss.str());
}
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
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: parse command", "[uwebsocketgate]")
{
    {
        string_view cmd = "ask:sensor1,sensor2";
        auto pos = cmd.find_first_of(':');
        REQUIRE_FALSE(pos == string_view::npos);
        REQUIRE(cmd.substr(0, pos) == "ask");
        REQUIRE(cmd.substr(pos + 1) == "sensor1,sensor2");
    }

    {
        string_view cmd = "freeze:sensor1=1,sensor2=2";
        auto pos = cmd.find_first_of(':');
        REQUIRE_FALSE(pos == string_view::npos);
        REQUIRE(cmd.substr(0, pos) == "freeze");
        REQUIRE(cmd.substr(pos + 1) == "sensor1=1,sensor2=2");
    }

    {
        string_view cmd = "unfreeze:sensor1,sensor2";
        auto pos = cmd.find_first_of(':');
        REQUIRE_FALSE(pos == string_view::npos);
        REQUIRE(cmd.substr(0, pos) == "unfreeze");
        REQUIRE(cmd.substr(pos + 1) == "sensor1,sensor2");
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: set", "[uwebsocketgate]")
{
    InitTest();

    HTTPClientSession cs(addr, port);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/wsgate", HTTPRequest::HTTP_1_1);
    HTTPResponse response;
    WebSocket ws(cs, request, response);

    std::string cmd("set:1=10,2=20,3=30");
    ws.sendFrame(cmd.data(), (int)cmd.size());

    msleep(50);

    REQUIRE( ui->getValue(1) == 10 );
    REQUIRE( ui->getValue(2) == 20 );
    REQUIRE( ui->getValue(3) == 30 );

    cmd = "set:1=11,2=21,3=31";
    ws.sendFrame(cmd.data(), (int)cmd.size());

    msleep(50);

    REQUIRE( ui->getValue(1) == 11 );
    REQUIRE( ui->getValue(2) == 21 );
    REQUIRE( ui->getValue(3) == 31 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: freeze/unfreeze", "[uwebsocketgate]")
{
    InitTest();

    REQUIRE_NOTHROW( ui->setValue(1, 1) );
    REQUIRE_NOTHROW( ui->setValue(2, 2) );
    REQUIRE_NOTHROW( ui->setValue(3, 3) );

    REQUIRE( ui->getValue(1) == 1 );
    REQUIRE( ui->getValue(2) == 2 );
    REQUIRE( ui->getValue(3) == 3 );

    HTTPClientSession cs(addr, port);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/wsgate", HTTPRequest::HTTP_1_1);
    HTTPResponse response;
    WebSocket ws(cs, request, response);

    std::string cmd("freeze:1=10,2=20,3=30");
    ws.sendFrame(cmd.data(), (int)cmd.size());

    msleep(50);

    REQUIRE( ui->getValue(1) == 10 );
    REQUIRE( ui->getValue(2) == 20 );
    REQUIRE( ui->getValue(3) == 30 );

    REQUIRE_NOTHROW( ui->setValue(1, 11) );
    REQUIRE_NOTHROW( ui->setValue(2, 12) );
    REQUIRE_NOTHROW( ui->setValue(3, 13) );

    REQUIRE( ui->getValue(1) == 10 );
    REQUIRE( ui->getValue(2) == 20 );
    REQUIRE( ui->getValue(3) == 30 );

    cmd = "unfreeze:1,2,3";
    ws.sendFrame(cmd.data(), (int)cmd.size());

    msleep(50);

    REQUIRE( ui->getValue(1) == 11 );
    REQUIRE( ui->getValue(2) == 12 );
    REQUIRE( ui->getValue(3) == 13 );

    REQUIRE_NOTHROW( ui->setValue(1, 1) );
    REQUIRE_NOTHROW( ui->setValue(2, 2) );
    REQUIRE_NOTHROW( ui->setValue(3, 3) );
    REQUIRE( ui->getValue(1) == 1 );
    REQUIRE( ui->getValue(2) == 2 );
    REQUIRE( ui->getValue(3) == 3 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: ask", "[uwebsocketgate]")
{
    InitTest();

    HTTPClientSession cs(addr, port);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/wsgate", HTTPRequest::HTTP_1_1);
    HTTPResponse response;
    WebSocket ws(cs, request, response);

    ui->setValue(1, 1);
    ui->setValue(2, 42);
    ui->setValue(3, 42);

    std::string cmd("ask:1,2,3");
    ws.sendFrame(cmd.data(), (int)cmd.size());

    char buffer[1024] = {};
    int flags;
    ws.receiveFrame(buffer, sizeof(buffer), flags);
    REQUIRE(flags == WebSocket::FRAME_TEXT);

    Poco::JSON::Parser parser;
    auto result = parser.parse(buffer);
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    //  {
    //     "data": [
    //      {"type": "SensorInfo"...},
    //      {"type": "SensorInfo"...},
    //      {"type": "SensorInfo"...}
    //      ...
    //     ]
    //  }

    auto jdata = json->get("data").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jdata);

    REQUIRE(jdata->size() == 3);

    for( int i = 0; i < 3; i++ )
    {
        auto j = jdata->getObject(i);
        REQUIRE(j);
        REQUIRE( j->get("type").convert<std::string>() == "SensorInfo" );
        REQUIRE( j->has("supplier_id") );
        const long supplier_id = j->get("supplier_id").convert<long>();
        if( supplier_id != DefaultObjectId )
            REQUIRE( j->has("supplier") );

        long id  = j->get("id").convert<long>();

        if( id == 1 )
        {
            REQUIRE( j->get("iotype").convert<std::string>() == "DI" );
            REQUIRE( j->get("value").convert<long>() == 1 );
        }
        else
        {
            REQUIRE( j->get("iotype").convert<std::string>() == "AI" );
            REQUIRE( j->get("value").convert<long>() == 42 );
        }
    }


    // sensorInfo
    ui->setValue(2, 84);
    memset(buffer, 0, sizeof(buffer));
    ws.receiveFrame(buffer, sizeof(buffer), flags);
    REQUIRE(flags == WebSocket::FRAME_TEXT);

    result = parser.parse(buffer);
    json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);
    jdata = json->get("data").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jdata);
    REQUIRE(jdata->size() == 1);
    auto j = jdata->getObject(0);
    REQUIRE(j);
    REQUIRE( j->get("type").convert<std::string>() == "SensorInfo" );
    REQUIRE( j->has("supplier_id") );
    const long supplier_id = j->get("supplier_id").convert<long>();
    if( supplier_id != DefaultObjectId )
        REQUIRE( j->has("supplier") );
    REQUIRE( j->get("iotype").convert<std::string>() == "AI" );
    REQUIRE( j->get("value").convert<long>() == 84 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: ask max", "[uwebsocketgate][cmdmax]")
{
    try
    {
        InitTest();

        HTTPClientSession cs(addr, port);
        HTTPRequest request(HTTPRequest::HTTP_GET, "/wsgate", HTTPRequest::HTTP_1_1);
        HTTPResponse response;
        WebSocket ws(cs, request, response);

        ui->setValue(1, 1);
        ui->setValue(2, 42);
        ui->setValue(3, 42);
        ui->setValue(4, 1);
        ui->setValue(5, 42);
        ui->setValue(6, 42);

        std::string cmd("ask:1,2,3,4,5,6");
        ws.sendFrame(cmd.data(), (int)cmd.size());

        char buffer[1024] = {};
        int flags;
        ws.receiveFrame(buffer, sizeof(buffer), flags);
        REQUIRE(flags == WebSocket::FRAME_TEXT);

        Poco::JSON::Parser parser;
        auto result = parser.parse(buffer);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        //  {
        //     "data": [
        //      {"type": "SensorInfo"...},
        //      {"type": "SensorInfo"...},
        //      {"type": "SensorInfo"...}
        //      ...
        //     ]
        //  }

        auto jdata = json->get("data").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(jdata);

        REQUIRE(jdata->size() == 6);

        for( int i = 0; i < 6; i++ )
        {
            auto j = jdata->getObject(i);
            REQUIRE(j);
            REQUIRE( j->get("type").convert<std::string>() == "SensorInfo" );
            REQUIRE( j->has("supplier_id") );
            const long supplier_id = j->get("supplier_id").convert<long>();
            if( supplier_id != DefaultObjectId )
                REQUIRE( j->has("supplier") );

            long id  = j->get("id").convert<long>();

            if( id == 1 || id == 3 )
            {
                REQUIRE( j->get("iotype").convert<std::string>() == "DI" );
                REQUIRE( j->get("value").convert<long>() == 1 );
            }
            else
            {
                REQUIRE( j->get("iotype").convert<std::string>() == "AI" );
                REQUIRE( j->get("value").convert<long>() == 42 );
            }
        }


        // sensorInfo

        ui->setValue(2, 84);
        memset(buffer, 0, sizeof(buffer));
        ws.receiveFrame(buffer, sizeof(buffer), flags);
        REQUIRE(flags == WebSocket::FRAME_TEXT);

        result = parser.parse(buffer);
        json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);
        jdata = json->get("data").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(jdata);
        REQUIRE(jdata->size() == 1);
        auto j = jdata->getObject(0);
        REQUIRE(j);
        REQUIRE( j->get("type").convert<std::string>() == "SensorInfo" );
        REQUIRE( j->has("supplier_id") );
        const long supplier_id = j->get("supplier_id").convert<long>();
        if( supplier_id != DefaultObjectId )
            REQUIRE( j->has("supplier") );
        REQUIRE( j->get("iotype").convert<std::string>() == "AI" );
        REQUIRE( j->get("value").convert<long>() == 84 );

    } // TODO: WTF?
    catch( std::exception& ex ) {}
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: del", "[uwebsocketgate]")
{
    InitTest();

    HTTPClientSession cs(addr, port);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/wsgate", HTTPRequest::HTTP_1_1);
    HTTPResponse response;
    WebSocket ws(cs, request, response);

    ui->setValue(1, 1);

    std::string cmd("ask:1");
    ws.sendFrame(cmd.data(), (int)cmd.size());

    char buffer[1024] = {};
    int flags;
    ws.receiveFrame(buffer, sizeof(buffer), flags);
    REQUIRE(flags == WebSocket::FRAME_TEXT);

    cmd = ("del:1");
    ws.sendFrame(cmd.data(), (int)cmd.size());

    msleep(100);

    ui->setValue(1, 0);
    memset(buffer, 0, sizeof(buffer));
    ws.receiveFrame(buffer, sizeof(buffer), flags);

    string str(buffer);
    REQUIRE( str == "." );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: get", "[uwebsocketgate]")
{
    InitTest();

    HTTPClientSession cs(addr, port);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/wsgate", HTTPRequest::HTTP_1_1);
    HTTPResponse response;
    WebSocket ws(cs, request, response);

    ui->setValue(1, 111);
    ui->setValue(2, 222);
    ui->setValue(3, 333);

    std::string cmd("get:1,2,3");
    ws.sendFrame(cmd.data(), (int)cmd.size());

    char buffer[1024] = {};
    int flags;
    memset(buffer, 0, sizeof(buffer));
    ws.receiveFrame(buffer, sizeof(buffer), flags);
    REQUIRE(flags == WebSocket::FRAME_TEXT);

    Poco::JSON::Parser parser;
    auto result = parser.parse(buffer);
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    //  {
    //     "data": [
    //      {"type": "ShortSensorInfo"...},
    //      {"type": "ShortSensorInfo"...},
    //      {"type": "ShortSensorInfo"...},
    //      ...
    //     ]
    //  }

    auto jdata = json->get("data").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jdata);

    REQUIRE(jdata->size() == 3);

    for( int i = 0; i < 3; i++ )
    {
        auto j = jdata->getObject(i);
        REQUIRE(j);
        REQUIRE( j->get("type").convert<std::string>() == "ShortSensorInfo" );
        REQUIRE( j->has("supplier_id") );
        const long supplier_id = j->get("supplier_id").convert<long>();
        if( supplier_id != DefaultObjectId )
            REQUIRE( j->has("supplier") );

        long id  = j->get("id").convert<long>();

        if( id == 1 )
        {
            REQUIRE( j->get("value").convert<long>() == 111 );
        }
        else if( id == 2 )
        {
            REQUIRE( j->get("value").convert<long>() == 222 );
        }
        else if( id == 3 )
        {
            REQUIRE( j->get("value").convert<long>() == 333 );
        }
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UWebSocketGate]: http api", "[uwebsocketgate][http]")
{
    InitTest();

    {
        auto result = httpGetJson("/api/v2/list");
        auto arr = result.extract<Poco::JSON::Array::Ptr>();
        REQUIRE(arr);
        REQUIRE(arr->size() == 1);
        REQUIRE(arr->get(0).convert<std::string>() == "UWebSocketGate1");
    }

    {
        auto result = httpGetJson("/api/v2/UWebSocketGate1/help");
        auto json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);
        REQUIRE(json->has("UWebSocketGate1"));
    }

    {
        auto result = httpGetJson("/api/v2/UWebSocketGate1/status");
        auto json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);
        auto obj = json->getObject("object");
        REQUIRE(obj);
        REQUIRE(obj->getValue<std::string>("extensionType") == "UWebSocketGate");
        REQUIRE(obj->has("logserver"));
        REQUIRE(obj->has("websockets"));
    }

    {
        auto result = httpGetJson("/api/v2/UWebSocketGate1/list");
        auto json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);
        REQUIRE(json->has("sessions"));
    }
}
// -----------------------------------------------------------------------------
