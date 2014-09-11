#ifndef DEBUGEXTBUF_H
#define DEBUGEXTBUF_H

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

using std::ostream;
using std::streambuf;
using std::streamsize;
using std::filebuf;
using std::stringbuf;
using std::cerr;
using std::ios;

/*
ostream & operator<<(ostream & o, Debug::type t)
{
    return o << int(t);
}
*/

/** This is a streambuffer that never prints out anything, at least
    that is the intention. You can call it a no-op streambuffer, and
    the ostream that uses it will be a no-op stream.
*/
class nullbuf : public streambuf {
protected:
#ifndef MODERN_STL_STREAMS
    typedef char char_type;
    typedef int int_type;
    ///
    virtual int sync() { return 0; }
#endif
    ///
    virtual streamsize xsputn(char_type const *, streamsize n) {
        // fakes a purge of the buffer by returning n
        return n;
    }
#ifdef MODERN_STL_STREAMS
    ///
    virtual int_type overflow(int_type c = traits_type::eof()) {
        // fakes success by returning c
        return c == traits_type::eof() ? ' ' : c;
    }
#else
    ///
    virtual int_type overflow(int_type c = EOF) {
        // fakes success by returning c
        return c == EOF ? ' ' : c;
    }
#endif
};

/** A streambuf that sends the output to two different streambufs. These
    can be any kind of streambufs.
*/
class teebuf : public streambuf {
public:
    ///
    teebuf(streambuf * b1, streambuf * b2)
        : streambuf(), sb1(b1), sb2(b2) {}
protected:
#ifdef MODERN_STL_STREAMS
    ///
    virtual int sync() {
        sb2->pubsync();
        return sb1->pubsync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        sb2->sputn(p, n);
        return sb1->sputn(p, n);
    }
    ///
    virtual int_type overflow(int_type c = traits_type::eof()) {
        sb2->sputc(c);
        return sb1->sputc(c);
    }
#else
    typedef char char_type;
    typedef int int_type;
    ///
    virtual int sync() {
        sb2->sync();
        return sb1->sync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        sb2->xsputn(p, n);
        return sb1->xsputn(p, n);
    }
    ///
    virtual int_type overflow(int_type c = EOF) {
        sb2->overflow(c);
        return sb1->overflow(c);
    }
#endif
private:
    ///
    streambuf * sb1;
    ///
    streambuf * sb2;
};

/** A streambuf that sends the output to two different streambufs. These
    can be any kind of streambufs.
*/
class threebuf : public streambuf {
public:
    ///
    threebuf(streambuf * b1, streambuf * b2, streambuf * b3)
        : streambuf(), sb1(b1), sb2(b2), sb3(b3) {}
protected:
#ifdef MODERN_STL_STREAMS
    ///
    virtual int sync() {
        sb2->pubsync();
        return sb1->pubsync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        sb2->sputn(p, n);
		sb3->sputn(p, n);
        return sb1->sputn(p, n);
    }
    ///
    virtual int_type overflow(int_type c = traits_type::eof()) {
        sb2->sputc(c);
		sb3->sputc(c);
        return sb1->sputc(c);
    }
#else
    typedef char char_type;
    typedef int int_type;
    ///
    virtual int sync() {
        sb2->sync();
		sb3->sync();
        return sb1->sync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        sb2->xsputn(p, n);
		sb2->xsputn(p, n);
        return sb1->xsputn(p, n);
    }
    ///
    virtual int_type overflow(int_type c = EOF) {
        sb2->overflow(c);
		sb3->owerflow(c);
        return sb1->overflow(c);
    }
#endif
private:
    ///
    streambuf * sb1;
    ///
    streambuf * sb2;
	///
	streambuf * sb3;
};

///
class debugbuf : public streambuf {
public:
    ///
    debugbuf(streambuf * b)
        : streambuf(), sb(b) {}
protected:
#ifdef MODERN_STL_STREAMS
    ///
    virtual int sync() {
        return sb->pubsync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        return sb->sputn(p, n);
    }
    ///
    virtual int_type overflow(int_type c = traits_type::eof()) {
        return sb->sputc(c);
    }
#else
    typedef char char_type;
    typedef int int_type;
    ///
    virtual int sync() {
        return sb->sync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        return sb->xsputn(p, n);
    }
    ///
    virtual int_type overflow(int_type c = EOF) {
        return sb->overflow(c);
    }
#endif
private:
    ///
    streambuf * sb;
};

///
class stringsigbuf : public streambuf {
public:
    stringsigbuf():sb(new stringbuf())
	{
	}

    ~stringsigbuf()
	{
		if( sb )
		{
			delete sb;
			sb = 0;
		}
	}

    ///
    stringsigbuf( stringbuf* b )
        : streambuf(), sb(b) {}

    typedef sigc::signal<void,const std::string&> StrBufOverflow_Signal;
    inline StrBufOverflow_Signal signal_overflow(){ return s_overflow; }

protected:
#ifdef MODERN_STL_STREAMS
    ///
    virtual int sync() {
        UniSetTypes::uniset_rwmutex_wrlock l(mut);
        return sb->pubsync();
    }

    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        UniSetTypes::uniset_rwmutex_wrlock l(mut);
        streamsize r = sb->sputn(p, n);
        s_overflow.emit( sb->str() );
        sb->str("");
        return r;
    }
    ///
    virtual int_type overflow(int_type c = traits_type::eof()) {
        int_type r = sb->sputc(c);
        if( r == '\n' )
        {
            UniSetTypes::uniset_rwmutex_wrlock l(mut);
            s_overflow.emit( sb->str() );
            sb->str("");
        }
        return r;
    }
#else
    typedef char char_type;
    typedef int int_type;
    ///
    virtual int sync() {
        return sb->sync();
    }
    ///
    virtual streamsize xsputn(char_type const * p, streamsize n) {
        return sb->xsputn(p, n);
    }

    virtual int_type overflow(int_type c = EOF) {
        int_type r = sb->overflow(c);
        if( r == '\n' )
        {
            UniSetTypes::uniset_rwmutex_wrlock l(mut);
            s_overflow.emit( sb->str() );
            sb->str("");
        }
        return r;
    }
#endif
private:
    ///
    StrBufOverflow_Signal s_overflow;
    stringbuf* sb;
    UniSetTypes::uniset_rwmutex mut;
};
//--------------------------------------------------------------------------
/// So that public parts of DebugStream does not need to know about filebuf
struct DebugStream::debugstream_internal {
    /// Used when logging to file.
    filebuf fbuf;
    stringsigbuf sbuf;
    nullbuf nbuf;
};
//--------------------------------------------------------------------------
#endif