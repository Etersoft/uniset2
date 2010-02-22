// This file is part of the NCS project. (c) 1999-2000 All rights reserved.
// $Id: AnalogCard_AI165A.cc,v 1.5 2005/01/28 21:04:08 vitlav Exp $

// 14-ти битная аналоговая карта 16 входов (8 дифф), 2 выхода (12-ти битные)
/*
#include <assert.h>

#include "conf.h"
#include "IOPort.h"
#include "IOs/AnalogCardAI165A.h"
#include "SysTimer.h"
#include "Exceptions.h"

//-----------------------------------------------------------------------------

AnalogCardAI165A::AnalogCardAI165A( void )
{
	if( ioperm( base_io_adr, 0x10, PORT_PERMISSION ) == -1 )
		throw GlobalError("Невозможно открыть порты");
	if (!((inb(base_io_adr+14)=='A')&&(inb(base_io_adr+15)==17)))
		throw GlobalError("Модуль AI16-5a-STB с FIFO не найден !");

	outw(0x0000,base_io_adr+0); 
	// Умножение входного сигнала
	int u=0x0000;
//    u=0x5555;
//      case 2 :  n=0xAAAA; break;
//      case 3 :  n=0xFFFF; break;
	outw(u,base_io_adr+6); 
	outw(u,base_io_adr+8); 
	maxInputChannel=16;
	
}

//-----------------------------------------------------------------------------

AnalogCardAI165A::~AnalogCardAI165A( void )
{
}

//-----------------------------------------------------------------------------

int AnalogCardAI165A::getValue( int io_channel )
{
	assert(io_channel>=0);
	assert(io_channel<16);
//	if(io_channel>=16)
//		printf("%d\n",io_channel);
//	io_channel&=0x0F;
	outb(0x00, base_io_adr+0);
	io_channel|=0x20; // Однопроводное подключение
	outb(io_channel, base_io_adr+2);
	msleep(10); // можно поставить for (int i=0;i<90;i++);
	//int o=inb(base_io_adr+0);
	//outb((o&240)|0x80, base_io_adr+0);
	outb(0x80, base_io_adr+0); // ST_RDY  Старт АЦП
	//for (int i=0; i<60; i++); // pause
	//while(!(inb(base_io_adr+0)&0x80)); // ST_RDY
	while(!(inb(base_io_adr+0)&0x20));
	int r=inw(base_io_adr+2);
	if (r<0) r=-1;
	//if (r>8191) r=8191;
	return r;
}

void AnalogCardAI165A::getChainValue( long * data, const int NumChannel )
{
	assert(NumChannel>=0);
	assert(NumChannel<16);
	outw(0x00, base_io_adr+0);
	int io_channel=0;
	io_channel|=0x20; // Однопроводное подключение
	outb(io_channel, base_io_adr+2);
	msleep(1);
	for(int n=0; n< NumChannel; n++)
	{
		outb(0x80|0x01, base_io_adr+0); // ST_RDY | n++
		while(!(inb(base_io_adr+0)&0x80)); // ST_RDY // Опрос бита готовности
		data[n]=inw(base_io_adr+2);
		// может быть здесь все же нужна пауза?
	}
	outw(0x00, base_io_adr+0);
}

//-----------------------------------------------------------------------------
// !!! Не устанавливается номер канала
void AnalogCardAI165A::setValue( int io_channel, int value )
	// io_channel -- 0 or 1
{
	assert(io_channel==0 || io_channel==1 );
	if (value<0) 
		value=0;
	else if (value>4095)
		value=4095;
	io_channel&=1;
	io_channel<<=12;
	value&=0x0FFF;
	outw(value,base_io_adr+14);
	outb(value,base_io_adr+14);
}

*/
