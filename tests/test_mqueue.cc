#include <catch.hpp>
// --------------------------------------------------------------------------
#include "UMessageQueue.h"
#include "MessageType.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: setup", "[mqueue]" )
{
	UMessageQueue mq;

	// проверка установки размера очереди
	mq.setMaxSizeOfMessageQueue(10);
	REQUIRE(mq.getMaxSizeOfMessageQueue() == 10 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: simple push/top", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;

	SensorMessage sm(100,2);
	TransportMessage tm( std::move(sm.transport_msg()) );
	mq.push(tm);

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	SensorMessage sm2( msg.get() );
	REQUIRE( sm.id == sm2.id );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow (lost old data)", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(1);

	mq.setLostStrategy( UMessageQueue::lostOldData );

	SensorMessage sm1(100,2);
	TransportMessage tm1( std::move(sm1.transport_msg()) );
	mq.push(tm1);

	REQUIRE( mq.size() == 1 );

	SensorMessage sm2(110,50);
	TransportMessage tm2( std::move(sm2.transport_msg()) );
	mq.push(tm2);

	REQUIRE( mq.size() == 1 );

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	SensorMessage sm( msg.get() );
	REQUIRE( sm.id == sm2.id );

	REQUIRE( mq.getCountOfQueueFull() == 1 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow (lost new data)", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(1);

	mq.setLostStrategy( UMessageQueue::lostNewData );

	SensorMessage sm1(100,2);
	TransportMessage tm1( std::move(sm1.transport_msg()) );
	mq.push(tm1);

	REQUIRE( mq.size() == 1 );

	SensorMessage sm2(110,50);
	TransportMessage tm2( std::move(sm2.transport_msg()) );
	mq.push(tm2);

	REQUIRE( mq.getCountOfQueueFull() == 1 );

	SensorMessage sm3(120,150);
	TransportMessage tm3( std::move(sm3.transport_msg()) );
	mq.push(tm3);

	REQUIRE( mq.size() == 1 );
	REQUIRE( mq.getCountOfQueueFull() == 2 );

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	SensorMessage sm( msg.get() );
	REQUIRE( sm.id == sm1.id );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: many read", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(1);
	mq.setLostStrategy( UMessageQueue::lostNewData );

	SensorMessage sm1(100,2);
	TransportMessage tm1( std::move(sm1.transport_msg()) );
	mq.push(tm1);

	REQUIRE( mq.size() == 1 );

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	SensorMessage sm( msg.get() );
	REQUIRE( sm.id == sm1.id );

	for( int i=0; i<5; i++ )
	{
		auto msg = mq.top();
		REQUIRE( msg==nullptr );
	}
}
// --------------------------------------------------------------------------
