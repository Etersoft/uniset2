#include <catch.hpp>

#include <memory>
#include <sstream>
#include <thread>

#include "Mutex.h"
#include "UniSetTypes.h"
#include "LogServer.h"
#include "LogAgregator.h"
#include "LogReader.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
static const std::string ip = "localhost";
static const int port = 33333;

static const std::string test_msg1 = "Test message N1";
static const std::string test_msg2 = "Test message N2";

static ostringstream msg;
static ostringstream msg2;
static ostringstream la_msg;
static uniset_mutex r1_mutex;
static uniset_mutex r2_mutex;

static std::atomic_bool g_read_cancel = ATOMIC_VAR_INIT(0);

static int readTimeout = 4000;

// --------------------------------------------------------------------------
void rlog1OnEvent( const std::string& s )
{
	uniset_mutex_lock l(r1_mutex);
	msg << s;
}
// --------------------------------------------------------------------------
void rlog2OnEvent( const std::string& s )
{
	uniset_mutex_lock l(r2_mutex);
	msg2 << s;
}
// --------------------------------------------------------------------------
void la_logOnEvent( const std::string& s )
{
	la_msg << s;
}
// --------------------------------------------------------------------------
void readlog_thread1()
{
	LogReader lr;
	lr.setinTimeout(readTimeout);
	lr.signal_stream_event().connect( sigc::ptr_fun(rlog1OnEvent) );
	lr.setReadCount(1);
	lr.setLogLevel(Debug::ANY);

	while( !g_read_cancel )
		lr.readlogs(ip, port); // ,LogServerTypes::cmdNOP,0,"",true);
}
// --------------------------------------------------------------------------
void readlog_thread2()
{
	LogReader lr;
	lr.setinTimeout(readTimeout);
	lr.signal_stream_event().connect( sigc::ptr_fun(rlog2OnEvent) );
	lr.setReadCount(1);
	lr.setLogLevel(Debug::ANY);

	while( !g_read_cancel )
		lr.readlogs(ip, port); // ,LogServerTypes::cmdNOP,0,"",true);
}
// --------------------------------------------------------------------------
TEST_CASE("LogAgregator", "[LogServer][LogAgregator]" )
{
	auto la = make_shared<LogAgregator>();
	auto log1 = la->create("log1");
	auto log2 = la->create("log2");

	log1->showDateTime(false);
	log2->showDateTime(false);
	log1->showLogType(false);
	log2->showLogType(false);

	la->signal_stream_event().connect( sigc::ptr_fun(la_logOnEvent) );

	log1->level(Debug::ANY);
	log2->level(Debug::ANY);

	log1->any() << test_msg1;
	REQUIRE( la_msg.str() == test_msg1 );

	la_msg.str("");

	log2->any() << test_msg2;
	REQUIRE( la_msg.str() == test_msg2 );

	auto lst = la->getLogList();
	REQUIRE( lst.size() == 2 );

	// Проверка поиска по регулярным выражениям
	auto lst2 = la->getLogList("/lo.*");
	REQUIRE( lst2.size() == 2 );

	auto lst3 = la->getLogList("/log\\d{1}");
	REQUIRE( lst3.size() == 2 );
}
// --------------------------------------------------------------------------
TEST_CASE("LogServer", "[LogServer]" )
{
	g_read_cancel = false;
	auto la = make_shared<LogAgregator>();
	auto log1 = la->create("log1");
	auto log2 = la->create("log2");

	log1->level(Debug::ANY);
	log2->level(Debug::ANY);
	log1->showDateTime(false);
	log2->showDateTime(false);
	log1->showLogType(false);
	log2->showLogType(false);

	la_msg.str("");
	la->signal_stream_event().connect( sigc::ptr_fun(la_logOnEvent) );

	LogServer ls(la);
	//ls.setSessionLog(Debug::ANY);
	ls.run( ip, port, true );

	for( int i = 0; i < 3 && !ls.isRunning(); i++ )
		msleep(500);

	CHECK( ls.isRunning() );

	msg.str("");
	auto r_thr = make_shared<std::thread>(readlog_thread1);

	msleep(100); // небольшая пауза на создание потока и т.п.

	ostringstream m;
	m << test_msg1 << endl;

	// по сети строка информации передаваться не будет пока не будет записан endl!
	// это сделано для "оптимизации передачи" (чтобы не передавать каждый байт)
	log1->any() << m.str();

	REQUIRE( la_msg.str() == m.str() );

	msleep(readTimeout); // пауза на переподключение reader-а к серверу..
	{
		uniset_mutex_lock l(r1_mutex);
		REQUIRE( msg.str() == m.str() );
	}

	msg.str("");
	la_msg.str("");

	ostringstream m2;
	m2 << test_msg2 << endl;
	log2->any() << m2.str();
	REQUIRE( la_msg.str() == m2.str() );

	msleep(readTimeout); // пауза на переподключение reader-а к серверу..

	{
		uniset_mutex_lock l(r1_mutex);
		REQUIRE( msg.str() == m2.str() );
	}

	g_read_cancel = true;
	msleep(readTimeout);

	if( r_thr->joinable() )
		r_thr->join();
}
// --------------------------------------------------------------------------
TEST_CASE("MaxSessions", "[LogServer]" )
{
	g_read_cancel = false;
	auto la = make_shared<LogAgregator>();
	auto log1 = la->create("log1");
	auto log2 = la->create("log2");

	log1->level(Debug::ANY);
	log2->level(Debug::ANY);
	log1->showDateTime(false);
	log2->showDateTime(false);
	log1->showLogType(false);
	log2->showLogType(false);

	la_msg.str("");
	la->signal_stream_event().connect( sigc::ptr_fun(la_logOnEvent) );

	LogServer ls(la);
	//ls.setSessionLog(Debug::ANY);
	ls.setMaxSessionCount(1);
	ls.run( ip, port, true );

	for( int i = 0; i < 3 && !ls.isRunning(); i++ )
		msleep(500);

	CHECK( ls.isRunning() );

	msg.str("");
	msg2.str("");

	auto r1_thr = make_shared<std::thread>(readlog_thread1);

	msleep(500); // пауза чтобы первый заведомо успел подключиться раньше второго..

	auto r2_thr = make_shared<std::thread>(readlog_thread2);

	msleep(100); // небольшая пауза на создание потока и т.п.

	ostringstream m;
	m << test_msg1 << endl;

	// по сети строка информации передаваться не будет пока не будет записан endl!
	// это сделано для "оптимизации передачи" (чтобы не передавать каждый байт)
	log1->any() << m.str();

	REQUIRE( la_msg.str() == m.str() );

	msleep(readTimeout); // пауза на переподключение reader-а к серверу..
	{
		uniset_mutex_lock l(r1_mutex);
		REQUIRE( msg.str() == m.str() );
	}

	{
		uniset_mutex_lock l(r2_mutex);
		// Ищем часть сообщения об ошибке: '(LOG SERVER): Exceeded the limit on the number of sessions = 1'
		size_t pos = msg2.str().find("Exceeded the limit");
		REQUIRE( pos != std::string::npos );
	}

	g_read_cancel = true;
	msleep(readTimeout);

	if( r1_thr->joinable() )
		r1_thr->join();

	if( r2_thr->joinable() )
		r2_thr->join();
}
// --------------------------------------------------------------------------
TEST_CASE("LogAgregator regexp", "[LogAgregator]" )
{
	auto la = make_shared<LogAgregator>("ROOT");
	auto log1 = la->create("log1");
	auto log2 = la->create("log2");
	auto log3 = la->create("a3/log1");
	auto log4 = la->create("a3/log2");
	auto log5 = la->create("a3/log3");
	auto log6 = la->create("a3/a4/log1");
	auto log7 = la->create("a3/a4/log2");

	auto lst = la->getLogList(".*/a4/.*");
	REQUIRE( lst.size() == 2 );

	lst = la->getLogList(".*a3/.*");
	REQUIRE( lst.size() == 5 );

	lst = la->getLogList(".*log1.*");
	REQUIRE( lst.size() == 3 );
}

// --------------------------------------------------------------------------
