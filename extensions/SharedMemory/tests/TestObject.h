#ifndef _TestObject_H_
#define _TestObject_H_
// -----------------------------------------------------------------------------
#include "TestObject_SK.h"
// -----------------------------------------------------------------------------
class TestObject:
	public TestObject_SK
{
	public:
		TestObject( uniset::ObjectId objId, xmlNode* cnode );
		virtual ~TestObject();

		void askDoNotNotify();
		void askNotifyChange();
		void askNotifyFirstNotNull();

		inline bool getEvnt()
		{
			return evntIsOK;
		}

		void stopHeartbeat();
		void runHeartbeat( int max = 3 );

		inline uniset::timeout_t getHeartbeatTime()
		{
			return ptHeartBeat.getInterval();
		}

		// тест на последовательность SensorMessage
		void askMonotonic();
		void startMonitonicTest();
		bool isMonotonicTestOK() const;
		long getLostMessages() const;
		long getLastValue() const;
		bool isEmptyQueue();
		bool isFullQueue();

		std::string getLastTextMessage() const;
		int getLastTextMessageType() const;

	protected:
		TestObject();

		virtual void sysCommand( const uniset::SystemMessage* sm ) override;
		virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
		virtual void onTextMessage( const uniset::TextMessage* msg ) override;

	private:
		bool evntIsOK = { false };

		bool monotonicFailed = { false };
		long lostMessages = { false };
		long lastValue = { 0 };
		std::string lastText = { "" };
		int lastTextType = { 0 };
};
// -----------------------------------------------------------------------------
#endif // _TestObject_H_
// -----------------------------------------------------------------------------
