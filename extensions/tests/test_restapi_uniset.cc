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
#endif // ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
