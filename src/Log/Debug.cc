/* This file is part of
* ======================================================
*
*           LyX, The Document Processor
*
*           Copyright 1999-2000 The LyX Team.
*
* ====================================================== */
// (c) 2002 adapted for UniSet by Lav, GNU LGPL license

#ifdef __GNUG__
#pragma implementation
#endif

#include <iomanip>
#include <sstream>

#include "Debug.h"
//#include "gettext.h"

using std::ostream;
using std::setw;
using std::endl;

struct error_item
{
	Debug::type level;
	char const* name;
	char const* desc;
};

static error_item errorTags[] =
{
	{ Debug::NONE,      "none",      ("No debugging message")},
	{ Debug::INIT,      "init",      ("Program initialisation")},
	{ Debug::INFO,      "info",      ("General information")},
	{ Debug::WARN,      "warn",      ("Warning meessages")},
	{ Debug::CRIT,      "crit",      ("Critical messages")},
	{ Debug::SYSTEM,    "system",    ("UniSet system messages")},
	{ Debug::LEVEL1,    "level1",    ("UniSet debug level1")},
	{ Debug::LEVEL2,    "level2",    ("UniSet debug level2")},
	{ Debug::LEVEL3,    "level3",    ("UniSet debug level3")},
	{ Debug::LEVEL4,    "level4",    ("UniSet debug level4")},
	{ Debug::LEVEL5,    "level5",    ("UniSet debug level5")},
	{ Debug::LEVEL6,    "level6",    ("UniSet debug level6")},
	{ Debug::LEVEL7,    "level7",    ("UniSet debug level7")},
	{ Debug::LEVEL8,    "level8",    ("UniSet debug level8")},
	{ Debug::LEVEL9,    "level9",    ("UniSet debug level9")},
	{ Debug::REPOSITORY, "repository", ("UniSet repository messages")},
	{ Debug::ANY,       "any",       ("All debugging messages")},
	{ Debug::EXCEPTION, "exception", ("Exception debug messages")},

};


static const int numErrorTags = sizeof(errorTags) / sizeof(error_item);


Debug::type const Debug::ANY = Debug::type(
								   Debug::INFO | Debug::INIT | Debug::WARN | Debug::CRIT |
								   Debug::LEVEL1 | Debug::LEVEL2 | Debug::LEVEL3 | Debug::LEVEL4 |
								   Debug::LEVEL5 | Debug::LEVEL6 | Debug::LEVEL7 | Debug::LEVEL8 |
								   Debug::LEVEL9 | Debug::REPOSITORY | Debug::SYSTEM |
								   Debug::EXCEPTION );


Debug::type Debug::value( std::string const& val)
{
	type l = Debug::NONE;
	std::string v(val);

	while (!v.empty())
	{
		std::string::size_type st = v.find(',');
		//string tmp(lowercase(v.substr(0, st)));
		std::string tmp(v.substr(0, st));


		if(tmp.empty())
			break;

		bool del = false;

		if( tmp[0] == '-' )
		{
			del = true;
			tmp = tmp.substr(1, tmp.size());
		}

		for (int i = 0 ; i < numErrorTags ; ++i)
			if (tmp == errorTags[i].name)
			{
				if( del )
					l = Debug::type(l & ~(errorTags[i].level));
				else
					l |= errorTags[i].level;

				break;
			}

		if (st == std::string::npos) break;

		v.erase(0, st + 1);
	}

	return l;
}


void Debug::showLevel(ostream& o, Debug::type level) noexcept
{
	// Show what features are traced
	for (int i = 0 ; i < numErrorTags ; ++i)
		if (errorTags[i].level != Debug::ANY
				&& errorTags[i].level != Debug::NONE
				&& errorTags[i].level & level)
			o << "Debugging `" << errorTags[i].name
			  << "' (" << errorTags[i].desc << ')' << endl;
}

void Debug::showTags(ostream& os) noexcept
{
	for (int i = 0 ; i < numErrorTags ; ++i)
		os << setw(7) << errorTags[i].level
		   << setw(10) << errorTags[i].name
		   << "  " << errorTags[i].desc << '\n';

	try
	{
		os.flush();
	}
	catch(...) {}
}

std::ostream& operator<<(std::ostream& os, Debug::type level ) noexcept
{

	for (int i = 0 ; i < numErrorTags ; ++i)
	{
		if( errorTags[i].level & level)        // errorTags[i].level != Debug::ANY && errorTags[i].level != Debug::NONE
			return os << errorTags[i].name; // << "(" << int(level) << ")";
	}

	return os << "???Debuglevel"; // << "(" << int(level) << ")";
}

std::string Debug::str( Debug::type level ) noexcept
{
	if( level == Debug::NONE )
		return "NONE";

	if( level == Debug::ANY )
		return "ANY";

	std::ostringstream s;
	bool first = true;

	for (int i = 0 ; i < numErrorTags ; ++i)
	{
		if (errorTags[i].level != Debug::ANY
				&& errorTags[i].level != Debug::NONE
				&& errorTags[i].level & level)
		{
			if( first )
			{
				first = false;
				s << errorTags[i].name;
			}
			else
				s << "," << errorTags[i].name;
		}
	}

	try
	{
		return s.str();
	}
	catch(...) {}

	return "";
}


//DebugStream ulog;
