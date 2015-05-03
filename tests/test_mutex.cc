#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <iostream>
#include "Mutex.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TEST_CASE("uniset_mutex", "[mutex][basic]" )
{
	uniset_mutex m("mutex1");

	CHECK( m.name() == "mutex1" );

	m.lock();
	CHECK_FALSE( m.try_lock_for(20) );

	m.unlock();
	CHECK( m.try_lock_for(20) );
	CHECK_FALSE( m.try_lock_for(20) );

	m.setName("m");
	CHECK( m.name() == "m" );
}
// -----------------------------------------------------------------------------
TEST_CASE("uniset_mutex_lock", "[mutex][basic]" )
{
	SECTION("simple lock");
	{
		uniset_mutex m("mutex1");
		{
			uniset_mutex_lock l(m);
			CHECK_FALSE( m.try_lock_for(20) );
		} // unlock

		CHECK( m.try_lock_for(20) );
	}

	SECTION("exception lock");
	{
		uniset_mutex m("mutex1");

		try
		{
			uniset_mutex_lock l(m);
			CHECK_FALSE( m.try_lock_for(20) );
			throw std::logic_error("err");
		}
		catch( const std::logic_error& e )
		{
		} // unlock

		CHECK( m.try_lock_for(20) );
	}

	SECTION("timeout lock");
	{
		uniset_mutex m("mutex1");
		{
			uniset_mutex_lock l(m, 10);
			CHECK_FALSE( m.try_lock_for(20) );

			uniset_mutex_lock l2(m, 20);
			CHECK_FALSE(l2.lock_ok());
		} // unlock..

		uniset_mutex_lock l3(m, 20);
		CHECK(l3.lock_ok());
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("uniset_rwmutex", "[mutex][basic]" )
{
	SECTION("simple lock");
	{
		uniset_rwmutex m("rwmutex");

		m.rlock(); // первый R-захват
		CHECK( m.try_rlock() ); // второй R-захват
		CHECK_FALSE( m.try_wrlock() );
		CHECK_FALSE( m.try_lock() );
		m.unlock(); // отпускаем "первый R-захват"
		CHECK_FALSE( m.try_wrlock() );
		CHECK_FALSE( m.try_lock() );
		m.unlock(); // отпускаем второй R-захват

		CHECK( m.try_wrlock() );
		CHECK_FALSE( m.try_rlock() );
		CHECK_FALSE( m.try_lock() );
		m.unlock();
		CHECK( m.try_lock() );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("uniset_rwmutex_wrlock", "[mutex][basic]" )
{
	SECTION("simple lock");
	{
		uniset_rwmutex m;
		{
			uniset_rwmutex_wrlock l(m);
			CHECK_FALSE( m.try_lock() );
		} // unlock

		uniset_rwmutex_wrlock l2(m);
		CHECK_FALSE( m.try_lock() );
	}

	SECTION("exception lock");
	{
		uniset_rwmutex m;

		try
		{
			uniset_rwmutex_wrlock l(m);
			CHECK_FALSE( m.try_lock() );
			throw std::logic_error("err");
		}
		catch( const std::logic_error& e )
		{
		} // unlock

		uniset_rwmutex_wrlock l2(m);
		CHECK_FALSE( m.try_lock() );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("uniset_rwmutex_rlock", "[mutex][basic]" )
{
	SECTION("simple lock");
	{
		uniset_rwmutex m;
		{
			uniset_rwmutex_rlock l(m);
			CHECK_FALSE( m.try_lock() );
		} // unlock

		uniset_rwmutex_rlock l2(m);
		CHECK_FALSE( m.try_lock() );
	}

	SECTION("exception lock");
	{
		uniset_rwmutex m;

		try
		{
			uniset_rwmutex_rlock l(m);
			CHECK_FALSE( m.try_lock() );
			throw std::logic_error("err");
		}
		catch( const std::logic_error& e )
		{
		} // unlock

		uniset_rwmutex_rlock l2(m);
		CHECK_FALSE( m.try_lock() );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("uniset_rwmutex_{wr|r}lock", "[mutex][basic]" )
{
	uniset_rwmutex m("rwmutex1");

	{
		uniset_rwmutex_rlock l1(m);
		uniset_rwmutex_rlock l2(m);
		uniset_rwmutex_rlock l3(m);
		CHECK_FALSE( m.try_wrlock() );
	} // unlock

	uniset_rwmutex_wrlock l1(m);
	CHECK_FALSE( m.try_lock() );
}
// -----------------------------------------------------------------------------
