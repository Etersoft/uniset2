// -----------------------------------------------------------------------------
#ifndef TestProc_H_
#define TestProc_H_
// -----------------------------------------------------------------------------
#include "TestProc_SK.h"
// -----------------------------------------------------------------------------
class TestProc:
	public TestProc_SK
{
	public:
		TestProc( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::conf->getNode("TestProc") );
		virtual ~TestProc();

	protected:
		TestProc();

		enum Timers
		{
			tmChange,
			tmCheckWorking,
			tmCheck
		};

		virtual void step();
		virtual void sensorInfo( UniSetTypes::SensorMessage *sm );
		virtual void timerInfo( UniSetTypes::TimerMessage *tm );
        virtual void sysCommand( UniSetTypes::SystemMessage* sm );
		
        void test_depend();
        void test_undefined_state();
        void test_thresholds();

	private:
		bool state;
		bool undef;
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
