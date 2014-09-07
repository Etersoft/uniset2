#ifndef LogAgregator_H_
#define LogAgregator_H_
// -------------------------------------------------------------------------
#include <string>
#include <list>
#include "DebugStream.h"
#include "LogServerTypes.h"
// -------------------------------------------------------------------------
class LogAgregator:
	public DebugStream
{
    public:

		explicit LogAgregator( Debug::type t = Debug::NONE );
    	explicit LogAgregator( char const * f, Debug::type t = Debug::NONE );

        virtual ~LogAgregator();

        virtual void logFile( const std::string& f );

        void add( DebugStream& log );

        // Управление "подчинёнными" логами
        void addLevel( const std::string& logname, Debug::type t );
        void delLevel( const std::string& logname, Debug::type t );
        void level( const std::string& logname, Debug::type t );

		DebugStream* getLog( const std::string& logname );

    protected:
        void logOnEvent( const std::string& s );

    private:
        typedef std::list<DebugStream*> LogList;
        LogList llst;
};
// -------------------------------------------------------------------------
#endif // LogAgregator_H_
// -------------------------------------------------------------------------
