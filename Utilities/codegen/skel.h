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
        Skel( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::conf->getNode("Skel") );
        virtual ~Skel();

    protected:
        Skel();

        virtual void step();
        virtual void sensorInfo( UniSetTypes::SensorMessage *sm );
        virtual void timerInfo( UniSetTypes::TimerMessage *tm );
        virtual void askSensors( UniversalIO::UIOCommand cmd );

    private:
};
// -----------------------------------------------------------------------------
#endif // Skel_H_
// -----------------------------------------------------------------------------
