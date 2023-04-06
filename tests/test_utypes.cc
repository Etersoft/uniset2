#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include <limits>
#include <iomanip>
#include <cstdint>
#include "Configuration.h"
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
#pragma GCC diagnostic ignored -Wwrite-strings
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: uni_atoi", "[utypes][uni_atoi]" )
{
    SECTION("int")
    {
        REQUIRE( uni_atoi("100") == 100 );
        REQUIRE( uni_atoi("-100") == -100 );
        REQUIRE( uni_atoi("0") == 0 );
        REQUIRE( uni_atoi("-0") == 0 );

        ostringstream imax;
        imax << std::numeric_limits<int>::max();
        REQUIRE( uni_atoi(imax.str()) == std::numeric_limits<int>::max() );

        ostringstream imin;
        imin << std::numeric_limits<int>::min();
        REQUIRE( uni_atoi(imin.str()) == std::numeric_limits<int>::min() );
    }

    SECTION("unsigned int")
    {
        ostringstream imax;
        imax << std::numeric_limits<unsigned int>::max();

        unsigned int uint_max_val = (unsigned int)uni_atoi(imax.str());

        REQUIRE(  uint_max_val == std::numeric_limits<unsigned int>::max() );

        ostringstream imin;
        imax << std::numeric_limits<int>::min();

        unsigned int uint_min_val = (unsigned int)uni_atoi(imin.str()); // "0"?

        REQUIRE( uint_min_val == std::numeric_limits<unsigned int>::min() );
    }

    SECTION("hex")
    {
        REQUIRE( uni_atoi("0xff") == 0xff );
        REQUIRE( uni_atoi("0xffff") == 0xffff );
        REQUIRE( uni_atoi("0x0") == 0 );
        REQUIRE( (uint32_t)uni_atoi("0xffffffff") == 0xffffffff );
        REQUIRE( uni_atoi("0xfffffff8") == 0xfffffff8 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: uni_strdup", "[utypes][uni_strdup]" )
{
    SECTION("uni_strdup string")
    {
        string str("Test string");
        const char* cp = uni_strdup(str);
        string str2(cp);
        REQUIRE( str == str2 );
        delete[] cp;
    }

    SECTION("uni_strdup char*")
    {
        const char* str = "Test string";
        char* str2 = uni_strdup(str);
        REQUIRE( !strcmp(str, str2) );
        delete[] str2;
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: explode", "[utypes][explode]" )
{
    const std::string str1("id1/wed/wedwed/");

    auto t1 = uniset::explode_str(str1, '/');
    CHECK( t1.size() == 3 );

    auto t2 = uniset::explode_str(str1, '.');
    CHECK( t2.size() == 1 );

    const std::string str2("id1/wed/wedwed/f");

    auto t3 = uniset::explode_str(str2, '/');
    CHECK( t3.size() == 4 );

    const std::string str3("/id1/wed/wedwed/");

    auto t4 = uniset::explode_str(str3, '/');
    CHECK( t4.size() == 3 );


    const std::string str4("");
    auto t5 = uniset::explode_str(str4, '/');
    CHECK( t5.size() == 0 );

    const std::string str5("/");
    auto t6 = uniset::explode_str(str5, '/');
    CHECK( t6.size() == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getSInfoList", "[utypes][getsinfo]" )
{
    const std::string str1("Input4_S@node2,Input1_S,5,5@node3,6@1001");

    auto t1 = uniset::getSInfoList(str1, uniset::uniset_conf());

    CHECK( t1.size() == 5 );

    vector<uniset::ParamSInfo> v(t1.begin(), t1.end());

    REQUIRE( v[0].si.id == 141 );
    REQUIRE( v[0].si.node == 1001 );

    REQUIRE( v[1].si.id == 1 );
    REQUIRE( v[1].si.node == DefaultObjectId );

    REQUIRE( v[2].si.id == 5 );
    REQUIRE( v[2].si.node == DefaultObjectId );

    REQUIRE( v[3].si.id == 5 );
    REQUIRE( v[3].si.node == 1002 );

    REQUIRE( v[4].si.id == 6 );
    REQUIRE( v[4].si.node == 1001 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getObjectsList", "[utypes][getolist]" )
{
    const std::string str1("TestProc@node2,TestProc,102,102@node3,103@1001");

    auto t1 = uniset::getObjectsList(str1);

    CHECK( t1.size() == 5 );

    vector<uniset::ConsumerInfo> v(t1.begin(), t1.end());

    REQUIRE( v[0].id == 100 );
    REQUIRE( v[0].node == 1001 );

    REQUIRE( v[1].id == 100 );
    REQUIRE( v[1].node == DefaultObjectId );

    REQUIRE( v[2].id == 102 );
    REQUIRE( v[2].node == DefaultObjectId );

    REQUIRE( v[3].id == 102 );
    REQUIRE( v[3].node == 1002 );

    REQUIRE( v[4].id == 103 );
    REQUIRE( v[4].node == 1001 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: replace_all", "[utypes][replace_all]" )
{
    const std::string str1("Text %p test text %p");

    std::string res = uniset::replace_all(str1, "%p", "my");
    REQUIRE( res == "Text my test text my" );

    const std::string str2("Text %rlong test text %rlong");
    res = uniset::replace_all(str2, "%rlong", "2");
    REQUIRE( res == "Text 2 test text 2" );

    res = uniset::replace_all(str2, "", "my");
    REQUIRE( res == str2 );

    res = uniset::replace_all(str2, "not found", "my");
    REQUIRE( res == str2 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: timespec comapre", "[utypes][timespec]" )
{
    timespec t1 = { 10, 300 };
    timespec t2 = { 10, 90 };

    REQUIRE( t1 != t2 );
    REQUIRE_FALSE( t1 == t2 );

    timespec t3 = { 20, 20 };
    timespec t4 = { 20, 20 };

    REQUIRE_FALSE( t3 != t4 );
    REQUIRE( t3 == t4 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: fcalibrate", "[utypes][fcalibrate]" )
{
    //  float fcalibrate(float raw, float rawMin, float rawMax, float calMin, float calMax, bool limit = true );

    REQUIRE( fcalibrate(0.5, 0.1, 1.0, 100.0, 1000.0, true) == 500.0 );
    REQUIRE( fcalibrate(10.0, 0.1, 1.0, 100.0, 1000.0, true) == 1000.0 );
    REQUIRE( fcalibrate(10.0, 0.1, 1.0, 100.0, 1000.0, false) == 10000.0 );

    REQUIRE( fcalibrate(0.0, 0.1, 1.0, 100.0, 1000.0, true) == 100.0 );
    REQUIRE( fcalibrate(0.0, 0.1, 1.0, 100.0, 1000.0, false) == 0.0 );

    REQUIRE( fcalibrate(-10.0, 0.1, 1.0, 100.0, 1000.0, true) == 100.0 );
    REQUIRE( fcalibrate(-10.0, 0.1, 1.0, 100.0, 1000.0, false) == -10000.0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: lcalibrate", "[utypes][lcalibrate]" )
{
    // long lcalibrate(long raw, long rawMin, long rawMax, long calMin, long calMax, bool limit = true );

    REQUIRE( lcalibrate(5, 1, 10, 100, 1000, true) == 500 );
    REQUIRE( lcalibrate(5, 1, 10, 100, 1000, false) == 500 );

    REQUIRE( lcalibrate(0, 1, 10, 100, 1000, true) == 100 );
    REQUIRE( lcalibrate(0, 1, 10, 100, 1000, false) == 0 );

    REQUIRE( lcalibrate(100, 1, 10, 100, 1000, true) == 1000 );
    REQUIRE( lcalibrate(100, 1, 10, 100, 1000, false) == 10000 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: setinregion", "[utypes][setinregion]" )
{
    //  long setinregion(long raw, long rawMin, long rawMax);

    REQUIRE( setinregion(5, 1, 10) == 5 );
    REQUIRE( setinregion(1, 1, 10) == 1 );
    REQUIRE( setinregion(10, 1, 10) == 10 );
    REQUIRE( setinregion(0, 1, 10) == 1 );
    REQUIRE( setinregion(100, 1, 10) == 10 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: setoutregion", "[utypes][setoutregion]" )
{
    //  long setoutregion(long raw, long calMin, long calMax);

    REQUIRE( setoutregion(5, 1, 10) == 1 );
    REQUIRE( setoutregion(1, 1, 10) == 1 );
    REQUIRE( setoutregion(10, 1, 10) == 10 );
    REQUIRE( setoutregion(100, 1, 10) == 100 );
    REQUIRE( setoutregion(0, 1, 10) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: file_exist", "[utypes][file_exist]" )
{
    CHECK_FALSE( file_exist("uknown_file") );

    auto conf = uniset_conf();
    REQUIRE( conf );
    CHECK( file_exist(conf->getConfFileName()) );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: directory_exist", "[utypes][directory_exist]" )
{
    CHECK_FALSE( directory_exist("uknown_dir") );
    CHECK( directory_exist("/") ); // linux only
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: check_filter", "[utypes][check_filter]" )
{
    // bool check_filter( UniXML::iterator& it, const std::string& f_prop, const std::string& f_val = "" ) noexcept;
    auto xml = uniset_conf()->getConfXML();

    xmlNode* xnode = xml->findNode(xml->getFirstNode(), "test_check_filter");
    REQUIRE(xnode);

    UniXML::iterator it(xnode);

    CHECK( check_filter(it, "one_prop", "") );
    CHECK_FALSE( check_filter(it, "empty_prop", "") );
    CHECK( check_filter(it, "fprop", "fvalue") );
    CHECK_FALSE( check_filter(it, "fprop", "badvalue") );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: findArgParam", "[utypes][findArgParam]" )
{
    // int findArgParam( const std::string& name, int _argc, const char* const* _argv )
    int argc = 5;
    char* argv[] = {"progname", "--param1", "val", "--param2", "val2"};

    REQUIRE( findArgParam("--param1", argc, argv) == 1 );
    REQUIRE( findArgParam("--param2", argc, argv) == 3 );
    REQUIRE( findArgParam("--unknownparam", argc, argv) == -1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getArgParam", "[utypes][getArgParam]" )
{
    //  getArgParam( const std::string& name,
    //                                     int _argc, const char* const* _argv,
    //                                     const std::string& defval = "" ) noexcept
    int argc = 5;
    char* argv[] = {"progname", "--param1", "val", "--param2", "val2"};

    REQUIRE( getArgParam("--param1", argc, argv) == "val" );
    REQUIRE( getArgParam("--param2", argc, argv) == "val2" );
    REQUIRE( getArgParam("--unknownparam", argc, argv, "val3") == "val3" );
    REQUIRE( getArgParam("--unknownparam2", argc, argv) == "" );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getArg2Param", "[utypes][getArg2Param]" )
{
    //  inline std::string getArg2Param(const std::string& name,
    //                                  int _argc, const char* const* _argv,
    //                                  const std::string& defval, const std::string& defval2 = "") noexcept
    int argc = 5;
    char* argv[] = {"progname", "--param1", "val", "--param2", "val2"};

    REQUIRE( getArg2Param("--param1", argc, argv, "") == "val" );
    REQUIRE( getArg2Param("--param2", argc, argv, "") == "val2" );
    REQUIRE( getArg2Param("--unknownparam", argc, argv, "val3") == "val3" );
    REQUIRE( getArg2Param("--unknownparam2", argc, argv, "", "val4") == "val4" );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getArgInt", "[utypes][getArgInt]" )
{
    //  inline int getArgInt( const std::string& name,
    //                    int _argc, const char* const* _argv,
    //                    const std::string& defval = "" ) noexcept
    int argc = 5;
    char* argv[] = {"progname", "--param1", "1", "--param2", "text"};

    REQUIRE( getArgInt("--param1", argc, argv) == 1 );
    REQUIRE( getArgInt("--param2", argc, argv) == 0 );
    REQUIRE( getArgInt("--unknownparam", argc, argv, "3") == 3 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getArgPInt", "[utypes][getArgPInt]" )
{
    //  inline int getArgPInt( const std::string& name,
    //                         int _argc, const char* const* _argv,
    //                         const std::string& strdefval, int defval ) noexcept
    int argc = 5;
    char* argv[] = {"progname", "--param1", "1", "--param2", "text"};

    REQUIRE( getArgPInt("--param1", argc, argv, "", 0) == 1 );
    REQUIRE( getArgPInt("--param2", argc, argv, "", 0) == 0 );
    REQUIRE( getArgPInt("--unknownparam", argc, argv, "3", 0) == 3 );
    REQUIRE( getArgPInt("--unknownparam", argc, argv, "", 42) == 42 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: is_digit", "[utypes][is_digit]" )
{
    // bool is_digit( const std::string& s ) noexcept;
    CHECK( is_digit("1") );
    CHECK( is_digit("100") );
    CHECK( is_digit("0") );
    CHECK_FALSE( is_digit("-1") );
    CHECK_FALSE( is_digit("10 000") );
    CHECK_FALSE( is_digit("100.0") );
    CHECK_FALSE( is_digit("a") );
    CHECK_FALSE( is_digit("'") );
    CHECK_FALSE( is_digit(";") );
    CHECK_FALSE( is_digit("") );
    CHECK_FALSE( is_digit("abs10as") );
    CHECK_FALSE( is_digit("10a") );
    CHECK_FALSE( is_digit("a10") );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getIOType", "[utypes][getIOType]" )
{
    // UniversalIO::IOType getIOType( const std::string& s ) noexcept;
    REQUIRE( getIOType("DI") == UniversalIO::DI );
    REQUIRE( getIOType("di") == UniversalIO::DI );
    REQUIRE( getIOType("DO") == UniversalIO::DO );
    REQUIRE( getIOType("do") == UniversalIO::DO );
    REQUIRE( getIOType("AI") == UniversalIO::AI );
    REQUIRE( getIOType("ai") == UniversalIO::AI );
    REQUIRE( getIOType("AO") == UniversalIO::AO );
    REQUIRE( getIOType("ao") == UniversalIO::AO );

    REQUIRE( getIOType("a") == UniversalIO::UnknownIOType );
    REQUIRE( getIOType("d") == UniversalIO::UnknownIOType );
    REQUIRE( getIOType("") == UniversalIO::UnknownIOType );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: ios_fmt_restorer", "[utypes][ios_fmt_restorer]" )
{
    std::ostringstream s;

    int value = 5;
    s << setw(2) << value;
    REQUIRE( s.str() == " 5" );

    {
        s.str("");

        uniset::ios_fmt_restorer l(s);
        s << std::setfill('0') << setw(2) << value;
        REQUIRE( s.str() == "05" );
    }

    {
        s.str("");

        uniset::ios_fmt_restorer l(s);
        s.setf(ios::left, ios::adjustfield);

        s << setw(2) << value;
        REQUIRE( s.str() == "5 " );
    }

    s.str("");
    s << setw(2) << value;
    REQUIRE( s.str() == " 5" );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: hash64", "[utypes][hash64]" )
{
    REQUIRE( uniset::hash64("test") == uint64_t(17703940110308125106) );
    REQUIRE( uniset::hash64("test2") == uint64_t(11165864767333097451) );
    REQUIRE( uniset::hash64("2tset") == uint64_t(15246161741804761271) );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: hash32", "[utypes][hash32]" )
{
    REQUIRE( sizeof(uniset::ObjectId) == sizeof(uint32_t) );
    REQUIRE( uniset::hash32("test") == uint32_t(403862830) );
    REQUIRE( uniset::hash32("test2") == uint32_t(2011244668) );
    REQUIRE( uniset::hash32("2tset") == uint32_t(3437323062) );
    REQUIRE( uniset::hash32("ttt1") != uniset::hash32("1ttt") );
    REQUIRE( uniset::hash32("ta") != uniset::hash32("at") );
    REQUIRE( uniset::hash32("DefaultObjectId") == 1920521126 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: key", "[utypes][key]" )
{
    REQUIRE( uniset::key(100, 100) == uint64_t(12423972911335977860) );
    REQUIRE( uniset::key(101, 100) == uint64_t(793928870581810946) );
    REQUIRE( uniset::key(100, 101) == uint64_t(2907929044583608809) );
    // max int
    REQUIRE( uniset::key(2147483647, 2147483647) == uint64_t(13802851885616383566) );
    REQUIRE( uniset::key(2147483647, 2147483646) == uint64_t(826978890373207942) );
    REQUIRE( uniset::key(2147483646, 2147483647) == uint64_t(18348137753129063869) );
}
// -----------------------------------------------------------------------------
