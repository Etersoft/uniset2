/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <memory>
#include <iomanip>
#include <sstream>
#include "UniSetTypes.h"
#include "DebugExtBuf.h"
#include "LogAgregator.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
	// -------------------------------------------------------------------------
	const std::string LogAgregator::sep = "/";
	// -------------------------------------------------------------------------
	LogAgregator::LogAgregator( const std::string& name ):
		LogAgregator(name, Debug::ANY)
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
		auto conn = l->signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );
		conmap.emplace(l, conn);
		lmap[logname] = l;
		return l;
	}
	// -------------------------------------------------------------------------
	void LogAgregator::add( std::shared_ptr<DebugStream> l, const std::string& lname )
	{
		auto lag = std::dynamic_pointer_cast<LogAgregator>(l);

		if( lag )
			addLogAgregator(lag, (lname.empty() ? l->getLogName() : lname) );
		else
			addLog(l, (lname.empty() ? l->getLogName() : lname), true);
	}
	// ------------------------------------------------------------------------
	void LogAgregator::add( std::shared_ptr<LogAgregator> loga, const std::string& lname  )
	{
		addLogAgregator(loga, (lname.empty() ? loga->getLogName() : lname) );
	}
	// -------------------------------------------------------------------------
	void LogAgregator::addLogAgregator( std::shared_ptr<LogAgregator> la, const std::string& lname )
	{
		auto i = lmap.find(lname);

		if( i != lmap.end() )
			return;

		auto lst = la->getLogList();

		for( auto&& l : lst )
		{
			auto c = conmap.find(l.log);

			if( c == conmap.end() )
			{
				auto conn = l.log->signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );
				conmap.emplace(l.log, conn);
			}
		}

		addLog(la, lname, false);
	}
	// ------------------------------------------------------------------------
	void LogAgregator::addLog( std::shared_ptr<DebugStream> l, const std::string& lname, bool connect )
	{
		auto i = lmap.find(lname);

		if( i != lmap.end() )
			return;

		if( connect )
		{
			auto c = conmap.find(l);

			if( c == conmap.end() )
			{
				auto conn = l->signal_stream_event().connect( sigc::mem_fun(this, &LogAgregator::logOnEvent) );
				conmap.emplace(l, conn);
			}
		}

		lmap[lname] = l;
	}
	// ------------------------------------------------------------------------
	void LogAgregator::addLevel( const std::string& logname, Debug::type t )
	{
		auto i = lmap.find(logname);

		if( i != lmap.end() )
			i->second->addLevel(t);
	}
	// -------------------------------------------------------------------------
	void LogAgregator::delLevel( const std::string& logname, Debug::type t )
	{
		auto i = lmap.find(logname);

		if( i != lmap.end() )
			i->second->delLevel(t);
	}
	// -------------------------------------------------------------------------
	void LogAgregator::level( const std::string& logname, Debug::type t )
	{
		auto i = lmap.find(logname);

		if( i != lmap.end() )
			i->second->level(t);
	}
	// -------------------------------------------------------------------------
	std::shared_ptr<DebugStream> LogAgregator::getLog( const std::string& logname )
	{
		if( logname.empty() )
			return nullptr;

		return findLog(logname);
	}
	// -------------------------------------------------------------------------
	std::ostream& LogAgregator::printLogList( std::ostream& os, const std::string& regexp_str ) const
	{
		std::list<iLog> lst;

		if( regexp_str.empty() )
			lst = getLogList();
		else
			lst = getLogList(regexp_str);

		return printLogList(os, lst);
	}
	// -------------------------------------------------------------------------
	std::ostream& LogAgregator::printLogList( std::ostream& os, std::list<iLog>& lst )
	{
		std::string::size_type max_width = 1;

		for( const auto& l : lst )
			max_width = std::max(max_width, l.name.length() );

		for( const auto& l : lst )
			os << std::left << setw(max_width) << l.name << std::left << " [ " << Debug::str(l.log->level()) << " ]" << endl;

		return os;
	}
	// -------------------------------------------------------------------------
	std::ostream& LogAgregator::printTree( std::ostream& os, const std::string& g_tab ) const
	{
		const std::string::size_type tab_width = 15;

		ostringstream s;
		s << setw(tab_width) << "." << g_tab;

		string s_tab(s.str());

		uniset::ios_fmt_restorer ifs(os);

		os << std::right << g_tab << setw(tab_width - 1) << std::right << getLogName() << sep << endl; // << setw(6) << " " << "[ " << Debug::str(DebugStream::level()) << " ]" << endl;

		std::list<std::shared_ptr<LogAgregator>> lstA;
		std::list<std::shared_ptr<DebugStream>> lst;

		for( const auto& l : lmap )
		{
			auto ag = dynamic_pointer_cast<LogAgregator>(l.second);

			if( ag )
				lstA.push_back(ag);
			else
				lst.push_back(l.second);
		}

		lst.sort([](const std::shared_ptr<DebugStream>& a, const std::shared_ptr<DebugStream>& b)
		{
			return a->getLogName() < b->getLogName();
		});

		lstA.sort([](const std::shared_ptr<LogAgregator>& a, const std::shared_ptr<LogAgregator>& b)
		{
			return a->getLogName() < b->getLogName();
		});

		// сперва выводим просто логи
		for( auto&& l : lst )
			os << s_tab << setw(tab_width) << std::right << l->getLogName() << std::left << " [ " << Debug::str(l->level()) << " ]" << endl;

		// потом дочерние агрегаторы
		for( auto&& ag : lstA )
			ag->printTree(os, s_tab);

		return os;
	}
	// -------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, LogAgregator& la )
	{
		la.printTree(os, "");
		return os;
	}
	// -------------------------------------------------------------------------
	std::ostream& operator<<(std::ostream& os, std::shared_ptr<LogAgregator> la )
	{
		return os << *(la.get());
	}
	// -------------------------------------------------------------------------
	std::vector<std::string> LogAgregator::splitFirst( const std::string& lname, const std::string s )
	{
		string::size_type pos = lname.find(s);

		if( pos == string::npos )
		{
			vector<string> v{lname, ""};
			return v;
		}

		vector<string> v = {lname.substr(0, pos), lname.substr(pos + 1, lname.length()) };
		return v;
	}
	// -------------------------------------------------------------------------
	bool LogAgregator::logExist( std::shared_ptr<DebugStream>& log ) const
	{
		for( const auto& l : lmap )
		{
			if( l.second == log )
				return true;

			bool res = false;
			auto ag = dynamic_pointer_cast<LogAgregator>(l.second);

			if( ag )
				res = ag->logExist(log);

			if( res )
				return true;
		}

		return false;
	}
	// -------------------------------------------------------------------------
	std::shared_ptr<DebugStream> LogAgregator::findByLogName( const std::string& logname ) const
	{
		std::shared_ptr<DebugStream> log;

		for( const auto& l : lmap )
		{
			if( l.second->getLogName() == logname )
				return l.second;

			auto ag = dynamic_pointer_cast<LogAgregator>(l.second);

			if( ag )
			{
				log = ag->findByLogName(logname);

				if( log != nullptr )
					return log;
			}
		}

		return nullptr;
	}
	// -------------------------------------------------------------------------
	std::shared_ptr<DebugStream> LogAgregator::findLog( const std::string& lname ) const
	{
		auto v = splitFirst(lname, sep);

		if( v[1].empty() )
		{
			auto l = lmap.find(lname);

			if( l == lmap.end() )
				return nullptr;

			return l->second;
		}

		auto l = lmap.find(v[0]);

		if( l == lmap.end() )
			return nullptr;

		auto ag = dynamic_pointer_cast<LogAgregator>(l->second);

		if( !ag )
			return l->second;

		return ag->findLog(v[1]); // рекурсия
	}
	// -------------------------------------------------------------------------
	void LogAgregator::getListByLogNameWithRule( std::list<LogAgregator::iLog>& lst, const std::regex& rule, const std::string& prefix ) const
	{
		ostringstream s;
		s << prefix << getLogName();
		const string p2 = s.str() + sep;

		for( const auto& l : lmap )
		{
			auto ag = dynamic_pointer_cast<LogAgregator>(l.second);

			if( !std::regex_match(l.second->getLogName(), rule) )
			{
				// recursive
				if( ag )
					ag->getListByLogNameWithRule(lst, rule, p2);

				continue;
			}

			if( !ag )
			{
				const std::string nm(p2 + l.second->getLogName());
				lst.emplace_back(l.second, std::move(nm) );
				continue;
			}

			// add all childs
			lst.splice(lst.end(), ag->getLogList());
		}
	}
	// -------------------------------------------------------------------------
	std::list<LogAgregator::iLog> LogAgregator::getLogList( const std::string& regex_str ) const
	{
		std::list<LogAgregator::iLog> lst;

		try
		{
			std::regex rule(regex_str);
			getListByLogNameWithRule(lst, rule, "");
		}
		catch( const std::exception& ex )
		{
			cerr << "(LogAgregator::getLogList): " << ex.what() << std::endl;
		}

		return lst;
	}
	// -------------------------------------------------------------------------
	std::list<LogAgregator::iLog> LogAgregator::getLogList() const
	{
		std::list<LogAgregator::iLog> lst = makeLogNameList("");
		lst.sort([](const LogAgregator::iLog & a, const LogAgregator::iLog & b)
		{
			return a.name < b.name;
		});
		return lst;
	}
	// -------------------------------------------------------------------------
	std::list<LogAgregator::iLog> LogAgregator::makeLogNameList( const std::string& prefix ) const
	{
		std::list<LogAgregator::iLog> lst;

		ostringstream s;
		s << prefix << getLogName();
		const string p2 = s.str() + sep;

		for( const auto& l : lmap )
		{
			auto ag = dynamic_pointer_cast<LogAgregator>(l.second);

			if( ag )
				lst.splice(lst.end(), ag->makeLogNameList(p2));
			else
			{
				const std::string nm(p2 + l.second->getLogName());
				lst.emplace_back(l.second, std::move(nm) );
			}
		}

		return lst;
	}
	// -------------------------------------------------------------------------
	void LogAgregator::offLogFile( const std::string& logname )
	{
		auto l = findLog(logname);

		if( l )
			l->offLogFile();
	}
	// -------------------------------------------------------------------------
	void LogAgregator::onLogFile( const std::string& logname )
	{
		auto l = findLog(logname);

		if( l )
			l->onLogFile();
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
