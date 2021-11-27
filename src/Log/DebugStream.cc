// Created by Lars Gullik Bj�nnes
// Copyright 1999 Lars Gullik Bj�nnes (larsbj@lyx.org)
// Released into the public domain.

// Primarily developed for use in the LyX Project http://www.lyx.org/
// but should be adaptable to any project.

// (c) 2002 adapted for UniSet by Lav, GNU LGPL license

//#include "DebugStream.h"
#include "Debug.h"
#include "Mutex.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include "DebugExtBuf.h"
#include "UniSetTypes.h"


using std::ostream;
using std::streambuf;
using std::streamsize;
using std::filebuf;
using std::stringbuf;
using std::cerr;
using std::ios;
//--------------------------------------------------------------------------
/// Constructor, sets the debug level to t.
DebugStream::DebugStream(Debug::type t, Debug::verbosity v)
	: /* ostream(new debugbuf(cerr.rdbuf())),*/
	  dt(t), nullstream(new nullbuf), internal(new debugstream_internal),
	  show_datetime(true), show_logtype(true),
	  fname(""),
	  logname(""),
	  verb(v)
{
	delete rdbuf(new teebuf(cerr.rdbuf(), &internal->sbuf));
	internal->sbuf.signal_overflow().connect(sigc::mem_fun(*this, &DebugStream::sbuf_overflow));
}

//--------------------------------------------------------------------------
/// Constructor, sets the log file to f, and the debug level to t.
DebugStream::DebugStream(char const* f, Debug::type t, bool truncate )
	: /* ostream(new debugbuf(cerr.rdbuf())), */
	  dt(t), nullstream(new nullbuf),
	  internal(new debugstream_internal),
	  show_datetime(true), show_logtype(true),
	  fname(""),
	  logname("")
{
	std::ios_base::openmode mode = ios::out;
	mode |= truncate ? ios::trunc : ios::app;

	internal->fbuf.open(f, mode);
	delete rdbuf(new threebuf(cerr.rdbuf(),
							  &internal->fbuf, &internal->sbuf));

	internal->sbuf.signal_overflow().connect(sigc::mem_fun(*this, &DebugStream::sbuf_overflow));
}
//--------------------------------------------------------------------------
void DebugStream::sbuf_overflow( const std::string& s ) noexcept
{
	try
	{
		s_stream.emit(s);
	}
	catch(...) {}
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
	if( &r == this )
		return *this;

	dt = r.dt;
	verb = r.verb;
	show_datetime = r.show_datetime;
	show_logtype = r.show_logtype;
	show_msec = r.show_msec;
	show_usec = r.show_usec;
	fname = r.fname;

	if( !r.fname.empty() )
		logFile(fname);

	// s_stream = r.s_stream;
	return *this;
}
//--------------------------------------------------------------------------
/// Sets the debugstreams' logfile to f.
void DebugStream::logFile( const std::string& f, bool truncate )
{
	if( !f.empty() && f != fname )
		fname = f;

	isWriteLogFile = !f.empty();

	if( internal )
	{
		internal->fbuf.close();
	}
	else
	{
		internal = new debugstream_internal;
	}

	if( !f.empty() )
	{
		std::ios_base::openmode mode = ios::out;
		mode |= truncate ? ios::trunc : ios::app;

		internal->fbuf.open(f.c_str(), mode);

		if( onScreen )
		{
			delete rdbuf(new threebuf(cerr.rdbuf(),
									  &internal->fbuf, &internal->sbuf));
		}
		else
		{
			// print to cerr disabled
			delete rdbuf(new teebuf(&internal->fbuf, &internal->sbuf));
		}
	}
	else
	{
		if( onScreen )
			delete rdbuf(new teebuf(cerr.rdbuf(), &internal->sbuf));
		else
			delete rdbuf(&internal->sbuf);
	}
}
//--------------------------------------------------------------------------
void DebugStream::enableOnScreen()
{
	onScreen = true;
	// reopen streams
	logFile(fname, false);
}
//--------------------------------------------------------------------------
void DebugStream::disableOnScreen()
{
	onScreen = false;
	// reopen streams
	logFile(fname, false);
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::debug(Debug::type t) noexcept
{
	if( (dt & t) && (vv <= verb) )
	{
		uniset::ios_fmt_restorer ifs(*this);

		if( show_datetime )
			printDateTime(t);

		if( show_logtype )
			*this << "(" << std::setfill(' ') << std::setw(6) << t << "):  "; // "):\t";

		if( show_labels )
		{
			for( const auto& l : labels )
			{
				*this << "[";

				if( !hide_label_key )
					*this << l.first << "=";

				*this << l.second << "]";
			}
		}

		return *this;
	}

	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::operator()(Debug::type t) noexcept
{
	if( (dt & t) && (vv <= verb) )
		return *this;

	return nullstream;
}
//--------------------------------------------------------------------------
DebugStream& DebugStream::V( Debug::verbosity v ) noexcept
{
	vv = v;
	return *this;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::printDate(Debug::type t, char brk) noexcept
{
	if( (dt & t) && (vv <= verb) )
	{
		uniset::ios_fmt_restorer ifs(*this);

		std::time_t tv = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm tms;
		if( show_localtime )
		    localtime_r(&tv, &tms);
		else
		    gmtime_r(&tv, &tms);

#if __GNUC__ >= 5
		std::ostringstream fmt;
		fmt << "%Od" << brk << "%Om" << brk << "%Y";
		return (*this) << std::put_time(&tms, fmt.str().c_str());
#else
		return (*this) << std::setw(2) << std::setfill('0') << tms.tm_mday << brk
			   << std::setw(2) << std::setfill('0') << tms.tm_mon + 1 << brk
			   << std::setw(4) << std::setfill('0') << tms.tm_year + 1900;
#endif
	}

	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::printTime(Debug::type t, char brk) noexcept
{
	if( (dt & t) && (vv <= verb) )
	{
		uniset::ios_fmt_restorer ifs(*this);

		timespec tv = uniset::now_to_timespec(); // gettimeofday(tv,0);
		std::tm tms;
		if( show_localtime )
		    localtime_r(&tv.tv_sec, &tms);
        else
		    gmtime_r(&tv.tv_sec, &tms);

#if __GNUC__ >= 5
		std::ostringstream fmt;
		fmt << "%OH" << brk << "%OM" << brk << "%OS";
		(*this) << std::put_time(&tms, fmt.str().c_str());
#else
		*this << std::setw(2) << std::setfill('0') << tms.tm_hour << brk
			  << std::setw(2) << std::setfill('0') << tms.tm_min << brk
			  << std::setw(2) << std::setfill('0') << tms.tm_sec;
#endif

		if( show_usec )
			(*this) << "." << std::setw(6) << (tv.tv_nsec / 1000);
		else if( show_msec )
			(*this) << "." << std::setw(3) << (tv.tv_nsec / 1000000);

		return *this;
	}

	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::printDateTime(Debug::type t) noexcept
{
	if( (dt & t) && (vv <= verb) )
	{
		uniset::ios_fmt_restorer ifs(*this);

		timespec tv = uniset::now_to_timespec(); // gettimeofday(tv,0);
		std::tm tms;
		if( show_localtime )
		    localtime_r(&tv.tv_sec, &tms);
        else
		    gmtime_r(&tv.tv_sec, &tms);

#if __GNUC__ >= 5
		*this << std::put_time(&tms, "%Od/%Om/%Y %OH:%OM:%OS");
#else
		*this << std::setw(2) << std::setfill('0') << tms.tm_mday << "/"
			  << std::setw(2) << std::setfill('0') << tms.tm_mon + 1 << "/"
			  << std::setw(4) << std::setfill('0') << tms.tm_year + 1900 << " "
			  << std::setw(2) << std::setfill('0') << tms.tm_hour << ":"
			  << std::setw(2) << std::setfill('0') << tms.tm_min << ":"
			  << std::setw(2) << std::setfill('0') << tms.tm_sec;
#endif

		if( show_usec )
			(*this) << "." << std::setw(6) << std::setfill('0') << (tv.tv_nsec / 1000);
		else if( show_msec )
			(*this) << "." << std::setw(3) << std::setfill('0') << (tv.tv_nsec / 1000000);

		return *this;
	}

	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::pos(int x, int y) noexcept
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
void DebugStream::addLabel( const std::string& key, const std::string& value ) noexcept
{
	auto it = std::find_if(labels.begin(), labels.end(), [key] (const Label & l)
	{
		return l.first == key;
	} );

	if( it == labels.end() )
		labels.emplace_back(key, value);
}
//--------------------------------------------------------------------------
void DebugStream::delLabel( const std::string& key ) noexcept
{
	auto it = std::find_if(labels.begin(), labels.end(), [key] (const Label & l)
	{
		return l.first == key;
	} );

	if( it != labels.end() )
		labels.erase(it);
}
//--------------------------------------------------------------------------
void DebugStream::cleanupLabels() noexcept
{
	labels.clear();
}
//--------------------------------------------------------------------------
std::vector<DebugStream::Label> DebugStream::getLabels() noexcept
{
	return labels;
}
//--------------------------------------------------------------------------
