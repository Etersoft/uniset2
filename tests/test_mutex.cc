#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <iostream>
#include <thread>
#include <future>
#include "Mutex.h"
#include "UniSetTypes.h"
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
// -----------------------------------------------------------------------------
uniset_rwmutex g_mutex;

bool writer_thread( int num )
{
	uniset_rwmutex_wrlock l(g_mutex);
	msleep(1000);
	return true;
}

bool reader_thread( int num )
{
	uniset_rwmutex_rlock l(g_mutex);
	msleep(50);
	return true;
}
// -----------------------------------------------------------------------------
TEST_CASE("uniset_rwmutex_{wr|r} thread lock", "[mutex][threadlock][basic]" )
{
	g_mutex.rlock();

	std::vector< std::future<bool> > vw(3);
	for( int w=0; w<3; w++ )
		vw.push_back( std::async(std::launch::async, writer_thread,w) );

	std::vector< std::future<bool> > vr(3);
	for( int r=0; r<5; r++ )
		vr.push_back( std::async(std::launch::async, reader_thread,r) );

	msleep(10);

	// read захватывают сразу без задержек..
	for( auto&& i: vr )
	{
		if( i.valid()  )
			REQUIRE( i.get() == true );
	}

	// теперь отпускаю mutex
	g_mutex.unlock();

	// проверяем что write-ы завершают работу.
	for( auto& i: vw )
	{
		if( i.valid() )
			REQUIRE( i.get() == true );
	}
}
// -----------------------------------------------------------------------------
