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
 *  \brief Заголовочный файл для организации низкоуровневого ввода-вывода.
 *  \author Vitaly Lipatov
 *  \date   $Date: 2005/11/01 21:44:53 $
 *  \version $Id: DigitalCard.h,v 1.4 2005/11/01 21:44:53 vpashka Exp $
 */
// --------------------------------------------------------------------------
#ifndef DIGITALCARD_H__
#define DIGITALCARD_H__

/*! Абстрактный класс работы с цифровым вводом/выводом */
class DigitalCard
{
	public:
		virtual ~DigitalCard(){};

//		virtual bool getCurrentState(int module)=0;
		virtual bool get(int module) = 0;
		virtual void set(int module, bool state) = 0;
};

#endif
