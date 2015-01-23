#include <memory>
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
std::shared_ptr<DebugStream> LogAgregator::create( const std::string& logname )
{
    auto t = getLog(logname);
    if( t )
        return t;

    auto l = std::make_shared<DebugStream>();
    l->setLogName(logname);
    l->signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );
    lmap[logname] = l;
    return l;
}
// -------------------------------------------------------------------------
void LogAgregator::add( std::shared_ptr<DebugStream> l )
{
    auto i = lmap.find(l->getLogName());
    if( i != lmap.end() )
        return;

    l->signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );
    lmap[l->getLogName()] = l;
}
// -------------------------------------------------------------------------
void LogAgregator::addLevel( const std::string& logname, Debug::type t )
{
    auto i = lmap.find(logname);
    if( i != lmap.end() )
        i->second.log->addLevel(t);
}
// -------------------------------------------------------------------------
void LogAgregator::delLevel( const std::string& logname, Debug::type t )
{
    auto i = lmap.find(logname);
    if( i != lmap.end() )
        i->second.log->delLevel(t);
}
// -------------------------------------------------------------------------
void LogAgregator::level( const std::string& logname, Debug::type t )
{
    auto i = lmap.find(logname);
    if( i != lmap.end() )
        i->second.log->level(t);
}
// -------------------------------------------------------------------------
std::shared_ptr<DebugStream> LogAgregator::getLog( const std::string& logname )
{
    if( logname.empty() )
        return nullptr;

    auto i = lmap.find(logname);
    if( i != lmap.end() )
        return i->second.log;

    return nullptr;
}
// -------------------------------------------------------------------------
LogAgregator::LogInfo LogAgregator::getLogInfo( const std::string& logname )
{
    if( logname.empty() )
        return LogInfo();

    auto i = lmap.find(logname);
    if( i != lmap.end() )
        return i->second;

    return LogInfo();
}
// -------------------------------------------------------------------------
