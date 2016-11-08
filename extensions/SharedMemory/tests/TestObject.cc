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
