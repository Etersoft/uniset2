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
		TestGenAlone( uniset::ObjectId id, xmlNode* confnode = uniset::uniset_conf()->getNode("TestGenAlone") );
		virtual ~TestGenAlone();


	protected:
		virtual void step() override;
		void sensorInfo( const uniset::SensorMessage* sm ) override;
		void timerInfo( const uniset::TimerMessage* tm ) override;

	private:
};
// -----------------------------------------------------------------------------
#endif // TestGenAlone_H_
// -----------------------------------------------------------------------------
