#ifndef DISABLE_COMPORT_485F
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
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/io.h>
#include <unistd.h>
#include <dirent.h>                 /* we need MAXNAMLEN */

#include "Exceptions.h"
#include "ComPort485F.h"
// --------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------------
/* This is for RTS control (through GPIO) */
#define GPIO_BA 0xF000
static void gpio_low_out_en( char gpio_num )
{
	unsigned long val;

	val = inl(GPIO_BA + 4);
	val |= (((unsigned long)1) << gpio_num);
	val &= ~(((unsigned long)1) << (gpio_num + 16));
	outl(val, GPIO_BA + 4);
}
// --------------------------------------------------------------------------------
static void gpio_low_set_value( char gpio_num, char value )
{
	unsigned int val;

	val = inl(GPIO_BA);

	if (value)
	{
		val |= (((unsigned int)1) << gpio_num);
		val &= ~(((unsigned int)1) << (gpio_num + 16));
	}
	else
	{
		val &= ~(((unsigned int)1) << gpio_num);
		val |= (((unsigned int)1) << (gpio_num + 16));
	}

	outl(val, GPIO_BA);
}
// --------------------------------------------------------------------------------
/* Set/Clear RTS for port */
static void setRTS(int fd, int state)
{
	int status;

	ioctl(fd, TIOCMGET, &status);

	if (state)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	ioctl(fd, TIOCMSET, &status);
}
// --------------------------------------------------------------------------------
ComPort485F::ComPort485F( const string& dev, char gpio_num, bool tmit_ctrl ):
	ComPort(dev, false),
	gpio_num(gpio_num),
	tmit_ctrl_on(tmit_ctrl)
{
	if( tmit_ctrl_on )
	{
		iopl(3);
		gpio_low_out_en(gpio_num);
		setRTS(fd, 0);
		gpio_low_set_value(gpio_num, 0);
	}
}
// --------------------------------------------------------------------------------
void ComPort485F::setTimeout( timeout_t msec )
{
	tout_msec = msec;
	ComPort::setTimeout(msec);
}
// --------------------------------------------------------------------------------
unsigned char ComPort485F::m_receiveByte( bool wait )
{
	if( tmit_ctrl_on )
	{
		setRTS(fd, 0);
		gpio_low_set_value(gpio_num, 0);
	}

	if( rq.empty() )
	{
		ssize_t rc = 0;

		if( wait )
		{
			if( tout_msec > 20 )
				ptRecv.setTiming(tout_msec);
			else
				ptRecv.setTiming(20);

			while( !ptRecv.checkTime() )
			{
				ioctl(fd, FIONREAD, &rc);

				if( rc > 0 )
				{
					rc = ::read(fd, tbuf, sizeof(tbuf));

					if( rc > 0 )
					{
						if( remove_echo(tbuf, rc) )
							break;
					}
				}

				std::this_thread::sleep_for(std::chrono::microseconds(3));
			}

			if ( ptRecv.checkTime() )
				rc = -1;
		}
		else
		{
			for( unsigned int i = 0; i < 3; i++ )
			{
				rc = ::read(fd, tbuf, sizeof(tbuf));

				if( rc > 0 )
				{
					if( remove_echo(tbuf, rc) )
						break;
				}

				std::this_thread::sleep_for(std::chrono::microseconds(3));
			}
		}

		if( rc <= 0 )
			throw uniset::TimeOut();
	}


	unsigned char x = rq.front();
	rq.pop();
	return x;
}
// --------------------------------------------------------------------------------
ssize_t ComPort485F::sendBlock(unsigned char* msg, size_t len )
{
	if( tmit_ctrl_on )
	{
		setRTS(fd, 1);
		gpio_low_set_value(gpio_num, 1);
	}

	ssize_t r = 0;

	try
	{
		cleanupChannel();
		r = ComPort::sendBlock(msg, len);

		if( tmit_ctrl_on )
		{
			tcdrain(fd);
			gpio_low_set_value(gpio_num, 0);
			setRTS(fd, 0);
		}

		if( r > 0 )
		{
			save2queue(msg, len, r);
			m_read(2000);
		}
	}
	catch( const uniset::Exception& )
	{
		if( tmit_ctrl_on )
		{
			setRTS(fd, 0);
			gpio_low_set_value(gpio_num, 0);
		}

		throw;
	}

	if( tmit_ctrl_on )
	{
		setRTS(fd, 0);
		gpio_low_set_value(gpio_num, 0);
	}

	return r;
}
// --------------------------------------------------------------------------------
void ComPort485F::sendByte( unsigned char x )
{
	/* Fire transmitter */
	if( tmit_ctrl_on )
	{
		setRTS(fd, 1);
		gpio_low_set_value(gpio_num, 1);
	}

	try
	{
		ComPort::sendByte(x);

		if( tmit_ctrl_on )
		{
			tcdrain(fd);
			setRTS(fd, 0);
			gpio_low_set_value(gpio_num, 0);
		}

		wq.push(x);
		m_read(2000);
	}
	catch( const uniset::Exception& )
	{
		if( tmit_ctrl_on )
		{
			tcdrain(fd);
			setRTS(fd, 0);
			gpio_low_set_value(gpio_num, 0);
		}

		throw;
	}

	if( tmit_ctrl_on )
	{
		setRTS(fd, 0);
		gpio_low_set_value(gpio_num, 0);
	}
}
// --------------------------------------------------------------------------------
void ComPort485F::save2queue( unsigned char* msg, size_t len, size_t bnum )
{
	for( size_t i = 0; i < len && i < bnum; i++ )
		wq.push(msg[i]);
}
// --------------------------------------------------------------------------------
bool ComPort485F::remove_echo( unsigned char tb[], ssize_t len )
{
	ssize_t i = 0;

	while( !wq.empty() )
	{
		unsigned char x = wq.front();

		if( x != tb[i] )
			break;

		wq.pop();

		if( (++i) >= len )
			break;
	}

	if( wq.empty() && i < len )
	{
		// значит считали и полезную информацию
		// её перемещаем в очередь чтения
		for( ; i < len; i++ )
			rq.push(tb[i]);
	}

	// возвращаем число оставшихся символов
	return wq.empty();
}
// --------------------------------------------------------------------------------
void ComPort485F::m_read( timeout_t tmsec )
{
	ptRecv.setTiming(tmsec);
	ssize_t rc = 0;

	while( !ptRecv.checkTime() )
	{
		ioctl(fd, FIONREAD, &rc);

		if( rc > 0 )
		{
			rc = ::read(fd, tbuf, sizeof(tbuf));

			if( rc > 0 )
			{
				if( remove_echo(tbuf, rc) )
					break;
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(3));
	}
}
// --------------------------------------------------------------------------------
void ComPort485F::cleanupChannel()
{
	while( !wq.empty() )
		wq.pop();

	while( !rq.empty() )
		rq.pop();

	ComPort::cleanupChannel();
}
// --------------------------------------------------------------------------------
void ComPort485F::reopen()
{
	while( !wq.empty() )
		wq.pop();

	while( !rq.empty() )
		rq.pop();

	ComPort::reopen();
}
// --------------------------------------------------------------------------------
#endif // #ifndef DISABLE_COMPORT_485F
