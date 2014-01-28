// -*- C++ -*-
/* This file is part of
 * ====================================================== 
 * 
 *           LyX, The Document Processor
 *        
 *           Copyright 1995 Matthias Ettrich
 *           Copyright 1995-2000 The LyX Team.
 *
 * ====================================================== */
// (c) 2002 adapted for UniSet by Lav, GNU GPL license

#ifndef LYXDEBUG_H
#define LYXDEBUG_H

//#ifdef __GNUG__
//#pragma interface
//#endif

#if __GNUC__ > 2
#define MODERN_STL_STREAMS
#endif

#include <iosfwd>

#include <string>
//#include <lstrings.h>
//#ifndef _
//	#define _(n) n
//#endif

using std::string;

/** Ideally this should have been a namespace, but since we try to be
    compilable on older C++ compilators too, we use a struct instead.
    This is all the different debug levels that we have.
*/
struct Debug {
	///
	enum type {
		///
		NONE = 0,
		///
		INFO	= (1 << 0),   // 1
		///
		INIT	= (1 << 1),   // 2
		///
		WARN	= (1 << 2),   // 4
		///
		CRIT	= (1 << 3),   // 8
		///
		LEVEL1	= (1 << 4),   // 16
		///
		LEVEL2	= (1 << 5),   // 32
		///
		LEVEL3	= (1 << 6),   // 64
		///
		LEVEL4	= (1 << 7),   // 128
		///
		LEVEL5	= (1 << 8),   // 256
		///
		LEVEL6	= (1 << 9),   // 512
		///
		LEVEL7	= (1 << 10),  // 1024
		///
		LEVEL8	= (1 << 11),  // 2048
		///
		LEVEL9	= (1 << 12),  // 4096
		///
		REPOSITORY = (1 << 13),
		///
		SYSTEM	   = (1 << 14),
		///
		EXCEPTION  = (1 << 15)

	};
	///
	static type const ANY;

	///
//	friend inline void operator|=(Debug::type & d1, Debug::type d2);

	/** A function to convert symbolic string names on debug levels
	    to their numerical value.
	*/
	static Debug::type value(string const & val);

	/** Display the tags and descriptions of the current debug level
	    of ds
	*/
	static void showLevel(std::ostream & o, type level);

	/** show all the possible tags that can be used for debugging */
	static void showTags(std::ostream & o);

	friend std::ostream& operator<<(std::ostream& os, Debug::type level );
};



inline
void operator|=(Debug::type & d1, Debug::type d2)
{
	d1 = static_cast<Debug::type>(d1 | d2);
}


#include "DebugStream.h"

std::ostream & operator<<(std::ostream & o, Debug::type t);
//extern DebugStream unideb;

#endif
