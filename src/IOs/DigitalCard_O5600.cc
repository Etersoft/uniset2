/***************************************************************************
* This file is part of the UniSet* library                                 *
* Copyright (C) 1999-2002 SET Research Institute. All rights reserved.     *
***************************************************************************/
/*! \file
 *  \brief Класс для дискретной карты O5600
 *  \author Vitaly Lipatov
 *  \date   $Date: 2006/12/19 15:46:14 $
 *  
 *  
 */
/**************************************************************************/


#include <assert.h>

#include "IOs/DigitalCard_O5600.h"
#include "Exceptions.h"

using namespace std;

// Получить маску для модуля module
char DigitalCard_O5600::getMask( int module )
{
	assert ( module >= 0 && module < 24 );
	return 1 << ( module & 0x07 );
}

// Получить адрес порта для  указанного модуля
int DigitalCard_O5600::getPort( int module )
{
	//assert( baseadr + 2 < SIZE_OF_PORTMAP );
	if ( ( module >= 0 ) && ( module < 8 ) )
	{
		return baseadr + 2; // C
	}
	else if ( module > 7 && module < 16 )
	{
		return baseadr + 0; // A
	}
	else if ( module > 15 && module < 24 )
	{
		return baseadr + 1; // B
	}
	//	Чего за ерунду передаете?!!

	//logErr.Save( "Неправильный номер модуля для функции GetPort, module=%d", module );
	//throw Exceptions::OutOfRange();
	return baseadr;

}

bool DigitalCard_O5600::get( int module )
{
	char tmp = ~in( getPort( module ) );
	return tmp & getMask( module );
}


char DigitalCard_O5600::getPrevious( int module )
{
	assert ( getPort( module ) - baseadr >= 0 );
	assert ( getPort( module ) - baseadr < 4 );
	return portmap[ getPort( module ) - baseadr ];
}

/*
bool DigitalCard_O5600::getCurrentState( int module )
{
	return getPrevious( module ) & getMask( module );
}
*/

void DigitalCard_O5600::putByte( int ba, char val )
{
	portmap[ba - baseadr] = val;
	out( ba, ~val );
}

int DigitalCard_O5600::getByte( int ba )
{
	char tmp = ~in( ba );
	return tmp;
}


void DigitalCard_O5600::set( int module, bool state )
{
	char val = getPrevious( module );
	char mask = getMask( module );
	if ( state )
		val |= mask;
	else
		val &= ~mask;
	putByte( getPort( module ), val );
}

bool DigitalCard_O5600::init( bool flagInit, int numOfCard, int base_address, char mode )
{
	//if ( !Card.portmap )
	//	logIO.Save( "DigitalCard_O5600: Вызвана Init без предварительного получения памяти" );
	//this->numOfCard = numOfCard;
	baseadr = base_address;
	cout << "baseadr=" << baseadr << endl;
	iomode = mode;
	//logReg.Save( "DigitalCard_O5600: Начали" );
	out( baseadr + 0x03, iomode | 0x80  ); // Настройка режима карты
	for ( int k = 0;k < 3;k++ )
	{
		putByte( baseadr + k, 0 );
	}
	if ( in( baseadr + 0x03 ) == 0xff )  // Нет карты
		return false;
	//logReg.Save( "DigitalCard_O5600: Установлен" );
	return true;
}

