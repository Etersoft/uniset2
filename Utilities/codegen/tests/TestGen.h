// -----------------------------------------------------------------------------
#ifndef TestGen_H_
#define TestGen_H_
// -----------------------------------------------------------------------------
#include "TestGen_SK.h"
// -----------------------------------------------------------------------------
class TestGen:
	public TestGen_SK
{
	public:
		TestGen( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::conf->getNode("TestGen") );
		virtual ~TestGen();


	protected:
		TestGen();
		
		virtual void step();
		void sensorInfo( UniSetTypes::SensorMessage *sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		virtual void sigterm( int signo );
		
	private:
};
// -----------------------------------------------------------------------------
#endif // TestGen_H_
// -----------------------------------------------------------------------------
