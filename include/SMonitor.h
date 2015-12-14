#ifndef SMonitor_H_
#define SMonitor_H_
// -----------------------------------------------------------------------------
#include <list>
#include <UniSetObject.h>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
class SMonitor:
	public UniSetObject
{
	public:

		SMonitor( UniSetTypes::ObjectId id );
		~SMonitor();

		// -----
	protected:
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* si ) override;
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sigterm( int signo ) override;
		SMonitor();

	private:
		typedef std::list<UniSetTypes::ParamSInfo> MyIDList;
		MyIDList lst;
		std::string script;
};
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
