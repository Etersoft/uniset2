#include <catch.hpp>

#include "Exceptions.h"
#include "Extensions.h"
#include "UniXML.h"
#include "IOBase.h"

using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;

TEST_CASE("IOBase","[IOBase class tests]")
{
	CHECK( UniSetTypes::conf != 0 );

    SECTION("Default constructor (const data)")
    {
        IOBase ib;
        CHECK( ib.si.id == DefaultObjectId );
        CHECK( ib.si.node == DefaultObjectId );
        CHECK( ib.stype == UniversalIO::UnknownIOType );

        CHECK( ib.cal.minRaw == 0 );
        CHECK( ib.cal.maxRaw == 0 );
        CHECK( ib.cal.minCal == 0 );
        CHECK( ib.cal.maxCal == 0 );

        CHECK( ib.cdiagram == 0 );

        CHECK( ib.breaklim == 0 );
        CHECK( ib.value == 0 );
        CHECK( ib.craw == 0 );
        CHECK( ib.cprev == 0 );     /*!< предыдущее значение после калибровки */
        CHECK( ib.safety == 0 );    /*!< безопасное состояние при завершении процесса */
        CHECK( ib.defval == 0 );    /*!< состояние по умолчанию (при запуске) */

        CHECK( ib.nofilter == 0 );      /*!< отключение фильтра */
        CHECK_FALSE( ib.f_median );      /*!< признак использования медианного фильтра */
        CHECK_FALSE( ib.f_ls );          /*!< признак использования адаптивного фильтра по методу наименьших квадратов */
        CHECK_FALSE( ib.f_filter_iir );  /*!< признак использования рекурсивного фильтра */

        CHECK_FALSE( ib.ignore );    /*!< игнорировать при опросе */
        CHECK_FALSE( ib.invert );    /*!< инвертированная логика */
        CHECK_FALSE( ib.noprecision );

        CHECK_FALSE( ib.debounce_pause );
        CHECK_FALSE( ib.debounce_state );    /*!< значение для фильтра антидребезга */
        CHECK_FALSE( ib.ondelay_state );     /*!< значение для задержки включения */
        CHECK_FALSE( ib.offdelay_state );    /*!< значение для задержки отключения */

        // Порог
        REQUIRE( ib.t_ai == DefaultObjectId );
        CHECK_FALSE( ib.front ); // флаг работы по фронту
        REQUIRE( ib.front_type == IOBase::ftUnknown );
        CHECK_FALSE( ib.front_prev_state );
        CHECK_FALSE( ib.front_state );
    }

    SECTION("Init from xml")
    {
        xmlNode* cnode = conf->getNode("iobasetest");
        CHECK( cnode != NULL );
    	UniXML::iterator it(cnode);
   	    UInterface ui;
		ObjectId shmID = conf->getControllerID("SharedMemory");
		CHECK( shmID != DefaultObjectId );

		SMInterface shm(shmID,&ui,DefaultObjectId);
        IOBase ib;
        IOBase::initItem(&ib,it,&shm,"",false);
        CHECK( ib.si.id == 1 );
        CHECK( ib.si.node == conf->getLocalNode() );
        CHECK( ib.defval == -10 );
        CHECK( ib.cal.precision == 3 );
        CHECK( ib.cal.minRaw == -100 );
        CHECK( ib.cal.maxRaw == 100 );
        CHECK( ib.cal.minCal == 0 );
        CHECK( ib.cal.maxCal == 50 );
	}

    SECTION("Debounce function")
    {
	}

    SECTION("Delay function")
    {
	}

    SECTION("Front function")
    {
	}

    SECTION("'Channel break' function")
    {
	}
}
