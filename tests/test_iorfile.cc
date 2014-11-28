#include <catch.hpp>

#include <sstream>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "IORFile.h"
using namespace std;
using namespace UniSetTypes;

TEST_CASE("IORFile", "[iorfile][basic]" ) 
{
	CHECK( uniset_conf()!=nullptr );

	ObjectId testID = 1;
	const std::string iorstr("testIORstring");
	IORFile ior;
	ior.setIOR(testID,iorstr);
	REQUIRE( ior.getIOR(testID) == iorstr );

	// здесь выносим "наружу" немного "внутренней кухни" IORFile,
	// т.к. снаружи не известно как формируется "название файла"
	ostringstream s;
	s << uniset_conf()->getLockDir() << testID;
	CHECK( file_exist(s.str()) );

	ior.unlinkIOR(testID);
	CHECK_FALSE( file_exist(s.str()) );
}
