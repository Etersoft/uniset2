#ifndef _TestObject_H_
#define _TestObject_H_
// -----------------------------------------------------------------------------
#include "TestObject_SK.h"
// -----------------------------------------------------------------------------
class TestObject:
	public TestObject_SK
{
	public:
		TestObject( UniSetTypes::ObjectId objId, xmlNode* cnode );
		virtual ~TestObject();

		void askDoNotNotify();
		void askNotifyChange();
		void askNotifyFirstNotNull();

		inline bool getEvnt(){ return evntIsOK; }
	protected:
		TestObject();

		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;

	private:
		bool evntIsOK = { false };
};
// -----------------------------------------------------------------------------
#endif // _TestObject_H_
// -----------------------------------------------------------------------------
