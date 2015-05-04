#include <catch.hpp>

#include <future>
#include <time.h>
#include "LProcessor.h"
#include "UniSetTypes.h"
#include "Schema.h"
#include "TDelay.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;


static const std::string lp_schema = "lp_schema.xml";
// -----------------------------------------------------------------------------
bool run_lproc( std::shared_ptr<LProcessor> lp )
{
	lp->execute();
	return true;
}
// -----------------------------------------------------------------------------
class LPRunner
{
	public:
		LPRunner()
		{
			lp = make_shared<LProcessor>();
			lp->open(lp_schema);
			res = std::async(std::launch::async, run_lproc, lp);
		}

		~LPRunner()
		{
			if( lp )
			{
				lp->terminate();
				res.get();
			}
		}

		inline std::shared_ptr<LProcessor> get()
		{
			return lp;
		}

	private:
		shared_ptr<LProcessor> lp;
		std::future<bool> res;
};
// -----------------------------------------------------------------------------
void InitTest()
{
	auto conf = uniset_conf();
	CHECK( conf != nullptr );

	if( !ui )
	{
		ui = make_shared<UInterface>();
		CHECK( ui->getObjectIndex() != nullptr );
		CHECK( ui->getConf() == conf );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: elements", "[LogicProcessor][elements]")
{
	SECTION( "TOR" )
	{
		TOR e("1", 2); // элемент на два входа..
		REQUIRE( e.getOut() == 0 );
		e.setIn(1, true);
		CHECK( e.getOut() );
		e.setIn(2, true);
		CHECK( e.getOut() );
		e.setIn(1, false);
		CHECK( e.getOut() );
		e.setIn(2, false);
		CHECK_FALSE( e.getOut() );

		e.setIn(3, true); // несуществующий вход..
		CHECK_FALSE( e.getOut() );
	}
	SECTION( "TAND" )
	{
		TAND e("1", 2); // элемент на два входа..
		REQUIRE( e.getOut() == 0 );
		e.setIn(1, true);
		CHECK_FALSE( e.getOut() );
		e.setIn(2, true);
		CHECK( e.getOut() );
		e.setIn(1, false);
		CHECK_FALSE( e.getOut() );
		e.setIn(2, false);
		CHECK_FALSE( e.getOut() );

		e.setIn(3, true); // несуществующий вход..
		CHECK_FALSE( e.getOut() );
	}
	SECTION( "TNOT" )
	{
		TNOT e("1", false);
		CHECK_FALSE( e.getOut() );
		e.setIn(1, true);
		CHECK_FALSE( e.getOut() );
		e.setIn(1, false);
		CHECK( e.getOut() );

		// other constructor
		TNOT e1("1", true);
		CHECK( e1.getOut() );
		e1.setIn(1, true);
		CHECK_FALSE( e1.getOut() );
		e1.setIn(1, false);
		CHECK( e1.getOut() );
	}

	SECTION( "TDelay" )
	{
		TDelay e("1", 50, 1);
		CHECK_FALSE( e.getOut() );

		// ON DELAY
		e.setIn(1, true);
		CHECK_FALSE( e.getOut() );
		msleep(60);
		e.tick();
		CHECK( e.getOut() );

		msleep(60);
		e.tick();
		CHECK( e.getOut() );

		// OFF DELAY
		e.setIn(1, false);
		CHECK_FALSE( e.getOut() );

		// delay 0 msek..
		e.setDelay(0);
		e.setIn(1, true);
		CHECK( e.getOut() );

		// delay < 0 === 0
		e.setIn(1, false);
		e.setDelay(-10);
		e.setIn(1, true);
		CHECK( e.getOut() );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: schema", "[LogicProcessor][schema]")
{
	SchemaXML sch;
	sch.read("schema.xml");

	CHECK( !sch.empty() );
	REQUIRE( sch.size() == 6 );

	REQUIRE( sch.find("1") != nullptr );
	REQUIRE( sch.find("2") != nullptr );
	REQUIRE( sch.find("3") != nullptr );
	REQUIRE( sch.find("4") != nullptr );
	REQUIRE( sch.find("5") != nullptr );
	REQUIRE( sch.find("6") != nullptr );

	REQUIRE( sch.findOut("TestMode_S") != nullptr );

	auto e = sch.find("6");
	sch.remove(e);
	CHECK( sch.find("6") == nullptr );
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: lp", "[LogicProcessor][logic]")
{
	InitTest();
	LPRunner p;
	auto lp = p.get();

	auto sch = lp->getSchema();

	CHECK( sch != nullptr );

	auto e1 = sch->find("1");
	REQUIRE( e1 != nullptr );

	auto e2 = sch->find("2");
	REQUIRE( e2 != nullptr );

	auto e3 = sch->find("3");
	REQUIRE( e3 != nullptr );

	auto e4 = sch->find("4");
	REQUIRE( e4 != nullptr );

	auto e5 = sch->find("5");
	REQUIRE( e5 != nullptr );

	auto e6 = sch->find("6");
	REQUIRE( e6 != nullptr );

	CHECK_FALSE( e1->getOut() );
	CHECK_FALSE( e2->getOut() );
	CHECK_FALSE( e3->getOut() );
	CHECK_FALSE( e4->getOut() );
	CHECK_FALSE( e5->getOut() );
	CHECK_FALSE( e6->getOut() );

	// e1
	ui->setValue(500, 1);
	msleep(lp->getSleepTime() + 10);
	CHECK( e1->getOut() );
	CHECK_FALSE( e3->getOut() );

	ui->setValue(500, 0);
	msleep(lp->getSleepTime() + 10);
	CHECK_FALSE( e1->getOut() );
	CHECK_FALSE( e3->getOut() );

	ui->setValue(501, 1);
	msleep(lp->getSleepTime() + 10);
	CHECK( e1->getOut() );
	CHECK_FALSE( e3->getOut() );


	// e4
	CHECK_FALSE( e4->getOut() );
	ui->setValue(504, 1);
	msleep(lp->getSleepTime() + 10);
	CHECK( e4->getOut() );
	// e5
	CHECK( e5->getOut() );

	ui->setValue(504, 0);
	msleep(lp->getSleepTime() + 10);
	CHECK_FALSE( e4->getOut() );
	// e5
	CHECK_FALSE( e5->getOut() );

	// e2
	ui->setValue(503, 1);
	msleep(lp->getSleepTime() + 10);
	CHECK( e2->getOut() );

	// e3
	CHECK( e3->getOut() );

	// e4
	CHECK( e4->getOut() );
	// e5
	CHECK( e5->getOut() );

	// e6
	CHECK_FALSE( e6->getOut() );
	msleep(1000);
	CHECK_FALSE( e6->getOut() );
	msleep(2100);
	CHECK( e6->getOut() );
}
// -----------------------------------------------------------------------------
