#ifndef SMonitor_H_
#define SMonitor_H_
// -----------------------------------------------------------------------------
#include <list>
#include <UniSetObject_LT.h>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
class SMonitor: 
    public UniSetObject_LT
{
    public:

        SMonitor( UniSetTypes::ObjectId id );
        ~SMonitor();

        // -----
    protected:
        virtual void sysCommand( const UniSetTypes::SystemMessage *sm );
        virtual void sensorInfo( const UniSetTypes::SensorMessage *si );
        virtual void timerInfo( const UniSetTypes::TimerMessage *tm );
        virtual void sigterm( int signo );
        SMonitor();

    private:
        typedef std::list<UniSetTypes::ParamSInfo> MyIDList;
        MyIDList lst;
        std::string script;
};
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
