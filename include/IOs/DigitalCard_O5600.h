/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Vitaly Lipatov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
/*! \file
 *  \brief Заголовочный файл для дискретной карты DIO5600
 *  \author Vitaly Lipatov
 */
// -------------------------------------------------------------------------- 
#ifndef __DIO5600_H_
#define __DIO5600_H_
// --------------------------------------------------------------------------
#include "DigitalCard.h"
#include "IOs/IOAccessOld.h"

//  портов для функций ввода/вывода
const int DIO5600_A = 0x10;
const int DIO5600_B = 0x02;
const int DIO5600_C_LO = 0x01;
const int DIO5600_C_HI = 0x08;
const int DIO5600_C = (DIO5600_C_LO | DIO5600_C_HI);

class DigitalCard_O5600: public DigitalCard, IOAccess
{
	public:
		//inline	DIO5600() { }
		inline	DigitalCard_O5600(int ba, char mode)
	{
		init(1, 0, ba, mode);
	}

		bool	get(int module);
	//	bool	getCurrentState(int module);
		void	set(int module, bool state);

	protected:
	//	void	reinit();
		char	getMask(int module);
		char	getPrevious(int module);
		int		getPort(int module);
		void	putByte(int port, char val);
		int 	getByte(int ba);

		bool	init(bool flagInit, int numOfCard, int base_address,char mode);
		int		baseadr;	//
		char	iomode;		//
		char	portmap[4];


};
#endif
