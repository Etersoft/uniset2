#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "Exceptions.h"
#include "Extensions.h"
#include "UniXML.h"
#include "IOBase.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: default constructor", "[iobase][extensions]")
{
	CHECK( uniset_conf() != nullptr );

	IOBase ib;
	CHECK( ib.si.id == DefaultObjectId );
	CHECK( ib.si.node == DefaultObjectId );
	CHECK( ib.stype == UniversalIO::UnknownIOType );

	CHECK( ib.cal.minRaw == 0 );
	CHECK( ib.cal.maxRaw == 0 );
	CHECK( ib.cal.minCal == 0 );
	CHECK( ib.cal.maxCal == 0 );

	CHECK( ib.cdiagram == nullptr );
	CHECK( ib.calcrop == true );

	CHECK( ib.breaklim == 0 );
	CHECK( ib.value == 0 );
	CHECK( ib.craw == 0 );
	CHECK( ib.cprev == 0 );     /*!< предыдущее значение после калибровки */
	CHECK( ib.safeval == 0 );    /*!< безопасное состояние при завершении процесса */
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

	// Зависимость (d - depend)
	CHECK( ib.d_id == DefaultObjectId );  /*!< идентификатор датчика, от которого зависит данный */
	REQUIRE( ib.d_value == 1 ) ; /*!< разрешающее работу значение датчика от которого зависит данный */
	REQUIRE( ib.d_off_value == 0); /*!< блокирующее значение */
	REQUIRE( ib.d_iotype == UniversalIO::UnknownIOType );

	// Порог
	REQUIRE( ib.t_ai == DefaultObjectId );
	CHECK_FALSE( ib.front ); // флаг работы по фронту
	REQUIRE( ib.front_type == IOBase::ftUnknown );
	CHECK_FALSE( ib.front_prev_state );
	CHECK_FALSE( ib.front_state );
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: Init from xml", "[iobase][init][extensions]")
{
	CHECK( uniset_conf() != nullptr );

	SECTION("Init from xml")
	{
		auto conf = uniset_conf();
		xmlNode* cnode = conf->getNode("iobasetest");
		CHECK( cnode != NULL );
		UniXML::iterator it(cnode);
		auto ui = make_shared<UInterface>();
		ObjectId shmID = conf->getControllerID("SharedMemory");
		CHECK( shmID != DefaultObjectId );

		auto shm = make_shared<SMInterface>(shmID, ui, DefaultObjectId);
		IOBase ib;
		IOBase::initItem(&ib, it, shm, "", false);
		CHECK( ib.si.id == 1 );
		CHECK( ib.si.node == conf->getLocalNode() );
		CHECK( ib.defval == -10 );
		CHECK( ib.calcrop == true );
		CHECK( ib.cal.precision == 3 );
		CHECK( ib.cal.minRaw == -100 );
		CHECK( ib.cal.maxRaw == 100 );
		CHECK( ib.cal.minCal == 0 );
		CHECK( ib.cal.maxCal == 50 );
	}

	SECTION("Init from xml (prefix)")
	{
		auto conf = uniset_conf();
		xmlNode* cnode = conf->getNode("iobasetest3");
		CHECK( cnode != NULL );
		UniXML::iterator it(cnode);
		auto ui = make_shared<UInterface>();
		ObjectId shmID = conf->getControllerID("SharedMemory");
		CHECK( shmID != DefaultObjectId );

		auto shm = make_shared<SMInterface>(shmID, ui, DefaultObjectId);
		IOBase ib;
		IOBase::initItem(&ib, it, shm, "myprefix_", false);
		CHECK( ib.si.id == 10 );
		CHECK( ib.si.node == conf->getLocalNode() );
		CHECK( ib.defval == 5 );
		CHECK( ib.calcrop == false );
		CHECK( ib.cal.precision == 5 );
		CHECK( ib.cal.minRaw == -10 );
		CHECK( ib.cal.maxRaw == 10 );
		CHECK( ib.cal.minCal == -4 );
		CHECK( ib.cal.maxCal == 30 );
		CHECK( ib.cdiagram != nullptr );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: Debounce function", "[iobase][debounce][extensions]")
{
	CHECK( uniset_conf() != nullptr );

	IOBase ib;
	ib.ptDebounce.setTiming(100);
	// Проверка установки сигнала (с дребезгом)
	CHECK_FALSE( ib.check_debounce(true) );
	msleep(20);
	CHECK_FALSE( ib.check_debounce(false) );
	CHECK_FALSE( ib.check_debounce(true) );
	CHECK_FALSE( ib.check_debounce(false) );
	msleep(10);
	CHECK_FALSE( ib.check_debounce(true) );
	msleep(30);
	CHECK_FALSE( ib.check_debounce(false) );
	msleep(10);
	CHECK_FALSE( ib.check_debounce(true) );
	msleep(35);
	CHECK( ib.check_debounce(true) ); // сработал..
	// Проверка сброса сигнала (с дребезгом)
	msleep(30);
	CHECK( ib.check_debounce(true) );
	CHECK( ib.check_debounce(false) );
	msleep(20);
	CHECK( ib.check_debounce(false) );
	msleep(20);
	CHECK( ib.check_debounce(false) );
	CHECK( ib.check_debounce(true) );
	CHECK( ib.check_debounce(false) );
	CHECK( ib.check_debounce(true) );
	CHECK( ib.check_debounce(false) );
	msleep(80);
	CHECK_FALSE( ib.check_debounce(false) ); // сбросился

	// Проверка "устойчивости"
	msleep(30);
	CHECK_FALSE( ib.check_debounce(true) );
	msleep(10);
	CHECK_FALSE( ib.check_debounce(false) );
	msleep(90);
	CHECK_FALSE( ib.check_debounce(false) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: delay function", "[iobase][delay][extensions]")
{
	CHECK( uniset_conf() != nullptr );

	SECTION("On delay..")
	{
		IOBase ib;
		ib.ptOnDelay.setTiming(100);
		// простое срабатывание..
		CHECK_FALSE( ib.check_on_delay(true) );
		msleep(50);
		CHECK_FALSE( ib.check_on_delay(true) );
		msleep(60);
		CHECK( ib.check_on_delay(true) );

		// сброс "без задержки"
		CHECK_FALSE( ib.check_on_delay(false) );

		// задержка засекается каждый раз при переходе "FALSE" --> "TRUE"
		CHECK_FALSE( ib.check_on_delay(true) );
		msleep(90);
		CHECK_FALSE( ib.check_on_delay(false) );
		msleep(20);
		CHECK_FALSE( ib.check_on_delay(false) );
		CHECK_FALSE( ib.check_on_delay(true) );
		msleep(110);
		CHECK( ib.check_on_delay(true) );
	}
	SECTION("Off delay..")
	{
		IOBase ib;
		ib.ptOffDelay.setTiming(100);
		// простое срабатывание..
		CHECK( ib.check_off_delay(true) );
		CHECK( ib.check_off_delay(false) );
		msleep(50);
		CHECK( ib.check_off_delay(false) );
		msleep(60);
		CHECK_FALSE( ib.check_off_delay(false) );

		// устновка "без задержки"
		CHECK( ib.check_off_delay(true) );

		// задержка засекается каждый раз при переходе "TRUE" --> "FALSE"
		CHECK( ib.check_off_delay(false) );
		msleep(90);
		CHECK( ib.check_off_delay(true) );
		CHECK( ib.check_off_delay(false) );
		msleep(20);
		CHECK( ib.check_off_delay(false) );
		msleep(50);
		CHECK( ib.check_off_delay(false) );
		msleep(40);
		CHECK_FALSE( ib.check_off_delay(false) );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: front function", "[iobase][front][extensions]")
{
	CHECK( uniset_conf() != nullptr );

	SECTION("Front '0-->1'")
	{
		IOBase ib;
		ib.front_type = IOBase::ft01;
		ib.front = true;
		CHECK( ib.check_front(true) );
		CHECK( ib.check_front(true) );
		CHECK( ib.check_front(false) );
		CHECK( ib.check_front(false) );
		CHECK_FALSE( ib.check_front(true) );
		CHECK_FALSE( ib.check_front(true) );
		CHECK_FALSE( ib.check_front(false) );
		CHECK( ib.check_front(true) );
	}

	SECTION("Front '1-->0'")
	{
		IOBase ib;
		ib.front_type = IOBase::ft10;
		ib.front = true;
		CHECK_FALSE( ib.check_front(true) );
		CHECK_FALSE( ib.check_front(true) );
		CHECK( ib.check_front(false) );
		CHECK( ib.check_front(false) );
		CHECK( ib.check_front(true) );
		CHECK( ib.check_front(true) );
		CHECK_FALSE( ib.check_front(false) );
	}

	SECTION("Front 'unknown' (off)")
	{
		IOBase ib;
		CHECK( ib.check_front(true) );
		CHECK_FALSE( ib.check_front(false) );
		CHECK( ib.check_front(true) );
		CHECK_FALSE( ib.check_front(false) );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: channel break", "[iobase][extensions]")
{
	CHECK( uniset_conf() != nullptr );

	IOBase ib;
	// просто проверка.. при отключённом breaklim.. (всегда FALSE)
	CHECK_FALSE( ib.check_channel_break(100) );
	CHECK_FALSE( ib.check_channel_break(-100) );
	CHECK_FALSE( ib.check_channel_break(0) );

	const int breakValue = 200;

	ib.breaklim = breakValue;
	CHECK( ib.check_channel_break(breakValue - 10) );
	CHECK( ib.check_channel_break(-breakValue) );
	CHECK( ib.check_channel_break(0) );

	CHECK_FALSE( ib.check_channel_break(breakValue + 1) );
	CHECK_FALSE( ib.check_channel_break(breakValue) );
}
// -----------------------------------------------------------------------------
