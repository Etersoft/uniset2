#include <catch.hpp>

#include "PassiveTimer.h"
#include "UniSetTypes.h"
using namespace std;
// --------------------------------------------------------------------------
static std::atomic_int g_num = ATOMIC_VAR_INIT(0);
static std::mutex      g_mutex;
static std::shared_ptr<PassiveCondTimer> tmr;
// --------------------------------------------------------------------------
void thread_function( int msec )
{
    try
    {
        msleep(msec);
/*    
        std::chrono::milliseconds dura( msec );
        mstd::sleep_for( dura );
*/
        if( tmr )
            tmr->terminate();
    }
    catch( const std::exception& ex )
    {
        FAIL( ex.what() );
    }
}

TEST_CASE("PassiveCondTimer: wait", "[PassiveTimer][PassiveCondTimer]" ) 
{
    tmr = make_shared<PassiveCondTimer>();

    PassiveTimer ptTime;
    
    tmr->wait(300);
    
    REQUIRE( ptTime.getCurrent() >= 300 );
    REQUIRE( ptTime.getCurrent() <= 340 );

    tmr.reset();
}
// --------------------------------------------------------------------------
TEST_CASE("PassiveCondTimer: waitup", "[PassiveTimer][PassiveCondTimer]" ) 
{
    tmr = make_shared<PassiveCondTimer>();
    std::thread thr(thread_function,500);

    PassiveTimer ptTime;


    tmr->wait(UniSetTimer::WaitUpTime);

    REQUIRE( ptTime.getCurrent() >= 500 );
    REQUIRE( ptTime.getCurrent() <= 540 );

    thr.join();
    tmr.reset();
}
// --------------------------------------------------------------------------
