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
 *  \author Pavel Vainerman <pv>
 *  \date   $Date: 2007/06/17 21:30:55 $
 *  \version $Id: DBServer.h,v 1.8 2007/06/17 21:30:55 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#ifndef DBServer_H_
#define DBServer_H_
// --------------------------------------------------------------------------
#include "UniSetTypes.h"
#include "UniSetObject_LT.h"
//------------------------------------------------------------------------------------------
/*!
	 \page ServicesPage
	 \section secDBServer Сервис ведения БД
	 \subsection subDBS_common Общие сведения
	 	Предназначен для работы с БД. 
		Основная задача это - сохрание информации о датчиках, ведение журнала сообщений.
		\note
		Обрабатывает сообщения типа  UniSetTypes::DBMessage.

	 \subsection subDBS_idea Сценарий работы
	 	На узле, где ведётся БД запускается один экземпляр сервиса. Клиенты могут получить доступ, несколькими способами:
		- через NameService
		- при помощи UniversalInterface::send()

		Сервис является системным, поэтому его идентификатор можно получить при помощи 
	UniSetTypes::Configuration::getDBServer() объекта UniSetTypes::conf.

	Реализацию см. \ref DBServer_MySQL
*/

/*! Прототип реализации сервиса ведения БД */
class DBServer: 
	public UniSetObject_LT
{
	public:
		DBServer( UniSetTypes::ObjectId id );
		DBServer();
		~DBServer();

	protected:

		virtual void processingMessage( UniSetTypes::VoidMessage* msg );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm ){};

		// Функции обработки пришедших сообщений
		virtual void parse( UniSetTypes::SensorMessage* sm ){};
		virtual void parse( UniSetTypes::DBMessage* dbmsg ){};
		virtual void parse( UniSetTypes::InfoMessage* imsg ){};
		virtual void parse( UniSetTypes::AlarmMessage* amsg ){};
		virtual void parse( UniSetTypes::ConfirmMessage* cmsg ){};		

		virtual bool activateObject();
		virtual void init(){};
		
	private:
};
//------------------------------------------------------------------------------------------
#endif
