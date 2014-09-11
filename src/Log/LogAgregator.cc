#include "DebugExtBuf.h"
#include "LogAgregator.h"
// -------------------------------------------------------------------------
LogAgregator::LogAgregator( char const * f, Debug::type t ):
    DebugStream(f,t)
{
    delete rdbuf(new teebuf(&internal->fbuf,&internal->sbuf));
}
// -------------------------------------------------------------------------
LogAgregator::LogAgregator( Debug::type t ):
    DebugStream(t)
{
    delete rdbuf(new teebuf(&internal->nbuf,&internal->sbuf));
}
// -------------------------------------------------------------------------
void LogAgregator::logFile( const std::string& f )
{
    DebugStream::logFile(f);
    if( !f.empty() )
        delete rdbuf(new teebuf(&internal->fbuf,&internal->sbuf));
    else
        delete rdbuf(new teebuf(&internal->nbuf,&internal->sbuf));
}
// -------------------------------------------------------------------------
LogAgregator::~LogAgregator()
{
}
// -------------------------------------------------------------------------
void LogAgregator::logOnEvent( const std::string& s )
{
    (*this) << s;
}
// -------------------------------------------------------------------------
void LogAgregator::add( DebugStream& l )
{
    l.signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );
    for( LogList::iterator i=llst.begin(); i!=llst.end(); i++ )
    {
         if( &l == i->log )
             return;
    }

    llst.push_back(&l);
}
// -------------------------------------------------------------------------
void LogAgregator::addLevel( const std::string& logname, Debug::type t )
{
    for( auto& i: llst )
    {
        if( i.log->getLogName() == logname )
        {
            i.log->addLevel(t);
            break;
        }
    }
}
// -------------------------------------------------------------------------
void LogAgregator::delLevel( const std::string& logname, Debug::type t )
{
    for( auto& i: llst )
    {
        if( i.log->getLogName() == logname )
        {
            i.log->delLevel(t);
            break;
        }
    }
}
// -------------------------------------------------------------------------
void LogAgregator::level( const std::string& logname, Debug::type t )
{
    for( auto& i: llst )
    {
        if( i.log->getLogName() == logname )
        {
            i.log->level(t);
            break;
        }
    }
}
// -------------------------------------------------------------------------
DebugStream* LogAgregator::getLog( const std::string& logname )
{
    if( logname.empty() )
        return 0;

    for( auto& i: llst )
    {
        if( i.log->getLogName() == logname )
            return i.log;
    }

    return 0;
}
// -------------------------------------------------------------------------
LogAgregator::LogInfo LogAgregator::getLogInfo( const std::string& logname )
{
    if( logname.empty() )
        return LogInfo();

    for( auto& i: llst )
    {
        if( i.log->getLogName() == logname )
            return i;
    }

    return LogInfo();
}
// -------------------------------------------------------------------------
