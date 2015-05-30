// -----------------------------------------------------------------------------
#ifndef Skel_H_
#define Skel_H_
// -----------------------------------------------------------------------------
#include "Skel_SK.h"
// -----------------------------------------------------------------------------
class Skel:
	public Skel_SK
{
	public:
		Skel( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("Skel") );
		virtual ~Skel();

	protected:
		Skel();

		virtual void step() override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void askSensors( UniversalIO::UIOCommand cmd ) override;

	private:
};
// -----------------------------------------------------------------------------
#endif // Skel_H_
// -----------------------------------------------------------------------------
