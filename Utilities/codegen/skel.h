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
        Skel( uniset::ObjectId id, xmlNode* confnode = uniset::uniset_conf()->getNode("Skel") );
        virtual ~Skel();

    protected:
        Skel();

        virtual void step() override;
        virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
        virtual void timerInfo( const uniset::TimerMessage* tm ) override;
        virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
        virtual void sysCommand( const uniset::SystemMessage* sm ) override;

    private:
};
// -----------------------------------------------------------------------------
#endif // Skel_H_
// -----------------------------------------------------------------------------
