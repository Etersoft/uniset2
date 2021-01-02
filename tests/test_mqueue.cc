#include <catch.hpp>
// --------------------------------------------------------------------------
#include <limits>
#include "MQAtomic.h"
#include "MQMutex.h"
#include "MessageType.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace uniset;
// --------------------------------------------------------------------------
// ВНИМАНИЕ! ЗДЕСЬ ОПРЕДЕЛЯЕТСЯ ТИП ТЕСТИРУЕМОЙ ОЧЕРЕДИ
// (пока не придумал как параметризовать тест)
#define TEST_MQ_ATOMIC 1

#ifdef TEST_MQ_ATOMIC
typedef MQAtomic UMessageQueue;
#else
typedef MQMutex UMessageQueue;
#endif

// --------------------------------------------------------------------------
#ifdef TEST_MQ_ATOMIC
// специальный "декоратор" чтобы можно было тестировать переполнение индексов
class MQAtomicTest:
    public MQAtomic
{
    public:
        inline void set_wpos( unsigned long pos )
        {
            MQAtomic::set_wpos(pos);
        }

        inline void set_rpos( unsigned long pos )
        {
            MQAtomic::set_rpos(pos);
        }
};
#endif
// --------------------------------------------------------------------------
// ВНИМАНИЕ! ЗДЕСЬ ОПРЕДЕЛЯЕТСЯ ТИП ТЕСТИРУЕМОЙ ОЧЕРЕДИ
// (пока не придумал как параметризовать тест)
typedef MQAtomic UMessageQueue;
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
static bool pushMessage( UMessageQueue& mq, long id )
{
    SensorMessage sm(id, id);
    sm.consumer = id; // чтобы хоть как-то идентифицировать сообщений, используем поле consumer
    TransportMessage tm( std::move(sm.transport_msg()) );
    auto vm = make_shared<VoidMessage>(tm);
    return mq.push(vm);
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

    REQUIRE( pushMessage(mq, 100) );

    auto msg = mq.top();
    REQUIRE( msg != nullptr );
    REQUIRE( msg->consumer == 100 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow (lost old data)", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    UMessageQueue mq;
    mq.setMaxSizeOfMessageQueue(2);

    mq.setLostStrategy( UMessageQueue::lostOldData );

    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( mq.size() == 1 );

    REQUIRE( pushMessage(mq, 110) );
    REQUIRE( mq.size() == 2 );

    REQUIRE( pushMessage(mq, 120) );
    REQUIRE( mq.size() == 2 );

    auto msg = mq.top();
    REQUIRE( msg != nullptr );
    REQUIRE( msg->consumer == 110 );

    msg = mq.top();
    REQUIRE( msg != nullptr );
    REQUIRE( msg->consumer == 120 );

    REQUIRE( mq.getCountOfLostMessages() == 1 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow (lost new data)", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    UMessageQueue mq;
    mq.setMaxSizeOfMessageQueue(2);

    mq.setLostStrategy( UMessageQueue::lostNewData );

    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( mq.size() == 1 );

    REQUIRE( pushMessage(mq, 110) );
    REQUIRE( mq.size() == 2 );

    REQUIRE_FALSE( pushMessage(mq, 120) );
    REQUIRE( mq.size() == 2 );
    REQUIRE( mq.getCountOfLostMessages() == 1 );

    REQUIRE_FALSE( pushMessage(mq, 130) );
    REQUIRE( mq.size() == 2 );

    REQUIRE( mq.getCountOfLostMessages() == 2 );

    auto msg = mq.top();
    REQUIRE( msg != nullptr );
    REQUIRE( msg->consumer == 100 );

    msg = mq.top();
    REQUIRE( msg != nullptr );
    REQUIRE( msg->consumer == 110 );

    msg = mq.top();
    REQUIRE( msg == nullptr );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: many read", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    UMessageQueue mq;
    mq.setMaxSizeOfMessageQueue(1);
    mq.setLostStrategy( UMessageQueue::lostNewData );

    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( mq.size() == 1 );

    auto msg = mq.top();
    REQUIRE( msg != nullptr );
    REQUIRE( msg->consumer == 100 );

    for( int i = 0; i < 5; i++ )
    {
        auto msg = mq.top();
        REQUIRE( msg == nullptr );
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
    mq.setMaxSizeOfMessageQueue(num + 1);

    size_t rnum = 0;

    for( size_t i = 0; i < num; i++ )
    {
        pushMessage(mq, i);

        // каждые 50 читатем, имитируя реальную работу (чтение между записью)
        if( i % 50 )
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
#ifdef TEST_MQ_ATOMIC

TEST_CASE( "UMessageQueue: overflow index (strategy=lostOldData)", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    unsigned long max = std::numeric_limits<unsigned long>::max();
    MQAtomicTest mq;
    mq.setLostStrategy(MQAtomic::lostOldData);

    mq.set_wpos(max);
    mq.set_rpos(max);

    // При переходе через максимум ничего не должны потерять
    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( pushMessage(mq, 110) );
    REQUIRE( pushMessage(mq, 120) );

    auto m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 100 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 110 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 120 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: lost data (strategy=lostOldData)", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    unsigned long max = std::numeric_limits<unsigned long>::max();
    MQAtomicTest mq;
    mq.setLostStrategy(MQAtomic::lostOldData);
    mq.setMaxSizeOfMessageQueue(2);

    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( pushMessage(mq, 110) );
    REQUIRE( pushMessage(mq, 120) );

    auto m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 110 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 120 );

    m = mq.top();
    REQUIRE( m == nullptr );

    // Теперь проверяем + переполнение счётчика
    mq.set_wpos(max);
    mq.set_rpos(max);

    // При переходе через максимум ничего не должны потерять
    REQUIRE( pushMessage(mq, 140) );
    REQUIRE( pushMessage(mq, 150) );
    REQUIRE( pushMessage(mq, 160) );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 150 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 160 );

    m = mq.top();
    REQUIRE( m == nullptr );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: overflow index (strategy=lostNewData)", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    unsigned long max = std::numeric_limits<unsigned long>::max();
    MQAtomicTest mq;
    mq.setLostStrategy(MQAtomic::lostNewData);

    mq.set_wpos(max);
    mq.set_rpos(max);

    // При переходе через максимум ничего не должны потерять
    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( pushMessage(mq, 110) );
    REQUIRE( pushMessage(mq, 120) );

    auto m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 100 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 110 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 120 );
}
// --------------------------------------------------------------------------
TEST_CASE( "UMessageQueue: lost data (strategy=lostNewData)", "[mqueue]" )
{
    REQUIRE( uniset_conf() != nullptr );

    unsigned long max = std::numeric_limits<unsigned long>::max();
    MQAtomicTest mq;
    mq.setLostStrategy(MQAtomic::lostNewData);
    mq.setMaxSizeOfMessageQueue(2);

    REQUIRE( pushMessage(mq, 100) );
    REQUIRE( pushMessage(mq, 110) );
    REQUIRE_FALSE( pushMessage(mq, 120) );

    auto m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 100 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 110 );

    m = mq.top();
    REQUIRE( m == nullptr );

    // Теперь проверяем + переполнение счётчика
    mq.set_wpos(max);
    mq.set_rpos(max);

    // При переходе через максимум ничего не должны потерять
    REQUIRE( pushMessage(mq, 140) );
    REQUIRE( pushMessage(mq, 150) );
    REQUIRE_FALSE( pushMessage(mq, 160) );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 140 );

    m = mq.top();
    REQUIRE( m != nullptr );
    REQUIRE( m->consumer == 150 );

    m = mq.top();
    REQUIRE( m == nullptr );
}
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
#undef TEST_MQ_ATOMIC
// --------------------------------------------------------------------------

