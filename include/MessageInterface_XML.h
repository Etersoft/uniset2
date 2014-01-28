/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
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
 *  \brief Реализация интерфейса работы с сообщениями на основе XML
 *  \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef __MESSAGEINTERFACE_XML_H__
#define __MESSAGEINTERFACE_XML_H__
// --------------------------------------------------------------------------
#include <vector>
#include "UniXML.h"
#include "MessageInterface.h"
// --------------------------------------------------------------------------
class MessageInterface_XML:
	public MessageInterface
{
	public:
		MessageInterface_XML(const std::string confile="", int msize=200);
		MessageInterface_XML(UniXML& xml, int msize=200);
		virtual ~MessageInterface_XML();
		virtual	std::string getMessage( UniSetTypes::MessageCode code );
		virtual	bool isExist(UniSetTypes::MessageCode code);

		virtual	UniSetTypes::MessageCode getCode( const std::string& msg );
		virtual	UniSetTypes::MessageCode getCodeByIdName( const std::string& name );
		
		virtual std::ostream& printMessagesMap(std::ostream& os);
		friend std::ostream& operator<<(std::ostream& os, MessageInterface_XML& mi );
		
	protected:
		void read( UniXML& xml );
		std::vector<UniSetTypes::MessageInfo> msgmap;
};
// --------------------------------------------------------------------------
#endif
