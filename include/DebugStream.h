// -*- C++ -*-

// Created by Lars Gullik BjЬnnes
// Copyright 1999 Lars Gullik BjЬnnes (larsbj@lyx.org)
// Released into the public domain.

// Implemented and tested on g++ 2.7.2.3

// Primarily developed for use in the LyX Project http://www.lyx.org/
// but should be adaptable to any project.

// (c) 2002 adapted for UniSet by Lav, GNU LGPL license
// Modify for UniSet by pv@etersoft.ru, GNU LGPL license

#ifndef DEBUGSTREAM_H
#define DEBUGSTREAM_H

//#ifdef __GNUG__
//#pragma interface
//#endif

#include <iostream>
#include <string>
#include <sigc++/sigc++.h>
#include <vector>
#include "Debug.h"

#ifdef TEST_DEBUGSTREAM
#include <string>
struct Debug
{
	enum type
	{
		NONE = 0,
		INFO       = (1 << 0),   // 1
		WARN       = (1 << 1),   // 2
		CRIT       = (1 << 2)   // 4
	};
	static const type ANY = type(INFO | WARN | CRIT);
	static Debug::type value(std::string const& val)
	{
		if (val == "NONE") return Debug::NONE;

		if (val == "INFO") return Debug::INFO;

		if (val == "WARN") return Debug::WARN;

		if (val == "CRIT") return Debug::CRIT;

		return Debug::NONE;
	}
};
#endif

/** DebugStream is a ostream intended for debug output.
    It has also support for a logfile. Debug output is output to cerr
    and if the logfile is set, to the logfile.

    Example of Usage:
    DebugStream debug;
    debug.level(Debug::INFO);
    debug.debug(Debug::WARN) << "WARN\n";
    debug[Debug::INFO] << "INFO\n";
    debug << "Always\n";

    Will output:
    INFO
    Always

    If you want to have debug output from time critical code you should
    use this construct:
	if (debug.is_info()) {
     debug << "...debug output...\n";
    }

    To give debug info even if no debug (NONE) is requested:
    debug << "... always output ...\n";

    To give debug output regardless of what debug level is set (!NONE):
    debug.debug() << "...on debug output...\n";
    debug[Debug::ANY] << "...on debug output...\n";

    To give debug output when a specific debug level is set (INFO):
    debug.debug(Debug::INFO) << "...info...\n";
    debug[Debug::INFO] << "...info...\n";

    To give debug output when either on of debug levels is set (INFO or CRIT):
    debug.debug(Debug::type(Debug::INFO | Debug::CRIT)) << "...info/crit...\n";
    debug[Debug::type(Debug::INFO | Debug::CRIT)] << "...info/crit...\n";

*/
class DebugStream : public std::ostream
{
	public:
		/// Constructor, sets the debug level to t.
		explicit DebugStream(Debug::type t = Debug::NONE, Debug::verbosity v = 0);

		/// Constructor, sets the log file to f, and the debug level to t.
		explicit
		DebugStream(char const* f, Debug::type t = Debug::NONE, bool truncate = false );

		///
		virtual ~DebugStream();

		typedef sigc::signal<void, const std::string&> StreamEvent_Signal;
		StreamEvent_Signal signal_stream_event();

		/// Sets the debug level to t.
		void level(Debug::type t) noexcept
		{
			dt = Debug::type(t & Debug::ANY);
		}

		/// Returns the current debug level.
		Debug::type level() const noexcept
		{
			return dt;
		}

		/// Adds t to the current debug level.
		void addLevel(Debug::type t) noexcept
		{
			dt = Debug::type(dt | t);
		}

		/// Deletes t from the current debug level.
		void delLevel(Debug::type t) noexcept
		{
			dt = Debug::type(dt & ~t);
		}

		/// Sets the debugstreams' logfile to f.
		virtual void logFile( const std::string& f, bool truncate = false );

		inline std::string getLogFile() const noexcept
		{
			return fname;
		}

		// имя лог файла можно установить отдельно, но не включать запись..
		inline void setLogFile( const std::string& n ) noexcept
		{
			fname = n;
		}

		// включена ли запись лог-файла
		inline bool isOnLogFile() const noexcept
		{
			return isWriteLogFile;
		}

		// включить запись лог файла
		inline void onLogFile( bool truncate = false )
		{
			logFile(fname, truncate);
		}

		// отключить запись логфайла
		inline void offLogFile() noexcept
		{
			logFile("");
		}

		// enable print on screen
		void enableOnScreen();

		// disable print onscreen
		void disableOnScreen();

		/// Returns true if t is part of the current debug level.
		inline bool debugging(Debug::type t = Debug::ANY) const noexcept
		{
			return (dt & t);
		}

		/** Returns the no-op stream if t is not part of the
		    current debug level otherwise the real debug stream
		    is used.
		*/
		std::ostream& debug(Debug::type t = Debug::ANY) noexcept;

		/** This is an operator to give a more convenient use:
		    dbgstream[Debug::INFO] << "Info!\n";
		    Вывод осуществляется с датой и временем (если они не отключены)
		*/
		std::ostream& operator[](Debug::type t) noexcept
		{
			return debug(t);
		}

		/**
		    Вывод продолжения логов (без даты и времени)
		*/
		inline std::ostream& to_end(Debug::type t) noexcept
		{
			return this->operator()(t);
		}

		/**
		    Вывод продолжения логов (без даты и времени) "log()"
		*/
		std::ostream& operator()(Debug::type t) noexcept;

		inline void showDateTime(bool s) noexcept
		{
			show_datetime = s;
		}

		inline void showMilliseconds( bool s ) noexcept
		{
			show_msec = s;
		}

		inline void showMicroseconds( bool s ) noexcept
		{
			show_usec = s;
		}

		inline void showLogType(bool s) noexcept
		{
			show_logtype = s;
		}

		inline void showLabels(bool s) noexcept
		{
			show_labels = s;
		}

		inline void hideLabelKey(bool s) noexcept
		{
			hide_label_key = s;
		}

		inline std::ostream& log(Debug::type l) noexcept
		{
			return this->operator[](l);
		}

		void verbose(Debug::verbosity v) noexcept
		{
			verb = v;
		}

		/// Returns the current verbose level.
		Debug::verbosity verbose() const noexcept
		{
			return verb;
		}

		// example: dlog.V(1)[Debug::INFO] << "some log.." << endl;
		DebugStream& V( Debug::verbosity v ) noexcept;

		// labels
		typedef std::pair<std::string, std::string> Label;

		void addLabel( const std::string& key, const std::string& value ) noexcept;
		void delLabel( const std::string& key ) noexcept;
		void cleanupLabels() noexcept;
		std::vector<Label> getLabels() noexcept;

		// -----------------------------------------------------
		// короткие функции (для удобства)
		// log.level1()  - вывод с датой и временем  "date time [LEVEL] ...",
		//    если вывод даты и времени не выключен при помощи showDateTime(false)
		// if( log.is_level1() ) - проверка включён ли лог.."

#define DMANIP(FNAME,LEVEL) \
	inline std::ostream& FNAME( bool showdatetime=true ) noexcept \
	{\
		if( showdatetime )\
			return operator[](Debug::LEVEL); \
		return  operator()(Debug::LEVEL); \
	} \
	\
	inline bool is_##FNAME() const  noexcept\
	{ return debugging(Debug::LEVEL); }

		DMANIP(level1, LEVEL1)
		DMANIP(level2, LEVEL2)
		DMANIP(level3, LEVEL3)
		DMANIP(level4, LEVEL4)
		DMANIP(level5, LEVEL5)
		DMANIP(level6, LEVEL6)
		DMANIP(level7, LEVEL7)
		DMANIP(level8, LEVEL8)
		DMANIP(level9, LEVEL9)
		DMANIP(info, INFO)
		DMANIP(warn, WARN)
		DMANIP(crit, CRIT)
		DMANIP(repository, REPOSITORY)
		DMANIP(system, SYSTEM)
		DMANIP(exception, EXCEPTION)
		DMANIP(any, ANY)
#undef DMANIP

		std::ostream& printDate(Debug::type t, char brk = '/') noexcept;
		std::ostream& printTime(Debug::type t, char brk = ':') noexcept;
		std::ostream& printDateTime(Debug::type t) noexcept;

		std::ostream& pos(int x, int y) noexcept;

		const DebugStream& operator=(const DebugStream& r);

		inline void setLogName( const std::string& n ) noexcept
		{
			logname = n;
		}

		inline std::string  getLogName() const noexcept
		{
			return logname;
		}

	protected:
		void sbuf_overflow( const std::string& s ) noexcept;

		// private:
		/// The current debug level
		Debug::type dt = { Debug::NONE };
		/// The no-op stream.
		std::ostream nullstream;
		///
		struct debugstream_internal;
		///
		debugstream_internal* internal = { 0 };
		bool show_datetime = { true };
		bool show_logtype = { true };
		bool show_msec = { false };
		bool show_usec = { false };
		std::string fname = { "" };

		StreamEvent_Signal s_stream;
		std::string logname = { "" };

		bool isWriteLogFile = { false };
		bool onScreen = { true };

		Debug::verbosity verb = { 0 };
		Debug::verbosity vv = { 0 };

		std::vector<Label> labels;
		bool show_labels = { true };
		bool hide_label_key = { false };
};

// ------------------------------------------------------------------------------------------------
#endif
