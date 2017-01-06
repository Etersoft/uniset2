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
// -----------------------------------------------------------------------------
static void init_test()
{
	shm = smiInstance();
	REQUIRE( shm != nullptr );
	REQUIRE( uniset_conf() != nullptr );
}
// -----------------------------------------------------------------------------
TEST_CASE("[REST API: conf]", "[restapi][conf]")
{
	init_test();

	std::string s = shm->apiRequest("/conf/get?2,Input5_S&params=iotype");
	Poco::JSON::Parser parser;
	auto result = parser.parse(s);

	// Ожидаемый формат ответа:
	// {"conf": [
//      {"id":"2","iotype":"DI","mbaddr":"0x01","mbfunc":"0x06","mbreg":"0x02","mbtype":"rtu","name":"Input2_S","nbit":"11","priority":"Medium","rs":"4","textname":"Команда 2"}
//   	],
//	    "object":
//	         {"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"}
//	   }
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
	//	 QUERY: /get?xxx
	//  REPLY:
	//	{"object":
	//	  {"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
	//	"sensors":[
	//	    {"name":"Input2_S","id":"2","calibration":{"cmax":0,"cmin":0,"precision":0,"rmax":0,"rmin":0},"dbignore":false,"default_val":0,"nchanges":0,"real_value":0,"tv_nsec":369597308,"tv_sec":1483733666,"type":"DI","value":0}
	//	]}

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
//		{"object":{"id":5003,"isActive":true,"lostMessages":0,"maxSizeOfMessageQueue":1000,"msgCount":0,"name":"SharedMemory","objectType":"IONotifyController"},
//			"sensors":[{"error":"Sensor not found","name":"dummy"}]}

		std::string s = shm->apiRequest("/get?dummy");
		Poco::JSON::Parser parser;
		auto result = parser.parse(s);
		Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
		REQUIRE(json);

		auto jarr = json->get("sensors").extract<Poco::JSON::Array::Ptr>();
		REQUIRE(jarr);

		Poco::JSON::Object::Ptr jret = jarr->getObject(0);
		REQUIRE(jret);

		// просто проверем что 'error' не пустой..
		REQUIRE( jret->get("error").convert<std::string>().empty() == false );
	}
}
// -----------------------------------------------------------------------------
//if( req == "sensors"
//if( req == "consumers" )
//if( req == "lost"
// -----------------------------------------------------------------------------
#endif // ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
