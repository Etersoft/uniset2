// $Id: SMonitor.h,v 1.2 2007/12/17 22:51:00 vpashka Exp $
// ------------------------------------------------------------------------------------------
#ifndef SMonitor_H_
#define SMonitor_H_
// ------------------------------------------------------------------------------------------
#include <string>
#include <list>
#include <UniSetObject_LT.h>
#include "UniSetTypes.h"
// ------------------------------------------------------------------------------------------
class SMonitor: 
	public UniSetObject_LT
{
	public:
	
		SMonitor( UniSetTypes::ObjectId id );
	    ~SMonitor();

		// -----
	protected:
		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		virtual void sysCommand( UniSetTypes::SystemMessage *sm );
		virtual void sensorInfo( UniSetTypes::SensorMessage *si );
		virtual void timerInfo( UniSetTypes::TimerMessage *tm );
		virtual void sigterm( int signo );
	    SMonitor();
		
	private:
		UniSetTypes::IDList lst;
		std::string script;
};

#endif
