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
		TestGen( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("TestGen") );
		virtual ~TestGen();


	protected:
		TestGen();

		virtual void step() override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sigterm( int signo ) override;

	private:
};
// -----------------------------------------------------------------------------
#endif // TestGen_H_
// -----------------------------------------------------------------------------
