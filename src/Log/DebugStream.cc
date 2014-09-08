// Created by Lars Gullik Bj�nnes
// Copyright 1999 Lars Gullik Bj�nnes (larsbj@lyx.org)
// Released into the public domain.

// Primarily developed for use in the LyX Project http://www.lyx.org/
// but should be adaptable to any project.

// (c) 2002 adapted for UniSet by Lav, GNU GPL license

//#define TEST_DEBUGSTREAM


#ifdef __GNUG__
#pragma implementation
#endif

//#include "DebugStream.h"
#include "Debug.h"
#include "Mutex.h"

//�Since the current C++ lib in egcs does not have a standard implementation
// of basic_streambuf and basic_filebuf we don't have to include this
// header.
//#define MODERN_STL_STREAMS
#ifdef MODERN_STL_STREAMS
#include <fstream>
#endif
#include <iostream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <iomanip>
#include "DebugExtBuf.h"

using std::ostream;
using std::streambuf;
using std::streamsize;
using std::filebuf;
using std::stringbuf;
using std::cerr;
using std::ios;
//--------------------------------------------------------------------------

/// Constructor, sets the debug level to t.
DebugStream::DebugStream(Debug::type t)
    : ostream(new debugbuf(cerr.rdbuf())),
      dt(t), nullstream(new nullbuf), internal(new debugstream_internal),
      show_datetime(true),
      fname(""),
      logname("")
{
    delete rdbuf(new teebuf(cerr.rdbuf(),&internal->sbuf));
    internal->sbuf.signal_overflow().connect(sigc::mem_fun(*this, &DebugStream::sbuf_overflow));
}

//--------------------------------------------------------------------------
/// Constructor, sets the log file to f, and the debug level to t.
DebugStream::DebugStream(char const * f, Debug::type t)
    : ostream(new debugbuf(cerr.rdbuf())),
      dt(t), nullstream(new nullbuf),
      internal(new debugstream_internal),
      show_datetime(true),
      fname(""),
      logname("")
{
    internal->fbuf.open(f, ios::out|ios::app);
    delete rdbuf(new threebuf(cerr.rdbuf(),
                &internal->fbuf,&internal->sbuf));

    internal->sbuf.signal_overflow().connect(sigc::mem_fun(*this, &DebugStream::sbuf_overflow));
}
//--------------------------------------------------------------------------
void DebugStream::sbuf_overflow( const std::string& s )
{
    s_stream.emit(s);
}
//--------------------------------------------------------------------------
DebugStream::~DebugStream()
{
    delete nullstream.rdbuf(0); // Without this we leak
    delete rdbuf(0);            // Without this we leak
    delete internal;
}

//--------------------------------------------------------------------------
const DebugStream& DebugStream::operator=( const DebugStream& r )
{
    if( r == *this )
        return *this;

    dt = r.dt;
    show_datetime = r.show_datetime;
    fname = r.fname;
    if( !r.fname.empty() )
        logFile(fname);

    // s_stream = r.s_stream;
    return *this;
}
//--------------------------------------------------------------------------
/// Sets the debugstreams' logfile to f.
void DebugStream::logFile( const std::string& f )
{
    fname = f;
    if( internal ) {
        internal->fbuf.close();
    } else {
        internal = new debugstream_internal;
    }

    if( !f.empty() )
    {
        internal->fbuf.open(f.c_str(), ios::out|ios::app);
        delete rdbuf(new threebuf(cerr.rdbuf(),
                &internal->fbuf,&internal->sbuf));
    }
    else
        delete rdbuf(new teebuf(cerr.rdbuf(),&internal->sbuf));
}
//--------------------------------------------------------------------------
std::ostream & DebugStream::debug(Debug::type t) 
{
    if(dt & t)
    {
        if( show_datetime )
            printDateTime(t);
        *this << "(" << std::setfill(' ') << std::setw(6) << t << "):  "; // "):\t";
        return *this;
    }

    return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::operator()(Debug::type t)
{
    if(dt & t)
        return *this;

    return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::printDate(Debug::type t, char brk)
{
    if(dt && t)
    {
        time_t GMTime = time(NULL);
        struct tm *tms = localtime(&GMTime);
        return *this << std::setw(2) << std::setfill('0') << tms->tm_mday << brk
                      << std::setw(2) << std::setfill('0') << tms->tm_mon+1 << brk
                     << std::setw(4) << std::setfill('0') << tms->tm_year+1900;
    }
    
    return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::printTime(Debug::type t, char brk)
{
    if(dt && t)
    {
        time_t GMTime = time(NULL);
        struct tm *tms = localtime(&GMTime);
        return *this << std::setw(2) << std::setfill('0') << tms->tm_hour << brk
                     << std::setw(2) << std::setfill('0') << tms->tm_min << brk
                     << std::setw(2) << std::setfill('0') << tms->tm_sec;
    }
    
    return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::printDateTime(Debug::type t)
{
    if(dt & t)
    {
        time_t GMTime = time(NULL);
        struct tm *tms = localtime(&GMTime);
        return *this << std::setw(2) << std::setfill('0') << tms->tm_mday << "/"
                     << std::setw(2) << std::setfill('0') << tms->tm_mon+1 << "/"
                     << std::setw(4) << std::setfill('0') << tms->tm_year+1900 << " "
                     << std::setw(2) << std::setfill('0') << tms->tm_hour << ":"
                     << std::setw(2) << std::setfill('0') << tms->tm_min << ":"
                     << std::setw(2) << std::setfill('0') << tms->tm_sec;
    }
    
    return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::pos(int x, int y)
{
    if( !dt )
        return nullstream;

    return *this << "\033[" << y << ";" << x << "f";
}

//--------------------------------------------------------------------------
DebugStream::StreamEvent_Signal DebugStream::signal_stream_event()
{
    return s_stream;
}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#ifdef TEST_DEBUGSTREAM

// Example debug stream
DebugStream debugstream;

int main(int, char **)
{
    /**
       I have been running some tests on this to see how much overhead
       this kind of permanent debug code has. My conclusion is: not
       much. In all, but the most time critical code, this will have
       close to no impact at all.

       In the tests that I have run the use of
       if (debugstream.debugging(DebugStream::INFO))
       debugstream << "some debug\n";
       has close to no overhead when the debug level is not
       DebugStream::INFO.

       The overhead for
       debugstream.debug(DebugStream::INFO) << "some debug\n";
       is also very small when the debug level is not
       DebugStream::INFO. However the overhead for this will increase
       if complex debugging information is output.

       The overhead when the debug level is DebugStream::INFO can be
       significant, but since we then are running in debug mode it is
       of no concern.

       Why should we use this instead of the class Error that we already
       have? First of all it uses C++ iostream and constructs, secondly
       it will be a lot easier to output the debug info that we need
       without a lot of manual conversions, thirdly we can now use
       iomanipulators and the complete iostream formatting functions.
       pluss it will work for all types that have a operator<<
       defined, and can be used in functors that take a ostream & as
       parameter. And there should be less need for temporary objects.
       And one nice bonus is that we get a log file almost for
       free.

       Some of the names are of course open to modifications. I will try
       to use the names we already use in LyX.
    */
    // Just a few simple debugs to show how it can work.
    debugstream << "Debug level set to Debug::NONE\n";
    if (debugstream.debugging()) {
        debugstream << "Something must be debugged\n";
    }
    debugstream.debug(Debug::WARN) << "more debug(WARN)\n";
    debugstream.debug(Debug::INFO) << "even more debug(INFO)\n";
    debugstream.debug(Debug::CRIT) << "even more debug(CRIT)\n";
    debugstream.level(Debug::value("INFO"));
    debugstream << "Setting debug level to Debug::INFO\n";
    if (debugstream.debugging()) {
        debugstream << "Something must be debugged\n";
    }
    debugstream.debug(Debug::WARN) << "more debug(WARN)\n";
    debugstream.debug(Debug::INFO) << "even more debug(INFO)\n";
    debugstream.debug(Debug::CRIT) << "even more debug(CRIT)\n";
    debugstream.addLevel(Debug::type(Debug::CRIT |
                     Debug::WARN));
    debugstream << "Adding Debug::CRIT and Debug::WARN\n";
    debugstream[Debug::WARN] << "more debug(WARN)\n";
    debugstream[Debug::INFO] << "even more debug(INFO)\n";
    debugstream[Debug::CRIT] << "even more debug(CRIT)\n";
    debugstream.delLevel(Debug::INFO);
    debugstream << "Removing Debug::INFO\n";
    debugstream[Debug::WARN] << "more debug(WARN)\n";
    debugstream[Debug::INFO] << "even more debug(INFO)\n";
    debugstream[Debug::CRIT] << "even more debug(CRIT)\n";
    debugstream.logFile("logfile");
    debugstream << "Setting logfile to \"logfile\"\n";
    debugstream << "Value: " << 123 << " " << "12\n";
    int i = 0;
    int * p = new int;
    // note: the (void*) is needed on g++ 2.7.x since it does not
    // support partial specialization. In egcs this should not be
    // needed.
    debugstream << "automatic " << &i
            << ", free store " << p << endl;
    delete p;
    /*
    for (int j = 0; j < 200000; ++j) {
        DebugStream tmp;
        tmp << "Test" << endl;
    }
    */
}
#endif
