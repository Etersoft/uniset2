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
		TestGenAlone( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("TestGenAlone") );
		virtual ~TestGenAlone();


	protected:
		virtual void step() override;
		void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sigterm( int signo ) override;

	private:
};
// -----------------------------------------------------------------------------
#endif // TestGenAlone_H_
// -----------------------------------------------------------------------------
