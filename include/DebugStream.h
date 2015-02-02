// -*- C++ -*-

// Created by Lars Gullik BjЬnnes
// Copyright 1999 Lars Gullik BjЬnnes (larsbj@lyx.org)
// Released into the public domain.

// Implemented and tested on g++ 2.7.2.3

// Primarily developed for use in the LyX Project http://www.lyx.org/
// but should be adaptable to any project.

// (c) 2002 adapted for UniSet by Lav, GNU GPL license
// Modify for UniSet by pv@eterspft.ru, GNU GPL license

#ifndef DEBUGSTREAM_H
#define DEBUGSTREAM_H

//#ifdef __GNUG__
//#pragma interface
//#endif

#include <iostream>
#include <string>
#include <sigc++/sigc++.h>
#include "Debug.h"

#ifdef TEST_DEBUGSTREAM
#include <string>
struct Debug {
    enum type {
        NONE = 0,
        INFO       = (1 << 0),   // 1
        WARN       = (1 << 1),   // 2
        CRIT       = (1 << 2)   // 4
    };
    static const type ANY = type(INFO | WARN | CRIT);
    static Debug::type value(std::string const & val) {
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
    if (debug..is_info()) {
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
    explicit DebugStream(Debug::type t = Debug::NONE);

    /// Constructor, sets the log file to f, and the debug level to t.
    explicit
    DebugStream(char const * f, Debug::type t = Debug::NONE, bool truncate=false );

    ///
    virtual ~DebugStream();

    typedef sigc::signal<void,const std::string&> StreamEvent_Signal;
    StreamEvent_Signal signal_stream_event();

    /// Sets the debug level to t.
    void level(Debug::type t) {
        dt = Debug::type(t & Debug::ANY);
    }

    /// Returns the current debug level.
    Debug::type level() const {
        return dt;
    }

    /// Adds t to the current debug level.
    void addLevel(Debug::type t) {
        dt = Debug::type(dt | t);
    }

    /// Deletes t from the current debug level.
    void delLevel(Debug::type t) {
        dt = Debug::type(dt & ~t);
    }

    /// Sets the debugstreams' logfile to f.
    virtual void logFile( const std::string& f, bool truncate=false );

    inline std::string getLogFile(){ return fname; }

    /// Returns true if t is part of the current debug level.
    inline bool debugging(Debug::type t = Debug::ANY) const
    {  return (dt & t); }

    /** Returns the no-op stream if t is not part of the
        current debug level otherwise the real debug stream
        is used.
    */
    std::ostream & debug(Debug::type t = Debug::ANY);
//        if (dt & t) return *this;
//        return nullstream;
//    }


    /** This is an operator to give a more convenient use:
        dbgstream[Debug::INFO] << "Info!\n";
        Вывод осуществляется с датой и временем (если они не отключены)
    */
    std::ostream & operator[](Debug::type t) {
        return debug(t);
    }

    /**
        Вывод продолжения логов (без даты и времени)
    */
    inline std::ostream& to_end(Debug::type t)
    { return this->operator()(t); }

    /**
        Вывод продолжения логов (без даты и времени) "log()"
    */
    std::ostream& operator()(Debug::type t);

    inline void showDateTime(bool s)
    { show_datetime = s; }

    inline void showLogType(bool s)
    { show_logtype = s; }

    inline std::ostream& log(Debug::type l)
    {
        return this->operator[](l);
    }

// короткие функции (для удобства)
// log.level1()  - вывод с датой и временем  "date time [LEVEL] ...",
//    если вывод даты и времени не выключен при помощи showDateTime(false)
// if( log.is_level1() ) - проверка включён ли лог.."

#define DMANIP(FNAME,LEVEL) \
    inline std::ostream& FNAME( bool showdatetime=true ) \
    {\
        if( showdatetime )\
            return operator[](Debug::LEVEL); \
        return  operator()(Debug::LEVEL); \
    } \
\
    inline bool is_##FNAME() \
    { return debugging(Debug::LEVEL); }

    DMANIP(level1,LEVEL1)
    DMANIP(level2,LEVEL2)
    DMANIP(level3,LEVEL3)
    DMANIP(level4,LEVEL4)
    DMANIP(level5,LEVEL5)
    DMANIP(level6,LEVEL6)
    DMANIP(level7,LEVEL7)
    DMANIP(level8,LEVEL8)
    DMANIP(level9,LEVEL9)
    DMANIP(info,INFO)
    DMANIP(warn,WARN)
    DMANIP(crit,CRIT)
    DMANIP(repository,REPOSITORY)
    DMANIP(system,SYSTEM)
    DMANIP(exception,EXCEPTION)
    DMANIP(any,ANY)
#undef DMANIP

    std::ostream& printDate(Debug::type t, char brk='/');
    std::ostream& printTime(Debug::type t, char brk=':');
    std::ostream& printDateTime(Debug::type t);

    std::ostream& pos(int x, int y);

    const DebugStream &operator=(const DebugStream& r);

    inline void setLogName( const std::string& n ){ logname = n; }
    inline std::string  getLogName(){ return logname; }

protected:
    void sbuf_overflow( const std::string& s );

// private:
    /// The current debug level
    Debug::type dt;
    /// The no-op stream.
    std::ostream nullstream;
    ///
    struct debugstream_internal;
    ///
    debugstream_internal * internal;
    bool show_datetime;
    bool show_logtype;
    std::string fname;

    StreamEvent_Signal s_stream;
    std::string logname; 
};

#endif
