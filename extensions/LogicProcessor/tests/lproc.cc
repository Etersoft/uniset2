#include <catch.hpp>

#include <future>
#include <time.h>
#include "LProcessor.h"
#include "UniSetTypes.h"
#include "Schema.h"
#include "TDelay.h"
#include "TA2D.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;


static const std::string lp_schema = "lp_schema.xml";
static const std::string schema2 = "schema2.xml";
static const std::string schema3 = "schema3.xml";
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

        LPRunner() = delete;

        LPRunner(const std::string& schm)
        {
            lp = make_shared<LProcessor>("", nullptr);
            lp->open(schm);
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
    }

    SECTION( "TA2D" )
    {
        TA2D e("1", 10);
        REQUIRE_FALSE( e.getOut() );

        e.setIn(1, 5);
        REQUIRE_FALSE( e.getOut() );

        e.setIn(1, 10);
        REQUIRE( e.getOut() );


        e.setIn(1, 9);
        REQUIRE_FALSE( e.getOut() );

        e.setFilterValue(5);
        REQUIRE_FALSE( e.getOut() );

        e.setIn(1, 5);
        REQUIRE( e.getOut() );

        e.setIn(1, 6);
        REQUIRE_FALSE( e.getOut() );
    }

    SECTION( "SEL_R" )
    {
        TSEL_R e("1", false, 5, 7);
        CHECK( e.getOut() == 5 );
        e.setIn(1, true);
        CHECK( e.getOut() == 7 );
        e.setIn(1, false);
        REQUIRE( e.getOut() == 5 );

        // other constructor
        TSEL_R e1("2", true, 4, 6);
        CHECK( e1.getOut() == 6 );
        e1.setIn(1, false);
        CHECK( e1.getOut() == 4 );
        e1.setIn(1, true);
        CHECK( e1.getOut() == 6 );

        //change input variables
        TSEL_R e2("3", false, 3, 9);
        CHECK( e2.getOut() == 3 );
        e2.setIn(1, true);
        CHECK( e2.getOut() == 9 );
        e2.setIn(1, false);
        CHECK( e2.getOut() == 3 );
        e2.setIn(3, 10); // false inp
        CHECK( e2.getOut() == 10 );
        e2.setIn(2, 19); // true inp
        CHECK( e2.getOut() == 10 );
        e2.setIn(1, true);
        CHECK( e2.getOut() == 19 );
        e2.setIn(1, false);
        REQUIRE( e2.getOut() == 10 );
    }

    SECTION( "RS" )
    {
        TRS e("1", false);
        REQUIRE_FALSE( e.getOut() );
        e.setIn(1, false);
        e.setIn(2, false);
        REQUIRE_FALSE( e.getOut() );
        e.setIn(1, true);//set=1
        REQUIRE( e.getOut() );
        e.setIn(1, false);
        REQUIRE( e.getOut() );
        e.setIn(2, true);//reset
        REQUIRE_FALSE( e.getOut() );
        e.setIn(2, false);
        REQUIRE_FALSE( e.getOut() );

        // default true
        TRS e1("2", true);
        REQUIRE( e1.getOut() );
        e1.setIn(1, true);//set
        e1.setIn(2, false);
        REQUIRE( e1.getOut() );
        e1.setIn(1, false);
        REQUIRE( e1.getOut() );
        e1.setIn(2, true); //reset
        REQUIRE_FALSE( e1.getOut() );
        e1.setIn(2, false);
        REQUIRE_FALSE( e1.getOut() );

        // set input is dominante
        TRS e2("1", false );
        REQUIRE_FALSE( e2.getOut() );
        e2.setIn(1, true);//set
        REQUIRE( e2.getOut() );
        e2.setIn(2, true);//reset
        REQUIRE( e2.getOut() );
        e2.setIn(1, false);
        REQUIRE_FALSE( e2.getOut() );
        e2.setIn(2, false); //reset
        REQUIRE_FALSE( e2.getOut() );

        // reset inpiut is dominante
        TRS e3("1", true, /*dominantReset=*/true);
        REQUIRE( e3.getOut() );
        e3.setIn(1, true);//set
        REQUIRE( e3.getOut() );
        e3.setIn(2, true);//reset
        REQUIRE_FALSE( e3.getOut() );
        e3.setIn(2, false);
        REQUIRE( e3.getOut() );
        e3.setIn(1, false);
        REQUIRE( e3.getOut() );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: schema", "[LogicProcessor][schema]")
{
    SchemaXML sch;
    sch.read("schema.xml");

    CHECK( !sch.empty() );
    REQUIRE( sch.size() == 7 );

    REQUIRE( sch.find("1") != nullptr );
    REQUIRE( sch.find("2") != nullptr );
    REQUIRE( sch.find("3") != nullptr );
    REQUIRE( sch.find("4") != nullptr );
    REQUIRE( sch.find("5") != nullptr );
    REQUIRE( sch.find("6") != nullptr );
    REQUIRE( sch.find("7") != nullptr );

    REQUIRE( sch.findOut("TestMode_S") != nullptr );

    auto e = sch.find("6");
    sch.remove(e);
    CHECK( sch.find("6") == nullptr );
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: lp", "[LogicProcessor][logic]")
{
    InitTest();
    LPRunner p(lp_schema);
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

    auto e7 = sch->find("7");
    REQUIRE( e7 != nullptr );

    CHECK_FALSE( e1->getOut() );
    CHECK_FALSE( e2->getOut() );
    CHECK_FALSE( e3->getOut() );
    CHECK_FALSE( e4->getOut() );
    CHECK_FALSE( e5->getOut() );
    CHECK_FALSE( e6->getOut() );
    CHECK_FALSE( e7->getOut() );

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

    // e7 (Out2_S=507, AI_S=508)
    CHECK_FALSE( e7->getOut() );
    ui->setValue(508, 1);
    msleep(1000);
    REQUIRE_FALSE( e7->getOut() );
    REQUIRE_FALSE( ui->getValue(507) );
    ui->setValue(508, 6);
    msleep(1000);
    REQUIRE( e7->getOut() );
    msleep(1000);
    REQUIRE( ui->getValue(507) );
    ui->setValue(508, 7);
    msleep(1000);
    REQUIRE_FALSE( e7->getOut() );
    msleep(1000);
    REQUIRE_FALSE( ui->getValue(507) );
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: link with NOT", "[LogicProcessor][link][not]")
{
    SECTION( "FALSE" )
    {
        auto eNOT = make_shared<TNOT>("1",false);
        std::shared_ptr<Element> eAND = make_shared<TAND>("2",1); // элемент на один вход для проверки

        eNOT->addChildOut(eAND, 1);
        eNOT->setIn(1, false);
        CHECK( eAND->getOut() );
        eNOT->setIn(1, true);
        CHECK_FALSE( eAND->getOut() );
    }

    SECTION( "TRUE" )
    {
        auto eNOT = make_shared<TNOT>("1",true);
        std::shared_ptr<Element> eAND = make_shared<TAND>("2",1); // элемент на один вход для проверки

        eNOT->addChildOut(eAND, 1);
        eNOT->setIn(1, false);
        CHECK( eAND->getOut() );
        eNOT->setIn(1, true);
        CHECK_FALSE( eAND->getOut() );
    }

    SECTION( "LAST ELEMENT" )
    {
        std::shared_ptr<Element> eAND1 = make_shared<TAND>("1",1); // элемент на один вход для проверки
        std::shared_ptr<Element> eAND2 = make_shared<TAND>("2",1); // элемент на один вход для проверки
        std::shared_ptr<Element> eNOT = make_shared<TNOT>("3",false);

        eAND1->addChildOut(eAND2, 1);
        eAND2->addChildOut(eNOT, 1);

        eAND1->setIn(1, false);
        CHECK( eNOT->getOut() );
        eNOT->setIn(1, true);
        CHECK_FALSE( eNOT->getOut() );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: SEL_R", "[LogicProcessor][link][sel_r]")
{
    SECTION( "FALSE" )
    {
        std::shared_ptr<Element> eSEL_R1 = make_shared<TSEL_R>("1",false, 5, 7);
        std::shared_ptr<Element> eTA2D1 = make_shared<TA2D>("2", 5);
        std::shared_ptr<Element> eTA2D2 = make_shared<TA2D>("3", 7);

        //!Только добавляем выходы, но не значения!
        eSEL_R1->addChildOut(eTA2D1, 1);
        eSEL_R1->addChildOut(eTA2D2, 1);

        CHECK_FALSE( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );

        eSEL_R1->setIn(1, false);
        CHECK( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );

        eSEL_R1->setIn(1, true);
        CHECK( eTA2D2->getOut() );
        CHECK_FALSE( eTA2D1->getOut() );

        eSEL_R1->setIn(1, false);
        CHECK( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );
    }

    SECTION( "TRUE" )
    {
        std::shared_ptr<Element> eSEL_R1 = make_shared<TSEL_R>("1",true, 9, 2);
        std::shared_ptr<Element> eTA2D1 = make_shared<TA2D>("2", 9);
        std::shared_ptr<Element> eTA2D2 = make_shared<TA2D>("3", 2);

        eSEL_R1->addChildOut(eTA2D1, 1);
        eSEL_R1->addChildOut(eTA2D2, 1);

        CHECK_FALSE( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );

        eSEL_R1->setIn(1, true);
        CHECK( eTA2D2->getOut() );
        CHECK_FALSE( eTA2D1->getOut() );

        eSEL_R1->setIn(1, false);
        CHECK( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );

        eSEL_R1->setIn(1, true);
        CHECK( eTA2D2->getOut() );
        CHECK_FALSE( eTA2D1->getOut() );
    }

    SECTION( "CHANGE INPUT VARIABLES" )
    {
        std::shared_ptr<Element> eSEL_R1 = make_shared<TSEL_R>("1",true, 7, 3);
        std::shared_ptr<Element> eTA2D1 = make_shared<TA2D>("2", 4);
        std::shared_ptr<Element> eTA2D2 = make_shared<TA2D>("3", 5);

        eSEL_R1->addChildOut(eTA2D1, 1);
        eSEL_R1->addChildOut(eTA2D2, 1);

        CHECK_FALSE( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );

        eSEL_R1->setIn(1, true);
        CHECK_FALSE( eTA2D2->getOut() );
        CHECK_FALSE( eTA2D1->getOut() );

        eSEL_R1->setIn(1, false);
        CHECK_FALSE( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );

        eSEL_R1->setIn(2, 4);//true input
        eSEL_R1->setIn(3, 5);//false input
        CHECK_FALSE( eTA2D1->getOut() );
        CHECK( eTA2D2->getOut() );
        eSEL_R1->setIn(1, true);
        CHECK( eTA2D1->getOut() );
        CHECK_FALSE( eTA2D2->getOut() );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: RS", "[LogicProcessor][link][rs]")
{
    SECTION( "FALSE" )
    {
        std::shared_ptr<Element> eNOT1 = make_shared<TNOT>("1", false);
        std::shared_ptr<Element> eAND1 = make_shared<TAND>("2",1);
        std::shared_ptr<Element> eRS1 = make_shared<TRS>("3",false);

        eNOT1->addChildOut(eRS1, 1);//set
        eAND1->addChildOut(eRS1, 2);//reset

        eNOT1->setIn(1, true);
        CHECK_FALSE( eRS1->getOut() );

        eNOT1->setIn(1, false);
        CHECK( eRS1->getOut() );

        eNOT1->setIn(1, true);
        CHECK( eRS1->getOut() );

        eAND1->setIn(1, true);
        CHECK_FALSE( eRS1->getOut() );

        eAND1->setIn(1, false);
        CHECK_FALSE( eRS1->getOut() );
    }

    SECTION( "TRUE" )
    {
        std::shared_ptr<Element> eNOT1 = make_shared<TNOT>("1", true);
        std::shared_ptr<Element> eAND1 = make_shared<TAND>("2",1);
        std::shared_ptr<Element> eRS1 = make_shared<TRS>("3",false);

        eNOT1->addChildOut(eRS1, 1);//set
        eAND1->addChildOut(eRS1, 2);//reset

        eNOT1->setIn(1, false);
        CHECK( eRS1->getOut() );

        eNOT1->setIn(1, true);
        CHECK( eRS1->getOut() );

        eAND1->setIn(1, true);
        CHECK_FALSE( eRS1->getOut() );

        eAND1->setIn(1, false);
        CHECK_FALSE( eRS1->getOut() );
    }

    SECTION( "DOMINANT RESET" )
    {
        std::shared_ptr<Element> eNOT1 = make_shared<TNOT>("1", true);
        std::shared_ptr<Element> eAND1 = make_shared<TAND>("2",1);
        std::shared_ptr<Element> eRS1 = make_shared<TRS>("3",false, /*dominantReset=*/true);

        eNOT1->addChildOut(eRS1, 1);//set
        eAND1->addChildOut(eRS1, 2);//reset

        eAND1->setIn(1, true);
        CHECK_FALSE( eRS1->getOut() );

        eNOT1->setIn(1, false);
        CHECK_FALSE( eRS1->getOut() );

        eAND1->setIn(1, false);
        CHECK( eRS1->getOut() );

        eNOT1->setIn(1, true);
        CHECK( eRS1->getOut() );      
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: schema2", "[LogicProcessor][schema][not_rs_sel_r]")
{
    InitTest();
    LPRunner p(schema2);
    auto lp = p.get();

    auto sch = lp->getSchema();

    CHECK( sch != nullptr );

    CHECK_FALSE( sch->empty() );
    REQUIRE( sch->size() == 3 );

    auto e1 = sch->find("1");
    REQUIRE( e1 != nullptr );
    auto e2 = sch->find("2");
    REQUIRE( e2 != nullptr );
    auto e3 = sch->find("3");
    REQUIRE( e3 != nullptr );

    REQUIRE( sch->findOut("Out1_S") != nullptr );

    CHECK_FALSE( e1->getOut() );
    CHECK_FALSE( e2->getOut() );
    CHECK( e3->getOut() == 3 );

    //In1_S 500
    //In2_S 501
    //Out1_S 506
    ui->setValue(500, 1);
    ui->setValue(501, 0);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 3);

    ui->setValue(500, 0);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 7);
    ui->setValue(500, 1);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 7);

    ui->setValue(501, 1);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 3);
    ui->setValue(501, 0);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 3);
}
// -----------------------------------------------------------------------------
TEST_CASE("Logic processor: schema3", "[LogicProcessor][schema][not_rs_sel_r][params]")
{
    InitTest();
    LPRunner p(schema3);
    auto lp = p.get();

    auto sch = lp->getSchema();

    CHECK( sch != nullptr );

    CHECK_FALSE( sch->empty() );
    REQUIRE( sch->size() == 3 );

    auto e1 = sch->find("1");
    REQUIRE( e1 != nullptr );
    auto e2 = sch->find("2");
    REQUIRE( e2 != nullptr );
    auto e3 = sch->find("3");
    REQUIRE( e3 != nullptr );

    REQUIRE( sch->findOut("Out1_S") != nullptr );

    CHECK_FALSE( e1->getOut() );
    CHECK_FALSE( e2->getOut() );
    CHECK( e3->getOut() == 0 );

    //In1_S 500 !set
    //In2_S 501 reset
    //In3_S 502 true input var
    //In4_S 503 false input var
    //Out1_S 506 out
    ui->setValue(500, 1);//TRS set off
    ui->setValue(501, 0);//TRS reset off
    ui->setValue(502, 7);
    ui->setValue(503, 2);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 2);

    ui->setValue(500, 0);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 7);
    ui->setValue(501, 1); // reset is dominant
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 2);

    ui->setValue(501, 0);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 7);
    ui->setValue(500, 0);
    msleep(lp->getSleepTime() + 10);
    CHECK(ui->getValue(506) == 7);
}
// -----------------------------------------------------------------------------
