/***************************************************************************
* This file is part of the UniSet* library                                 *
* Copyright (C) 2002 SET Research Institute. All rights reserved.          *
***************************************************************************/
// --------------------------------------------------------------------------------
// $Id: ComPort485F.cc,v 1.1 2009/02/24 20:27:25 vpashka Exp $
// --------------------------------------------------------------------------------
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/io.h>
#include <unistd.h>
#include <dirent.h>			     /* we need MAXNAMLEN */

#include "Exceptions.h"
#include "ComPort485F.h"
// --------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------------
/* This is for RTS control (through GPIO) */
#define GPIO_BA 0xF000
static void gpio_low_out_en(char gpio_num)
{
	unsigned long val;

	val = inl(GPIO_BA + 4);
	val |= (((unsigned long)1) << gpio_num);
	val &= ~(((unsigned long)1) << (gpio_num + 16));
	outl(val, GPIO_BA + 4);
}
// --------------------------------------------------------------------------------
static void gpio_low_set_value(char gpio_num, char value)
{
	unsigned long val;

	val = inl(GPIO_BA);
	if (value) {
		val |= (((unsigned long)1) << gpio_num);
		val &= ~(((unsigned long)1) << (gpio_num + 16));
	} else {
		val &= ~(((unsigned long)1) << gpio_num);
		val |= (((unsigned long)1) << (gpio_num + 16));
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
ComPort485F::ComPort485F( string dev, int gpio_num, bool tmit_ctrl ):
	ComPort(dev,false),
	gpio_num(gpio_num)
{
	if( tmit_ctrl_on = tmit_ctrl )
	{
		iopl(3);
		gpio_low_out_en(gpio_num);
		setRTS(fd, 0);
		gpio_low_set_value(gpio_num, 0);
	}
}
// --------------------------------------------------------------------------------
void ComPort485F::setTimeout(int timeout)
{
	tout_msec = timeout / 1000;
	ComPort::setTimeout(timeout);
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
		int rc = 0;
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
					rc = ::read(fd,tbuf,sizeof(tbuf));
					if( rc > 0 )
					{
						if( remove_echo(tbuf,rc) ) 
							break;
					}
				}
				usleep(3000);
			}
			
			if ( ptRecv.checkTime() )
				rc = -1;
		}
		else
		{
			for( int i=0; i<3; i++ )
			{
				rc = ::read(fd,tbuf,sizeof(tbuf));
				if( rc > 0 )
				{
					if( remove_echo(tbuf,rc) )
						break;
				}
				usleep(3000);
			}
		}

		if( rc <= 0 )
			throw UniSetTypes::TimeOut();
	}

		
	unsigned char x = rq.front();
	rq.pop();
	return x;
}
// --------------------------------------------------------------------------------
int ComPort485F::sendBlock( unsigned char* msg, int len )
{
	if( tmit_ctrl_on )
	{
		setRTS(fd, 1);
		gpio_low_set_value(gpio_num, 1);
	}
	int r=0;
	try
	{
		cleanupChannel();
		r = ComPort::sendBlock(msg,len);
		if( tmit_ctrl_on )
		{
			tcdrain(fd);
			gpio_low_set_value(gpio_num, 0);
			setRTS(fd, 0);
		}
		if( r > 0 )
		{
			save2queue(msg,len,r);
			m_read(2000);
		}
	}
	catch( Exception& ex )
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
	catch( Exception& ex )
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
void ComPort485F::save2queue( unsigned char*msg, int len, int bnum )
{
	for( int i=0; i<len && i<bnum; i++ )
	{
		wq.push(msg[i]);
//		fprintf(stderr,"save 2 squeue: %x\n",msg[i]);
	}
}
// --------------------------------------------------------------------------------
bool ComPort485F::remove_echo( unsigned char tb[], int len )
{
	int i=0;
	while( !wq.empty() )
	{
		unsigned char x = wq.front();
		if( x != tb[i] )
			break;

		wq.pop();

		if( (++i) >= len )
			break;
	}

	if( wq.empty() && i<len )
	{
		// значит считали и полезную информацию
		// её перемещаем в очередь чтения
		for( ;i<len; i++ )
			rq.push(tb[i]);
	}

	// возвращаем число оставшихся символов	
	return wq.empty();
}
// --------------------------------------------------------------------------------
void ComPort485F::m_read( int tmsec )
{
	ptRecv.setTiming(tmsec);
	int rc = 0;
	while( !ptRecv.checkTime() )
	{
		ioctl(fd, FIONREAD, &rc);
		if( rc > 0 )
		{			
			rc = ::read(fd,tbuf,sizeof(tbuf));
			if( rc > 0 )
			{
				if( remove_echo(tbuf,rc) )
					break;
			}
		}
		usleep(3000);
	}
}
// --------------------------------------------------------------------------------
