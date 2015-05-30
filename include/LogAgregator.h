#ifndef LogAgregator_H_
#define LogAgregator_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <list>
#include <unordered_map>
#include "DebugStream.h"
#include "LogServerTypes.h"
// -------------------------------------------------------------------------
class LogAgregator:
	public DebugStream
{
	public:

		explicit LogAgregator( Debug::type t = Debug::NONE );
		explicit LogAgregator( char const* f, Debug::type t = Debug::NONE );

		virtual ~LogAgregator();

		virtual void logFile( const std::string& f, bool truncate = false ) override;

		void add( std::shared_ptr<DebugStream> log );
		std::shared_ptr<DebugStream> create( const std::string& logname );

		// Управление "подчинёнными" логами
		void addLevel( const std::string& logname, Debug::type t );
		void delLevel( const std::string& logname, Debug::type t );
		void level( const std::string& logname, Debug::type t );


		struct LogInfo
		{
			LogInfo(): log(0), logfile("") {}
			LogInfo( std::shared_ptr<DebugStream>& l ): log(l), logfile(l->getLogFile()) {}
			std::shared_ptr<DebugStream> log;
			std::string logfile;
		};

		std::shared_ptr<DebugStream> getLog( const std::string& logname );
		LogInfo getLogInfo( const std::string& logname );

		std::list<LogInfo> getLogList();
		std::list<LogInfo> getLogList( const std::string& regexp_str );

	protected:
		void logOnEvent( const std::string& s );


	private:
		typedef std::unordered_map<std::string, LogInfo> LogMap;
		LogMap lmap;
};
// -------------------------------------------------------------------------
#endif // LogAgregator_H_
// -------------------------------------------------------------------------
