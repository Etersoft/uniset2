// -----------------------------------------------------------------------------
#ifndef TestGenAlone_H_
#define TestGenAlone_H_
// -----------------------------------------------------------------------------
#include "TestGenAlone_SK.h"
// -----------------------------------------------------------------------------
class TestGenAlone:
	public TestGenAlone_SK
{
	public:
		TestGenAlone( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::conf->getNode("TestGenAlone") );
		virtual ~TestGenAlone();


	protected:
		virtual void step();
		void sensorInfo( UniSetTypes::SensorMessage *sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		virtual void sigterm( int signo );

	private:
};
// -----------------------------------------------------------------------------
#endif // TestGenAlone_H_
// -----------------------------------------------------------------------------
