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
// --------------------------------------------------------------------------
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "WDTInterface.h"
// --------------------------------------------------------------------------
#define CMD_PING "1"
#define CMD_STOP "V"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace uniset
{
	// --------------------------------------------------------------------------
	WDTInterface::WDTInterface( const std::string& _dev ):
		dev(_dev)
	{
	}
	// --------------------------------------------------------------------------
	WDTInterface::~WDTInterface()
	{
	}
	// --------------------------------------------------------------------------
	bool WDTInterface::ping()
	{
		int fd = open( dev.c_str(), O_WRONLY );

		if( fd < 0 )
		{
			cerr << ": Unable to open device " << dev << " with err: " << strerror(errno) << endl;
			return false;
		}

		int ret = write(fd, (void*)CMD_PING, sizeof(CMD_PING));

		if( ret == -1 )
		{
			cerr << ": Unable to open device " << dev << " with err: " << strerror(errno) << endl;
			close(fd);
			return false;
		}

		close(fd);
		return true;
	}
	// --------------------------------------------------------------------------
	bool WDTInterface::stop()
	{
		int fd = open( dev.c_str(), O_WRONLY );

		if( fd < 0 )
		{
			cerr << ": Unable to open device " << dev << " with err: " << strerror(errno) << endl;
			return false;
		}

		int ret = write(fd, (void*)CMD_STOP, sizeof(CMD_STOP));

		if( ret == -1 )
		{
			cerr << ": Unable to open device " << dev << " with err: " << strerror(errno) << endl;
			close(fd);
			return false;
		}

		close(fd);
		return true;
	}
	// ------------------------------------------------------------------------------------------
} // end of namespace uniset
