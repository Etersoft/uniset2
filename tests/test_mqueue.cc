#include <catch.hpp>
// --------------------------------------------------------------------------
#include "UMessageQueue.h"
#include "MessageType.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
static void pushMessage( UMessageQueue& mq, long id )
{
	SensorMessage sm(id,id);
	sm.consumer = id; // чтобы хоть как-то идентифицировать сообщений, используем поле consumer
	TransportMessage tm( std::move(sm.transport_msg()) );
	auto vm = make_shared<VoidMessage>(tm);
	mq.push(vm);
}
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

	pushMessage(mq,100);

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	REQUIRE( msg->consumer == 100 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow (lost old data)", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(2);

	mq.setLostStrategy( UMessageQueue::lostOldData );

	pushMessage(mq,100);
	REQUIRE( mq.size() == 1 );

	pushMessage(mq,110);
	REQUIRE( mq.size() == 2 );

	pushMessage(mq,120);
	REQUIRE( mq.size() == 2 );

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	REQUIRE( msg->consumer == 110 );

	msg = mq.top();
	REQUIRE( msg!=nullptr );
	REQUIRE( msg->consumer == 120 );

	REQUIRE( mq.getCountOfQueueFull() == 1 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow (lost new data)", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(2);

	mq.setLostStrategy( UMessageQueue::lostNewData );

	pushMessage(mq,100);
	REQUIRE( mq.size() == 1 );

	pushMessage(mq,110);
	REQUIRE( mq.size() == 2 );

	pushMessage(mq,120);
	REQUIRE( mq.size() == 2 );
	REQUIRE( mq.getCountOfQueueFull() == 1 );

	pushMessage(mq,130);
	REQUIRE( mq.size() == 2 );

	REQUIRE( mq.getCountOfQueueFull() == 2 );

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	REQUIRE( msg->consumer == 100 );

	msg = mq.top();
	REQUIRE( msg!=nullptr );
	REQUIRE( msg->consumer == 110 );

	msg = mq.top();
	REQUIRE( msg==nullptr );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: many read", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(1);
	mq.setLostStrategy( UMessageQueue::lostNewData );

	pushMessage(mq,100);
	REQUIRE( mq.size() == 1 );

	auto msg = mq.top();
	REQUIRE( msg!=nullptr );
	REQUIRE( msg->consumer == 100 );

	for( int i=0; i<5; i++ )
	{
		auto msg = mq.top();
		REQUIRE( msg==nullptr );
	}
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: correct operation", "[mqueue]" )
{
	REQUIRE( uniset_conf() != nullptr );

	// Проверка корректности работы, что сообщения не портяться
	// и не теряются
	// Тест: пишем num сообщений и читаем num сообщений
	// проверяем что ни одно не потерялось
	const size_t num = 1000;

	UMessageQueue mq;
	mq.setMaxSizeOfMessageQueue(num+1);

	size_t rnum = 0;
	for( size_t i=0; i<num; i++ )
	{
		pushMessage(mq,i);

		// каждые 50 читатем, имитируя реальную работу (чтение между записью)
		if( i%50 )
		{
			auto m = mq.top();
			REQUIRE( m->consumer == rnum );
			rnum++;
		}
	}

	REQUIRE( mq.size() == (num - rnum) );

	// дочитываем всё остальное
	while( !mq.empty() )
	{
		auto m = mq.top();
		REQUIRE( m->consumer == rnum );
		rnum++;
	}

	// проверяем что ничего не потерялось
	REQUIRE( rnum == num );
}
// --------------------------------------------------------------------------
