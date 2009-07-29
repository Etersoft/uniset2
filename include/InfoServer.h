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
 * \author Pavel Vainerman <pv>
 * \date  $Date: 2007/01/06 00:03:34 $
 * \version $Id: InfoServer.h,v 1.11 2007/01/06 00:03:34 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#ifndef InfoServer_H_
#define InfoServer_H_
//---------------------------------------------------------------------------
#include <map>
#include <list>
#include "MessageType.h"
#include "UniSetObject.h"
#include "InfoServer_i.hh"
//---------------------------------------------------------------------------
class ISRestorer;
//---------------------------------------------------------------------------
/*!
	\page ServicesPage
	\section secInfoServer Сервис сообщений
	 \subsection subIS_common Общие сведения
		В его задачи входит обработка всех сообщений для оператора.
	При приходе сообщения он производит следующие действия:
	- переправляет сообщение в БД (\ref secDBServer)
	- рассылает сообщение на все узлы, если оно помечено как broadcast.
	- рассылает сообщение всем занесённым в RouteList(см. конф.файл).
	- обрабатывает сообщение специфичным для каждого проекта образом (т.е. вызывает виртуальную 
	функцию InfoServer::processing() )

	\subsection subIS_idea Сценарий работы
	 	На узле, где ведётся БД запускается один экземпляр сервиса. Клиенты могут получить доступ, несколькими способами:
		- через NameService
		- при помощи UniversalInterface::send()
		
	\par 		
	Сервис является системным, поэтому его идентификатор можно получить при помощи 
	UniSetTypes::Configuration::getInfoServer() объекта UniSetTypes::conf.

	\subsection subIS_interface Интерфейс	
	InfoServer позволяет заказывать уведомелние о приходе тех или иных сообщений, 
	а также подтверждения(квитирования). Можно производить заказ сообщения по коду
	\code
		InfoServer::ackMessage(UniSetTypes::MessageCode msgid, const UniSetTypes::ConsumerInfo& ci, 
									UniversalIO::UIOCommand cmd, CORBA::Boolean acknotify);
	\endcode
	или сразу из диапазона кодов
	\code
		InfoServer::ackMessageRange(UniSetTypes::MessageCode from, UniSetTypes::MessageCode to,
									const UniSetTypes::ConsumerInfo& ci, 
									UniversalIO::UIOCommand cmd, CORBA::Boolean acknotify);
	\endcode


	Реализацию см. \ref InfoServer				
*/

/*!
 * \class InfoServer
 * \brief Интерфейс для вывода информации
*/ 
class InfoServer: 
	public UniSetObject,
	public POA_InfoServer_i
{
	public:
		InfoServer( UniSetTypes::ObjectId id=UniSetTypes::DefaultObjectId, ISRestorer* d=0 );
		virtual ~InfoServer();

		virtual UniSetTypes::ObjectType getType(){ return UniSetTypes::getObjectType("InfoServer"); }

		
		// реализация IDL интерфейса
		/*! заказ уведомления о приходе сообщения */
		virtual void ackMessage(UniSetTypes::MessageCode msgid, const UniSetTypes::ConsumerInfo& ci, 
							UniversalIO::UIOCommand cmd, CORBA::Boolean acknotify);
		/*! заказ уведомления о приходе сообщения из диапазона */
		virtual void ackMessageRange(UniSetTypes::MessageCode from, UniSetTypes::MessageCode to,
									const UniSetTypes::ConsumerInfo& ci, 
									UniversalIO::UIOCommand cmd, CORBA::Boolean acknotify);


		/*! информация о потребителе (заказчике) */
		struct ConsumerInfoExt:
			public	UniSetTypes::ConsumerInfo
		{
			ConsumerInfoExt( UniSetTypes::ConsumerInfo ci,
							UniSetObject_i_ptr ref=0):
			ConsumerInfo(ci),ref(ref){}

			UniSetObject_i_ptr ref;
			CORBA::Boolean ask;
		};

		/*! Список потребителей */
		typedef std::list<ConsumerInfoExt> ConsumerList; 				

		/*! массив пар идентификатор->список потребителей */
		typedef std::map<UniSetTypes::MessageCode,ConsumerList> AskMap;		

	
	protected:
		// Функции обработки пришедших сообщений 
		virtual void processingMessage( UniSetTypes::VoidMessage *msg );

		/*! Функция обработки UniSetTypes::AlarmMessage.
		 Переопределяется в кокретном проекте, если требуется специфичная обработка. */
		virtual void processing(UniSetTypes::AlarmMessage &amsg){};
		/*! Функция обработки UniSetTypes::InfoMessage.
		 Переопределяется в кокретном проекте, если требуется специфичная обработка. */
		virtual void processing(UniSetTypes::InfoMessage &imsg){};
		/*! Функция обработки UniSetTypes::AckMessage.
		 Переопределяется в кокретном проекте, если требуется специфичная обработка. */
		virtual void processing(UniSetTypes::ConfirmMessage &cmsg){};		

		
		/*! Предварительная обработка сообщения. Пересылка на другие узлы и
			сохранение в базе.
		*/
		void preprocessing( UniSetTypes::TransportMessage& tmsg, bool broadcast );
		void preprocessingConfirm( UniSetTypes::ConfirmMessage& am, bool broadcast );

		virtual bool activateObject();


		/*! сохранение списка заказчиков 
			По умолчанию делает dump, если объявлен dumper.
		*/
		virtual void dumpOrdersList(UniSetTypes::MessageCode mid, const ConsumerList& lst);

		/*! чтение dump-файла */
		virtual void readDump();

		//! посылка информации об приходе сообщения
		template<class TMessage>
		void event(UniSetTypes::MessageCode key, TMessage& msg, CORBA::Boolean askn);

		//----------------------
		//! посылка информации об изменении состояния датчика
		template <class TMessage>
		void send(ConsumerList& lst, TMessage& msg, CORBA::Boolean acknotify);
		bool addConsumer(ConsumerList& lst, const UniSetTypes::ConsumerInfo& cons, CORBA::Boolean acknotify); 	//!< добавить потребителя сообщения
		bool removeConsumer(ConsumerList& lst, const UniSetTypes::ConsumerInfo& cons, CORBA::Boolean acknotify);	//!< удалить потребителя сообщения

		
		//! обработка заказа 
		void ask(AskMap& askLst, UniSetTypes::MessageCode key, 
					const UniSetTypes::ConsumerInfo& cons, UniversalIO::UIOCommand cmd,
					CORBA::Boolean acknotify);

		/*! указатель на объект реализующий дамп списка заказчиков */
		ISRestorer* restorer;		

	private:
		friend class ISRestorer;
		bool dbrepeat; /*!< флаг пересылки сообщений DBServer-у */


		AskMap askList;	/*!< список потребителей */
		/*! замок для блокирования совместного доступа к cписку потребителей */			
		UniSetTypes::uniset_mutex askMutex; 
		
		std::list<UniSetTypes::ConsumerInfo> routeList;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
