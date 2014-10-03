#include <catch.hpp>

#include <time.h>
#include "LProcessor.h"
#include "UniSetTypes.h"
#include "Schema.h"
#include "TDelay.h"
using namespace std;
using namespace UniSetTypes;

TEST_CASE("Logic processor","[LogicProcessor]")
{
#if 0
	SECTION( "ShemaXML" )
	{
		SchemaXML sch;
		sch.read("schema.xml");
		CHECK( !sch.empty() );
	}
#endif
	SECTION( "TOR" )
	{
		TOR e("1",2); // элемент на два входа..
		REQUIRE( e.getOut() == 0 );
		e.setIn(1,true);
		CHECK( e.getOut() );
		e.setIn(2,true);
		CHECK( e.getOut() );
		e.setIn(1,false);
		CHECK( e.getOut() );
		e.setIn(2,false);
		CHECK_FALSE( e.getOut() );

		e.setIn(3,true); // несуществующий вход..
		CHECK_FALSE( e.getOut() );
	}
	SECTION( "TAND" )
	{
		TAND e("1",2); // элемент на два входа..
		REQUIRE( e.getOut() == 0 );
		e.setIn(1,true);
		CHECK_FALSE( e.getOut() );
		e.setIn(2,true);
		CHECK( e.getOut() );
		e.setIn(1,false);
		CHECK_FALSE( e.getOut() );
		e.setIn(2,false);
		CHECK_FALSE( e.getOut() );

		e.setIn(3,true); // несуществующий вход..
		CHECK_FALSE( e.getOut() );
	}
	SECTION( "TNOT" )
	{
		TNOT e("1",false);
		CHECK_FALSE( e.getOut() );
		e.setIn(1,true);
		CHECK_FALSE( e.getOut() );
		e.setIn(1,false);
		CHECK( e.getOut() );

		// other constructor
		TNOT e1("1",true);
		CHECK( e1.getOut() );
		e1.setIn(1,true);
		CHECK_FALSE( e1.getOut() );
		e1.setIn(1,false);
		CHECK( e1.getOut() );
	}

	SECTION( "TDelay" )
	{
		TDelay e("1",50,1);
		CHECK_FALSE( e.getOut() );

		// ON DELAY
		e.setIn(1,true);
		CHECK_FALSE( e.getOut() );
		msleep(60);
		e.tick();
		CHECK( e.getOut() );

		msleep(60);
		e.tick();
		CHECK( e.getOut() );

		// OFF DELAY	
		e.setIn(1,false);
		CHECK_FALSE( e.getOut() );

		// delay 0 msek..
		e.setDelay(0);
		e.setIn(1,true);
		CHECK( e.getOut() );
		
		// delay < 0 === 0
		e.setIn(1,false);
		e.setDelay(-10);
		e.setIn(1,true);
		CHECK( e.getOut() );
	}
}
