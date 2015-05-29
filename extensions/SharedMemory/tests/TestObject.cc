// -----------------------------------------------------------------------------
#include "TestObject.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TestObject::TestObject( UniSetTypes::ObjectId objId, xmlNode* cnode ):
	TestObject_SK(objId, cnode)
{
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
void TestObject::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	// фиксируем что SM прислала WDT при своём запуске
	if( sm->command == SystemMessage::WatchDog )
			evntIsOK = true;
}
// -----------------------------------------------------------------------------
