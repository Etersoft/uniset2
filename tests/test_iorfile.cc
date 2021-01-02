#include <catch.hpp>

#include <sstream>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "IORFile.h"
using namespace std;
using namespace uniset;

TEST_CASE("IORFile", "[iorfile][basic]" )
{
    CHECK( uniset_conf() != nullptr );

    ObjectId testID = 1;
    const std::string iorstr("testIORstring");
    IORFile ior;
    ior.setIOR(testID, iorstr);
    REQUIRE( ior.getIOR(testID) == iorstr );

    CHECK( file_exist(ior.getFileName(testID)) );

    ior.unlinkIOR(testID);
    CHECK_FALSE( file_exist(ior.getFileName(testID)) );
}
