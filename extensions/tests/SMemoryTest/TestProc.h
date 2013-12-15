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
			tmChange
		};

		virtual void step();
		void sensorInfo( UniSetTypes::SensorMessage *sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );

	private:
		bool state;
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
