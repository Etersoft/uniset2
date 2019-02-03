// -----------------------------------------------------------------------------
#include "TestObject.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TestObject::TestObject( uniset::ObjectId objId, xmlNode* cnode ):
	TestObject_SK(objId, cnode)
{
	vmonit(evntIsOK);
}
// -----------------------------------------------------------------------------
TestObject::~TestObject()
{
}
// -----------------------------------------------------------------------------
void TestObject::askDoNotNotify()
{
	preAskSensors(UniversalIO::UIODontNotify);
}
// -----------------------------------------------------------------------------
void TestObject::askNotifyChange()
{
	preAskSensors(UniversalIO::UIONotifyChange);
}
// -----------------------------------------------------------------------------
void TestObject::askNotifyFirstNotNull()
{
	preAskSensors(UniversalIO::UIONotifyFirstNotNull);
}
// -----------------------------------------------------------------------------
void TestObject::sysCommand( const uniset::SystemMessage* sm )
{
	// фиксируем что SM прислала WDT при своём запуске
	if( sm->command == SystemMessage::WatchDog )
		evntIsOK = true;
}
// -----------------------------------------------------------------------------
void TestObject::sensorInfo( const SensorMessage* sm )
{
	if( sm->id == monotonic_s )
	{
		if( (sm->value - lastValue) < 0 )
			monotonicFailed = true;

		if( (sm->value - lastValue) > 1 )
		{
			cerr << "LOST: sm->value=" << sm->value << " last=" << lastValue
				 << " lost: " << (sm->value - lastValue)
				 << endl;

			lostMessages += (sm->value - lastValue - 1);
		}

		lastValue = sm->value;
	}
}
// -----------------------------------------------------------------------------
void TestObject::onTextMessage( const TextMessage* msg )
{
	lastText = msg->txt;
}
// -----------------------------------------------------------------------------
void TestObject::stopHeartbeat()
{
	maxHeartBeat = 0;
}
// -----------------------------------------------------------------------------
void TestObject::runHeartbeat( int max )
{
	maxHeartBeat = max;
}
// -----------------------------------------------------------------------------
void TestObject::askMonotonic()
{
	askSensor(monotonic_s, UniversalIO::UIONotify);
}
// -----------------------------------------------------------------------------
void TestObject::startMonitonicTest()
{
	monotonicFailed = false;
	lostMessages = false;
	lastValue = in_monotonic_s;
}
// -----------------------------------------------------------------------------
bool TestObject::isMonotonicTestOK() const
{
	return !monotonicFailed;
}
// -----------------------------------------------------------------------------
long TestObject::getLostMessages() const
{
	return lostMessages;
}

long TestObject::getLastValue() const
{
	return lastValue;
}

bool TestObject::isEmptyQueue()
{
	return ( countMessages() == 0 );
}

bool TestObject::isFullQueue()
{
	return (getCountOfLostMessages() > 0);
}

string TestObject::getLastTextMessage() const
{
	return lastText;
}
// -----------------------------------------------------------------------------
