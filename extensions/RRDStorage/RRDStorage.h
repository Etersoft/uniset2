#ifndef _RRDStorage_H_
#define _RRDStorage_H_
// -----------------------------------------------------------------------------
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
// -----------------------------------------------------------------------------
/*!
*/
class RRDStorage:
	public UObject_SK
{
	public:
		RRDStorage( UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
					const std::string prefix="rrd" );
		virtual ~RRDStorage();
	
		/*! глобальная функция для инициализации объекта */
		static RRDStorage* init_rrdstorage( int argc, const char* const* argv, 
						    UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
							const std::string prefix="rrd" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

	protected:
		RRDStorage();

		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sensorInfo( UniSetTypes::SensorMessage* sm );
		virtual void timerInfo( UniSetTypes::TimerMessage* tm );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm );

		virtual void initRRD( xmlNode* cnode, int tmID );
		void step();

		SMInterface* shm;

		struct DSInfo
		{
			std::string dsname;
			long value;

			DSInfo( const std::string& dsname, long defval ):
				dsname(dsname),value(defval){}
		};

		typedef std::map<UniSetTypes::ObjectId,DSInfo> DSMap;

		struct RRDInfo
		{
			std::string filename;
			long tid;
			long sec;
			DSMap dsmap;
			
			RRDInfo( const std::string& fname, long tmID, long sec, const DSMap& ds ):
				filename(fname),tid(tmID),sec(sec),dsmap(ds){}
		};

		typedef std::list<RRDInfo> RRDList;

		RRDList rrdlist;	

	private:

		std::string prefix;
};
// -----------------------------------------------------------------------------
#endif // _RRDStorage_H_
// -----------------------------------------------------------------------------
