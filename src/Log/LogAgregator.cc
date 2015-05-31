#include <memory>
#include <regex>
#include <sstream>
#include "DebugExtBuf.h"
#include "LogAgregator.h"
// -------------------------------------------------------------------------
LogAgregator::LogAgregator( const std::string& name ):
	LogAgregator(name,Debug::NONE)
{
}
// ------------------------------------------------------------------------
#if 0
LogAgregator::LogAgregator( char const* f, Debug::type t ):
	DebugStream(f, t)
{
	delete rdbuf(new teebuf(&internal->fbuf, &internal->sbuf));
}
#endif
// -------------------------------------------------------------------------
LogAgregator::LogAgregator( const std::string& name, Debug::type t ):
	DebugStream(t)
{
	setLogName(name);
	delete rdbuf(new teebuf(&internal->nbuf, &internal->sbuf));
}
// -------------------------------------------------------------------------
void LogAgregator::logFile( const std::string& f, bool truncate )
{
	DebugStream::logFile(f, truncate);

	if( !f.empty() )
		delete rdbuf(new teebuf(&internal->fbuf, &internal->sbuf));
	else
		delete rdbuf(new teebuf(&internal->nbuf, &internal->sbuf));
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
void LogAgregator::add( std::shared_ptr<DebugStream> l, const std::string& lname )
{
	auto lag = std::dynamic_pointer_cast<LogAgregator>(l);
	if( lag )
		addLogAgregator(lag, (lname.empty() ? l->getLogName(): lname) );
	else
		addLog(l,(lname.empty() ? l->getLogName(): lname));
}
// ------------------------------------------------------------------------
void LogAgregator::add( std::shared_ptr<LogAgregator> loga, const std::string& lname  )
{
	addLogAgregator(loga, (lname.empty() ? loga->getLogName(): lname) );
}
// -------------------------------------------------------------------------
void LogAgregator::addLogAgregator( std::shared_ptr<LogAgregator> la, const std::string& lname )
{
	auto i = lmap.find(lname);
	if( i != lmap.end() )
		return;

	auto lst = la->getLogList();
	for( auto&& l: lst )
	{
		auto lag = std::dynamic_pointer_cast<LogAgregator>(l.log);
		if( lag )
		{
			std::ostringstream s;
			s << lname << sep << lag->getLogName();
			addLogAgregator(lag,s.str()); // рекурсия..
		}
		else
		{
			std::ostringstream s;
			s << lname << sep << l.log->getLogName();
			addLog(l.log,s.str());
		}
	}
}
// ------------------------------------------------------------------------
void LogAgregator::addLog( std::shared_ptr<DebugStream> l, const std::string& lname )
{
	auto i = lmap.find(lname);

	if( i != lmap.end() )
		return;

	bool have = false;

	// пока делаем "не оптимальный поиск" / пробегаем по всему списку, зато не храним дополнительный map /
	// на то, чтобы не подключаться к одному логу дважды,
	// иначе будет дублироваие при выводе потом (на экран)
	for( const auto& i : lmap )
	{
		if( i.second.log == l )
		{
			have = true;
			break;
		}
	}

	if( !have )
		l->signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );

	LogInfo li(l,lname);
	lmap[lname] = li;
}
// ------------------------------------------------------------------------
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
std::list<LogAgregator::LogInfo> LogAgregator::getLogList()
{
	std::list<LogAgregator::LogInfo> l;

	for( auto && i : lmap )
		l.push_back(i.second);

	return std::move(l);
}
// -------------------------------------------------------------------------
std::list<LogAgregator::LogInfo> LogAgregator::getLogList( const std::string& regex_str )
{
	std::list<LogAgregator::LogInfo> l;

	try
	{
		std::regex rule(regex_str);

		for( auto && i : lmap )
		{
			//if( std::regex_match(i.second.log->getLogName(), rule) )
			if( std::regex_match(i.first, rule) )
				l.push_back(i.second);
		}
	}
	catch( const std::exception& ex )
	{
		cerr << "(LogAgregator::getLogList): " << ex.what() << std::endl;
	}

	return std::move(l);
}
// -------------------------------------------------------------------------
