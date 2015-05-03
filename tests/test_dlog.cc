#include <time.h>
#include "Debug.h"
#include "UniSetTypes.h"
#include "LogAgregator.h"

using namespace std;
using namespace UniSetTypes;

std::ostringstream ss;
std::ostringstream ss1;

void check_log_signal( const string& s )
{
	ss << s;
}

void check_alog_signal( const string& s )
{
	ss1 << s;
}

int main( int argc, const char** argv )
{
	DebugStream tlog;

	tlog.signal_stream_event().connect(&check_log_signal);


	tlog.addLevel(Debug::ANY);

	tlog[Debug::INFO] << ": [info] ..." << endl;
	tlog(Debug::INFO) << ": (info) ..." << endl;
	cout << endl;
	tlog.level1() << ": [level1] ..." << endl;
	tlog.info() << ": [info] ..." << endl;

	if( tlog.is_level1() )
		tlog.level1() << ": is level1..." << endl;

	cout << "===== Test 1 =====" << endl;
	cout << ss.str();
	cout << "==================" << endl;

	auto log1 = make_shared<DebugStream>();
	log1->setLogName("log1");
	auto log2 = make_shared<DebugStream>();
	log2->setLogName("log2");

	LogAgregator la;
	la.signal_stream_event().connect(&check_alog_signal);
	la.add(log1);
	la.add(log2);

	log1->any() << "log1: test message..." << endl;
	log2->any() << "log2: test message..." << endl;
	la << "la: test message.." << endl;

	cout << "===== Test 2 =====" << endl;
	cout << ss1.str();
	cout << "==================" << endl;

	auto l = la.getLog("log1");

	if( l != &log1 )
		cout << "**** TEST FAILED:  LogAgregator::getLog() " << endl;


	cout << "===== Test 3 =====" << endl;
	tlog.level(Debug::ANY);
	tlog.logFile("tlog.log");
	tlog << "TEST TEXT" << endl;
	tlog.logFile("tlog.log", true);


	return 0;
}
