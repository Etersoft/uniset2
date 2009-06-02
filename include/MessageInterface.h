/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 *  \brief Класс работы с сообщениями
 *  \author Pavel Vainerman <pv>
 *  \date   $Date: 2006/12/22 13:37:48 $
 *  \version $Id: MessageInterface.h,v 1.7 2006/12/22 13:37:48 vpashka Exp $
 *
 *	Получение сообщения по id
 */
// --------------------------------------------------------------------------
#ifndef __MESSAGEINTERFACE_H__
#define __MESSAGEINTERFACE_H__
// --------------------------------------------------------------------------
#include <string>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
class MessageInterface
{
	public:
		virtual ~MessageInterface(){}
		virtual	std::string getMessage( UniSetTypes::MessageCode code )=0;
		virtual	bool isExist(UniSetTypes::MessageCode code)=0;

		virtual	UniSetTypes::MessageCode getCode( const std::string& msg ){ return UniSetTypes::DefaultMessageCode; }
		virtual	UniSetTypes::MessageCode getCodeByIdName( const std::string& name ){ return UniSetTypes::DefaultMessageCode; }

		virtual std::ostream& printMessagesMap(std::ostream& os)=0;
};
// --------------------------------------------------------------------------
#endif
