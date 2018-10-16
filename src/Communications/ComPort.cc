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
// --------------------------------------------------------------------------------
/*! \file
 *  \brief Обращения к последовательным интерфейсам
 *  \author Nick Lezzhov, Pavel Veynerman, Vitaly Lipatov
 */
/**************************************************************************/
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include "Exceptions.h"
#include "ComPort.h"
// --------------------------------------------------------------------------------
// почему-то в termios этого нет, но в ядре есть
#ifndef CMSPAR
#define CMSPAR    010000000000          /* mark or space (stick) parity */
#endif
// --------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------------
ComPort::~ComPort()
{
	//    printf("Destructor\n");
	if( fd > 0 )
	{
		tcsetattr(fd, TCSAFLUSH, &oldTermios);
		close(fd);
	}
}
// --------------------------------------------------------------------------------
ComPort::ComPort( const string& comDevice, bool nocreate ):
	curSym(0), bufLength(0), fd(-1), uTimeout(10000),
	waiting(true), dev(comDevice)
{
	if( !nocreate )
		openPort();
}
// --------------------------------------------------------------------------------
void ComPort::openPort()
{
	struct termios options;

	fd = open(dev.c_str(), O_RDWR | O_NOCTTY /*| O_NDELAY*/);

	if( fd == -1 )
	{
		string strErr = "Unable to open " + dev + " [Error: " + strerror(errno) + "]";
		throw uniset::SystemError(strErr.c_str());
	}

	/* Get the current options for the port */
	tcgetattr(fd, &options);

	oldTermios = options;

	cfsetispeed(&options, B19200);          /* Set the baud rates to 19200 */
	cfsetospeed(&options, B19200);

	cfmakeraw(&options);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN/*|NOFLUSH*/ | TOSTOP);
	options.c_cflag &= ~CSTOPB;
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK);
	options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IUCLC | IMAXBEL);
	options.c_oflag &= ~OPOST;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag &= ~HUPCL;
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 1;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag &= ~PARENB;
	options.c_iflag &= ~INPCK;
	options.c_cflag |= (CLOCAL | CREAD);

	tcsetattr(fd, TCSAFLUSH, &options);
}
// --------------------------------------------------------------------------------
void ComPort::reopen()
{
	if( fd > 0 )
	{
		struct termios options;
		tcgetattr(fd, &options);
		tcsetattr(fd, TCSAFLUSH, &oldTermios);
		close(fd);
		openPort();

		if( fd > 0 )
			tcsetattr(fd, TCSAFLUSH, &options);
	}
}
// --------------------------------------------------------------------------------
void ComPort::setSpeed( Speed s )
{
	struct termios options;
	speed = s;

	tcgetattr(fd, &options);

	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);

	tcsetattr(fd, TCSADRAIN, &options);
}
// --------------------------------------------------------------------------------
void ComPort::setParity(Parity parity)
{
	struct termios options;

	tcgetattr(fd, &options);

	switch(parity)
	{
		case Odd:
			options.c_cflag |= PARENB;
			options.c_cflag &= ~CMSPAR;
			options.c_cflag |= PARODD;
			break;

		case Even:
			options.c_cflag |= PARENB;
			options.c_cflag &= ~CMSPAR;
			options.c_cflag &= ~PARODD;
			break;

		case NoParity:
			options.c_cflag &= ~PARENB;
			break;

		case Space:
			options.c_cflag |= PARENB;
			options.c_cflag |= CMSPAR;
			options.c_cflag |= PARODD;
			break;

		case Mark:
			options.c_cflag |= PARENB;
			options.c_cflag |= CMSPAR;
			options.c_cflag &= ~PARODD;
			break;

		default:
			break;
	}

	tcsetattr(fd, TCSADRAIN, &options);
}
// --------------------------------------------------------------------------------
void ComPort::setCharacterSize(CharacterSize csize)
{
	struct termios options;

	tcgetattr(fd, &options);

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= csize;

	tcsetattr(fd, TCSADRAIN, &options);
}
// --------------------------------------------------------------------------------
void ComPort::setStopBits(StopBits sBit)
{
	struct termios options;

	tcgetattr(fd, &options);

	switch(sBit)
	{
		case OneBit:
			options.c_cflag &= ~CSTOPB;
			break;

		case OneAndHalfBits:
		case TwoBits:
			options.c_cflag |= CSTOPB;
			break;

		default:
			break;
	}

	tcsetattr(fd, TCSADRAIN, &options);
}

// --------------------------------------------------------------------------------
// Lav: Убрать
unsigned char ComPort::receiveByte()
{
	return m_receiveByte(waiting);
}
// --------------------------------------------------------------------------------
// Lav: Не использовать:
void ComPort::setWaiting(bool waiting)
{
	this->waiting = waiting;
}

// --------------------------------------------------------------------------------
// Lav: убрать receiveByte и не использовать буфер
// Lav: сделать корректное определение таймаута
unsigned char ComPort::m_receiveByte( bool wait )
{
	if( curSym >= bufLength )
	{
		curSym = 0;

		if(wait)
		{
			fd_set set;
			timeval timeout;

			FD_ZERO (&set);
			FD_SET (fd, &set);

			/* Initialize the timeout data structure. */
			timeout.tv_sec = 0;
			timeout.tv_usec = uTimeout;

			/* select' returns 0 if timeout, 1 if input available, -1 if error. */

			if(select(FD_SETSIZE, &set, NULL, NULL, &timeout) == 1)
				bufLength = ::read(fd, buf, BufSize);
			else
				bufLength = -1;
		}
		else
			bufLength = ::read(fd, buf, BufSize);

		if( bufLength <= 0 )
		{
			throw uniset::TimeOut();
		}
	}

	return buf[curSym++];
}
// --------------------------------------------------------------------------------
// Перенести в receiveBlock (определяется через timeout == 0)
void ComPort::setBlocking(bool blocking)
{
	if( blocking )
	{
		if( fcntl(fd, F_SETFL, 0) == -1 )
		{
			string str = "setBlocking: err: ";
			str += strerror(errno);
			throw uniset::SystemError(str.c_str());
		}
	}
	else
	{
		if( fcntl(fd, F_SETFL, O_NONBLOCK) == -1 )
		{
			string str = "setBlocking: err: ";
			str += strerror(errno);
			throw uniset::SystemError(str.c_str());
		}
	}
}
// --------------------------------------------------------------------------------
// Lav: Не использовать, удалить
void ComPort::sendByte(unsigned char x)
{
	//    fcntl(fd,F_SETFL,0);
	if( ::write(fd, &x, 1) < 0 )
	{
		string str = "Write Error: ";
		str += strerror(errno);
		throw uniset::SystemError(str.c_str());
	}

	//    fcntl(fd,F_SETFL,O_NONBLOCK);
}
// --------------------------------------------------------------------------------
// Lav: убрать, переделать в receiveBlock
void ComPort::setTimeout( timeout_t msec )
{
	uTimeout = msec * 1000;
}
// --------------------------------------------------------------------------------
timeout_t ComPort::getTimeout() const
{
	return uTimeout / 1000;    // msec
}
// --------------------------------------------------------------------------------
// Lav: ситуация, когда отправлено меньше запрошенного, не типична и должна
// генерировать исключение
ssize_t ComPort::sendBlock( unsigned char* msg, size_t len )
{
	//    fcntl(fd,F_SETFL,0);

	ssize_t sndLen =::write(fd, msg, len);

	//    fcntl(fd,F_SETFL,O_NONBLOCK);

	if(sndLen < 0)
	{
		string str = "Write Error: ";
		str += strerror(errno);
		throw uniset::SystemError(str.c_str());
	}

	return sndLen;
}
// --------------------------------------------------------------------------------
// Lav: ожидание задавать третим необязательным параметром
// Lav: Никогда не возвращаТЬ меньше запрошенного (кроме 0)
size_t ComPort::receiveBlock(unsigned char* msg, size_t len)
{
	size_t k = 0;

	if(!len)
		return 0;

	for(k = 0; k < len; k++)
	{
		try
		{
			msg[k] = m_receiveByte(waiting);
		}
		catch( const uniset::TimeOut& ex )
		{
			break;
		}
	}

	if(!k)
	{
		throw uniset::TimeOut();
	}

	return k;
}

// --------------------------------------------------------------------------------
void ComPort::cleanupChannel()
{
	if( fd < 0 )
		return;

	/*
	    unsigned char tmpbuf[100];
	    int k = 0;
	    do
	    {
	        k = ::read(fd,tmpbuf,sizeof(tmpbuf));
	    }
	    while( k>0 );
	*/
	unsigned char tmpbuf[100];
	ssize_t k = 0;
	bool old_waiting = waiting;
	waiting = false;
	int mask = fcntl(fd, F_GETFL);
	(void)fcntl(fd, F_SETFL, O_NONBLOCK);

	try
	{
		do
		{
			k = receiveBlock(tmpbuf, sizeof(tmpbuf));
		}
		while( k > 0 );
	}
	catch(...) {}

	(void)fcntl(fd, F_SETFL, mask);
	waiting = old_waiting;
}
// --------------------------------------------------------------------------------
void ComPort::setSpeed( const std::string& s )
{
	Speed sp = getSpeed(s);

	if( sp != ComPort::ComSpeed0 )
		setSpeed(sp);
}
// --------------------------------------------------------------------------------
ComPort::Speed ComPort::getSpeed() const
{
	return speed;
}
// --------------------------------------------------------------------------------
std::string ComPort::getSpeed( Speed s )
{
	if( s == ComSpeed9600 )
		return "9600";

	if( s == ComSpeed19200 )
		return "19200";

	if( s == ComSpeed38400 )
		return "38400";

	if( s == ComSpeed57600 )
		return "57600";

	if( s == ComSpeed115200 )
		return "115200";

	if( s == ComSpeed4800 )
		return "4800";

	if( s == ComSpeed0 )
		return "";

	if( s == ComSpeed50 )
		return "50";

	if( s == ComSpeed75 )
		return "75";

	if( s == ComSpeed110 )
		return "110";

	if( s == ComSpeed134 )
		return "134";

	if( s == ComSpeed150 )
		return "150";

	if( s == ComSpeed200 )
		return "200";

	if( s == ComSpeed300 )
		return "300";

	if( s == ComSpeed600 )
		return "600";

	if( s == ComSpeed1200 )
		return "1200";

	if( s == ComSpeed1800 )
		return "1800";

	if( s == ComSpeed2400 )
		return "2400";

	if( s == ComSpeed230400 )
		return "230400";

	if( s == ComSpeed460800 )
		return "460800";

	if( s == ComSpeed500000 )
		return "500000";

	if( s == ComSpeed576000 )
		return "576000";

	if( s == ComSpeed921600 )
		return "921600";

	if( s == ComSpeed1000000 )
		return "1000000";

	if( s == ComSpeed1152000 )
		return "1152000";

	if( s == ComSpeed1500000 )
		return "1500000";

	if( s == ComSpeed2000000 )
		return "2000000";

	if( s == ComSpeed2500000 )
		return "2500000";

	if( s == ComSpeed3000000 )
		return "3000000";

	if( s == ComSpeed3500000 )
		return "3500000";

	if( s == ComSpeed4000000 )
		return "4000000";

	return "";
}
// --------------------------------------------------------------------------------
#define CHECK_SPEED(var,speed) \
	if( var == __STRING(speed) ) \
		return ComPort::ComSpeed##speed;

ComPort::Speed ComPort::getSpeed( const string& s )
{
	// см. ComPort.h

	// сперва самые вероятные
	CHECK_SPEED(s, 9600)
	CHECK_SPEED(s, 19200)
	CHECK_SPEED(s, 38400)
	CHECK_SPEED(s, 57600)
	CHECK_SPEED(s, 115200)

	// далее просто поддерживаемые
	CHECK_SPEED(s, 50)
	CHECK_SPEED(s, 75)
	CHECK_SPEED(s, 110)
	CHECK_SPEED(s, 134)
	CHECK_SPEED(s, 150)
	CHECK_SPEED(s, 200)
	CHECK_SPEED(s, 300)
	CHECK_SPEED(s, 600)
	CHECK_SPEED(s, 1200)
	CHECK_SPEED(s, 1800)
	CHECK_SPEED(s, 2400)
	CHECK_SPEED(s, 4800)

	CHECK_SPEED(s, 230400)
	CHECK_SPEED(s, 460800)
	CHECK_SPEED(s, 500000)
	CHECK_SPEED(s, 576000)
	CHECK_SPEED(s, 921600)
	CHECK_SPEED(s, 1000000)
	CHECK_SPEED(s, 1152000)
	CHECK_SPEED(s, 1500000)
	CHECK_SPEED(s, 2000000)
	CHECK_SPEED(s, 2500000)
	CHECK_SPEED(s, 3000000)
	CHECK_SPEED(s, 3500000)
	CHECK_SPEED(s, 4000000)

	return ComPort::ComSpeed0;
}
// --------------------------------------------------------------------------------
#undef CHECK_SPEED
// --------------------------------------------------------------------------------
