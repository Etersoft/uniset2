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
#ifndef UTypes_H_
#define UTypes_H_
// --------------------------------------------------------------------------
#include <string>
#include "Configuration.h"
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace UTypes
{
const long DefaultID = uniset::DefaultObjectId;
const long DefaultSupplerID = uniset::AdminID;

struct Params
{
	static const int max = 20;

	Params(): argc(0)
	{
		memset(argv, 0, sizeof(argv));
	}

	bool add( char* s )
	{
		if( argc < Params::max )
		{
			argv[argc++] = uniset::uni_strdup(s);
			return true;
		}

		return false;
	}

	bool add_str( const std::string s )
	{
		if( argc < Params::max )
		{
			argv[argc++] = uniset::uni_strdup(s);
			return true;
		}

		return false;
	}

	int argc;
	char* argv[max];

	static Params inst()
	{
		return Params();
	}
};

struct ShortIOInfo
{
	long value;
	unsigned long tv_sec;
	unsigned long tv_nsec;
	long supplier;
};
}

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
