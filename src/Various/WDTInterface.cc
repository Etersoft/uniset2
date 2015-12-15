// --------------------------------------------------------------------------
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
