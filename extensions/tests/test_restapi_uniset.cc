#ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include <Poco/JSON/Parser.h>
#include "Exceptions.h"
#include "Extensions.h"
#include "tests_with_sm.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
static std::shared_ptr<SMInterface> shm;
static uniset::ObjectId testOID = DefaultObjectId;
static ObjectId aid = DefaultObjectId;
static const std::string aidName = "AI_AS";
// -----------------------------------------------------------------------------
static void init_test()
{
    shm = smiInstance();
    REQUIRE( shm != nullptr );

    auto conf = uniset_conf();

    REQUIRE( conf != nullptr );

    testOID = conf->getObjectID("TestProc");
    CHECK( testOID != DefaultObjectId );

    aid = conf->getSensorID(aidName);
    CHECK( aid != DefaultObjectId );
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: conf]", "[restapi][configure]")
{
    init_test();

    std::string s = shm->apiRequest("/configure/get?2,Input5_S&params=iotype");
    Poco::JSON::Parser parser;
    auto result = parser.parse(s);

    // Ожидаемый формат ответа:
    // {"conf": [
    //      {"id":"2","iotype":"DI","mbaddr":"0x01","mbfunc":"0x06","mbreg":"0x02","mbtype":"rtu","name":"Input2_S","nbit":"11","priority":"Medium","rs":"4","textname":"Команда 2"}
    //      ],
    //      "object":
    //           {"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"}
    //     }
    //
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jconf = json->get("conf").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jconf);

    Poco::JSON::Object::Ptr jret = jconf->getObject(0);
    REQUIRE(jret);

    REQUIRE( jret->get("iotype").convert<std::string>() == "DI" );
    REQUIRE( jret->get("id").convert<ObjectId>() == 2 );

    Poco::JSON::Object::Ptr jret2 = jconf->getObject(1);
    REQUIRE(jret2);

    REQUIRE( jret2->get("iotype").convert<std::string>() == "DI" );
    REQUIRE( jret2->get("name").convert<std::string>() == "Input5_S" );
    REQUIRE( jret2->get("id").convert<ObjectId>() == 5 );

}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: /]", "[restapi][info]")
{
    init_test();

    std::string s = shm->apiRequest("/");
    Poco::JSON::Parser parser;
    auto result = parser.parse(s);

    // Ожидаемый формат ответа:
    // {"object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"}}

    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jret = json->get("object").extract<Poco::JSON::Object::Ptr>();
    REQUIRE(jret);

    auto sm = shm->SM();

    REQUIRE( jret->get("id").convert<ObjectId>() == sm->getId() );
    REQUIRE( jret->get("name").convert<std::string>() == sm->getName() );
    REQUIRE( jret->get("objectType").convert<std::string>() == "IONotifyController" );
    REQUIRE( jret->get("isActive").convert<bool>() == true );
    REQUIRE( jret->get("lostMessages").convert<long>() == 0 );
}
// -----------------------------------------------------------------------------
static void check_get( const std::string& query )
{
    //   QUERY: /get?xxx
    //  REPLY:
    //  {"object":
    //    {"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
    //  "sensors":[
    //      {"name":"Input2_S","id":"2","calibration":{"cmax":0,"cmin":0,"precision":0,"rmax":0,"rmin":0},"dbignore":false,"default_val":0,"nchanges":0,"real_value":0,"tv_nsec":369597308,"tv_sec":1483733666,"type":"DI","value":0}
    //  ]}

    std::string s = shm->apiRequest(query);
    Poco::JSON::Parser parser;
    auto result = parser.parse(s);
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jarr = json->get("sensors").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jarr);

    Poco::JSON::Object::Ptr jret = jarr->getObject(0);
    REQUIRE(jret);

    REQUIRE( jret->get("id").convert<ObjectId>() == 122 );
    REQUIRE( jret->get("name").convert<std::string>() == "API_Sensor_AS" );
    REQUIRE( jret->get("type").convert<std::string>() == "AI" );
    REQUIRE( jret->get("value").convert<long>() == 10 );
    REQUIRE( jret->get("real_value").convert<long>() == 10 );
    REQUIRE( jret->get("default_val").convert<long>() == 10 );

    auto jcal = jret->get("calibration").extract<Poco::JSON::Object::Ptr>();
    REQUIRE( jcal->get("cmin").convert<long>() == 10 );
    REQUIRE( jcal->get("cmax").convert<long>() == 20 );
    REQUIRE( jcal->get("rmin").convert<long>() == 100 );
    REQUIRE( jcal->get("rmax").convert<long>() == 200 );
    REQUIRE( jcal->get("precision").convert<long>() == 1 );
}
// -----------------------------------------------------------------------------
static void check_set( const std::string& query )
{
    //  QUERY: /set?xxx=val&yyy=val2
    //  REPLY:
    //  {"object":
    //    {"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
    //   "errors":[]
    //  }

    std::string s = shm->apiRequest(query);
    Poco::JSON::Parser parser;
    auto result = parser.parse(s);
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jarr = json->get("errors").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jarr);

    // success (no errors)
    REQUIRE(jarr->empty());
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: /get]", "[restapi][get]")
{
    init_test();

    SECTION("getByName")
    {
        check_get("/get?API_Sensor_AS");
    }

    SECTION("getByID")
    {
        check_get("/get?122");
    }

    SECTION("BadFormat")
    {
        // Запрос без ответа
        // QUERY: /get
        // Ожидаемый формат ответа:
        // {"ecode":500,"error":"SharedMemory(request): 'get'. Unknown ID or Name. Use parameters: get?ID1,name2,ID3,..."}
        std::string s = shm->apiRequest("/get");
        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        REQUIRE( json->get("ecode").convert<int>() == 500 );
    }

    SECTION("NotFound")
    {
        // QUERY: /get?dummy
        // Ожидаемый формат ответа:
        //  {"ecode":500,"error":"SharedMemory(request): 'get'. Unknown ID or Name. Use parameters: get?ID1,name2,ID3,..."}

        std::string s = shm->apiRequest("/get?dummy");

        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);
        REQUIRE( json->get("ecode").convert<int>() == 500 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: /set]", "[restapi][set]")
{
    init_test();

    SECTION("setByName")
    {
        check_set("/set?API_Sensor_AS=11&API_Sensor2_AS=11");
        REQUIRE( shm->getValue(122) == 11 );
        REQUIRE( shm->getValue(123) == 11 );
    }

    SECTION("setByID")
    {
        check_set("/set?122=12&123=12");
        REQUIRE( shm->getValue(122) == 12 );
        REQUIRE( shm->getValue(123) == 12 );
    }

    SECTION("setByIDWithSupplier")
    {
        check_set("/set?supplier=TestProc1&122=5&123=5");
        REQUIRE( shm->getValue(122) == 5 );
        REQUIRE( shm->getValue(123) == 5 );
    }

    SECTION("BadFormat")
    {
        // Запрос без параметров
        // QUERY: /set
        // Ожидаемый формат ответа:
        // {"ecode":500,"error":"SharedMemory(request): 'set'. Unknown ID or Name. Use parameters: set?ID1=val1&name2=va;2&ID3=val3,..."}
        std::string s = shm->apiRequest("/set");
        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        REQUIRE( json->get("ecode").convert<int>() == 500 );
    }

    SECTION("NotFound")
    {
        // QUERY: /set?dummy=5
        // Ожидаемый формат ответа:
        //      {"object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
        //          "errors":[{"error":"not found","name":"dummy"}]}

        std::string s = shm->apiRequest("/set?dummy=5");
        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        auto jarr = json->get("errors").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(jarr);

        Poco::JSON::Object::Ptr jret = jarr->getObject(0);
        REQUIRE(jret);

        // просто проверяем что 'error' не пустой..
        REQUIRE( jret->get("error").convert<std::string>().empty() == false );
    }

    SECTION("NotFound some sensor")
    {
        // QUERY: /set?122=10&dummy=15
        // Ожидаемый формат ответа:
        //      {"object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
        //          "errors":[{"error":"not found","name":"dummy"}]}

        std::string s = shm->apiRequest("/set?122=10&dummy=15");

        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        auto jarr = json->get("errors").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(jarr);

        bool found_error = false;

        for( int i = 0; i < jarr->size(); i++ )
        {
            Poco::JSON::Object::Ptr jret = jarr->getObject(i);

            if( jret->get("name").convert<std::string>() == "dummy"
                    && jret->get("error").convert<std::string>().empty() == false )
            {
                found_error = true;
                break;
            }
        }

        REQUIRE(found_error);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: /sensors]", "[restapi][sensors]")
{
    init_test();

    // QUERY: /sensors?limit=1
    // Ожидаемый формат ответа:
    // {"count":1,
    //  "object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
    //  "sensors":[
    //      {"calibration":{"cmax":0,"cmin":0,"precision":0,"rmax":0,"rmin":0},"dbignore":false,"default_val":0,"id":15095,"name":"Sensor15095_S","nchanges":0,"real_value":0,"tv_nsec":735385587,"tv_sec":1483744698,"type":"AI","value":0}
    //  ],
    //  "size":5071}


    long lastID = 0;

    SECTION("limit")
    {
        std::string s = shm->apiRequest("/sensors?limit=2");
        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        REQUIRE( json->get("count").convert<int>() == 2 );

        auto jarr = json->get("sensors").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(jarr);

        Poco::JSON::Object::Ptr jret = jarr->getObject(0);
        REQUIRE(jret);

        Poco::JSON::Object::Ptr jret2 = jarr->getObject(1);
        REQUIRE(jret2);

        lastID = jret2->get("id").convert<long>();
    }

    SECTION("offset")
    {
        std::string s = shm->apiRequest("/sensors?offset=2&limit=2");
        Poco::JSON::Parser parser;
        auto result = parser.parse(s);
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);

        REQUIRE( json->get("count").convert<int>() == 2 );

        auto jarr = json->get("sensors").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(jarr);

        Poco::JSON::Object::Ptr jret = jarr->getObject(0);
        REQUIRE(jret);

        long firstID = jret->get("id").convert<long>();
        REQUIRE( firstID != lastID );

        Poco::JSON::Object::Ptr jret2 = jarr->getObject(1);
        REQUIRE(jret2);


    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: /consumers]", "[restapi][consumers]")
{
    init_test();

    REQUIRE_NOTHROW( shm->askSensor(aid, UniversalIO::UIONotify, testOID) );

    // QUERY: /consumers
    // Ожидаемый формат ответа:
    //  {"object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
    //  "sensors":[
    //      {"consumers":[
    //          {"attempt":4,"id":6000,"lostEvents":4,"name":"TestProc","node":3000,"node_name":"localhost","smCount":0}
    //      ],
    //      "sensor":{"id":10,"name":"AI_AS"}}
    //      {"consumers":[
    //          {"attempt":10,"id":6000,"lostEvents":0,"name":"TestProc","node":3000,"node_name":"localhost","smCount":0}
    //      ],
    //      "sensor":{"id":1,"name":"Input1_S"}},
    //  ]}


    std::string s = shm->apiRequest("/consumers");
    Poco::JSON::Parser parser;
    auto result = parser.parse(s);
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jarr = json->get("sensors").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jarr);

    auto jret = jarr->getObject(0);
    REQUIRE(jret);

    auto sens = jret->get("sensor").extract<Poco::JSON::Object::Ptr>();
    REQUIRE(sens);

    REQUIRE( sens->get("id").convert<ObjectId>() == aid );
    REQUIRE( sens->get("name").convert<std::string>() == aidName );

    auto cons = jret->get("consumers").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(cons);
    REQUIRE( cons->size() == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: /lost]", "[restapi][lost]")
{
    init_test();

    // QUERY: /lost
    // Ожидаемый формат ответа:
    //  {"lost consumers":[
    //      ...
    //  ],
    //   "object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"}
    //  }

    // Сперва имитируем зазачика (который "исчезнет").
    const ObjectId myID = 6013; // TestProc2
    const ObjectId sid = 122; // test sensor

    shm->askSensor(sid, UniversalIO::UIONotify, myID );

    // имитируем изменения
    for( size_t i = 200; i < 220; i++ )
        shm->setValue(sid, i);

    // проверяем список "потерянных"
    std::string s = shm->apiRequest("/lost");
    Poco::JSON::Parser parser;
    auto result = parser.parse(s);
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jarr = json->get("lost consumers").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jarr);

    auto jret = jarr->getObject(0);
    REQUIRE(jret);

    REQUIRE( jret->get("id").convert<ObjectId>() == myID );
}
// -----------------------------------------------------------------------------
#endif // ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
