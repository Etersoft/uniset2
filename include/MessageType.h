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
 *  \brief Базовые типы сообщений
 *  \author Vitaly Lipatov, Pavel Vainerman
 */
// -------------------------------------------------------------------------- 
#ifndef MessageType_H_
#define MessageType_H_
// --------------------------------------------------------------------------
#include <sys/time.h>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "IOController_i.hh"

namespace UniSetTypes
{
	class Message
	{
		public:
			enum TypeOfMessage
			 {
				Unused,	// Сообщение не содержит информации
				SensorInfo,
				SysCommand, // Сообщение содержит системную команду
				Confirm,	// Сообщение содержит подтверждение
				Info,	// Сообщения содержит информацию для оператора
				Timer,	// Сообщения о срабатывании таймера
				Alarm,	// Аварийное сообщение
				DataBase,
				TheLastFieldOfTypeOfMessage // Обязательно оставьте последним
			};
	
			int type;	// Содержание сообщения (тип)

			enum Priority
			{
				Low,
				Medium,
				High,
				Super
			};

			Priority priority;
			ObjectId node;		// откуда
			ObjectId supplier;	// от кого
			ObjectId consumer;	// кому
			struct timeval tm;

			Message();

			template<class In> 
			static TransportMessage transport(const In& msg)
			{
				TransportMessage tmsg;
				assert(sizeof(UniSetTypes::RawDataOfTransportMessage)>=sizeof(msg));
				memcpy(&tmsg.data,&msg,sizeof(msg));
				return tmsg;
			}
	};
	

	class VoidMessage : public Message
	{
		public:
			VoidMessage( const TransportMessage& tm );
			VoidMessage();
	    	inline bool operator < ( const VoidMessage& msg ) const
			{
				if( priority != msg.priority )
					return priority < msg.priority;
	
				if( tm.tv_sec != msg.tm.tv_sec )
					return tm.tv_sec >= msg.tm.tv_sec;

				return tm.tv_usec >= msg.tm.tv_usec;
			}

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			UniSetTypes::ByteOfMessage data[sizeof(UniSetTypes::RawDataOfTransportMessage)-sizeof(Message)];
	};

	/*! Сообщение об изменении состояния датчика */
	class SensorMessage : public Message
	{
		public:

			ObjectId id;
			bool state;
			long value;
			bool undefined;

			// время изменения состояния датчика
			long sm_tv_sec;
			long sm_tv_usec;

			UniversalIO::IOTypes sensor_type;
			IOController_i::CalibrateInfo ci;
			
			// для пороговых датчиков
			bool threshold;
			UniSetTypes::ThresholdId tid;

			SensorMessage();
			SensorMessage(ObjectId id, bool state, Priority priority = Message::Medium, 
							UniversalIO::IOTypes st = UniversalIO::DigitalInput,
							ObjectId consumer=UniSetTypes::DefaultObjectId);

			SensorMessage(ObjectId id, long value, IOController_i::CalibrateInfo ci,
							Priority priority = Message::Medium, 
							UniversalIO::IOTypes st = UniversalIO::AnalogInput,
							ObjectId consumer=UniSetTypes::DefaultObjectId);

			SensorMessage(const VoidMessage *msg);
			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
	};

	/*! Системное сообщение */
	class SystemMessage : public Message
	{
		public:
			enum Command
			{
				StartUp,	/*! начать работу */
				FoldUp, 	/*! нет связи с главной станцией */
				Finish,		/*! завершить работу */
				WatchDog,	/*! контроль состояния */
				ReConfiguration,		/*! обновились параметры конфигурации */
				NetworkInfo,			/*! обновилась информация о состоянии узлов в сети
											поля 
											data[0]	- кто 
											data[1] - новое состояние(true - connect,  false - disconnect)
										 */
				LogRotate	/*! переоткрыть файлы логов */
			};
			
			SystemMessage();
			SystemMessage(Command command, Priority priority = Message::High, 
							ObjectId consumer=UniSetTypes::DefaultObjectId);
			SystemMessage(const VoidMessage *msg);

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			int command;
			long data[2];
	};

	/*! Информационное собщение оператору */
	class InfoMessage : public Message
	{
		public:
			enum Character{
								Normal,
								Warning
							};
			InfoMessage();
			InfoMessage(ObjectId id,  std::string str, ObjectId node = conf->getLocalNode(), 
						Character ch = InfoMessage::Normal, 
						Priority priority = Message::Medium, ObjectId consumer=UniSetTypes::DefaultObjectId);

			InfoMessage(ObjectId id, MessageCode icode, ObjectId node = conf->getLocalNode(), 
						Character ch = InfoMessage::Normal, 
						Priority priority = Message::Medium, ObjectId consumer=UniSetTypes::DefaultObjectId);

			InfoMessage(const VoidMessage *msg);

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			ObjectId id;					/*!< от кого */
			MessageCode infocode;			/*!< код сообщения */
			Character character;			/*!< характер */
			bool broadcast;					/*!< флаг рассылки на другие узлы */
	
			/*! 
				признак, что сообщение является пересланным. 
				(т.е. в БД второй раз сохранять не надо, пересылать
				 второй раз тоже не надо). 
			*/
			bool route;

			// т.к. размер транспортных сообщений ограничен, то 
			// мы можем высчитать максимальную длину текстового сообщения
			// считаем размер текстового сообщения по формуле
			// размерТраснпортного - РазмерБазового-суммаВсехВспомПолейДляДанногоСообщения-1
			// всё это конечно не очень хорошо, но пока так.
//			static const int size_of_info_message = sizeof(UniSetTypes::RawDataOfTransportMessage)-sizeof(Message)-sizeof(ObjectId)-sizeof(MessageCode)-sizeof(Character)-2*sizeof(bool)-1;
			// пока делаем размер постоянным
			static const unsigned int size_of_info_message = 55;
			char message[size_of_info_message];	/*!< сообщение */
	};

	/*! Собщение об аварии */
	class AlarmMessage : public Message
	{
		public:
			enum Character{
								Normal,
								Attention,
								Warning,
								Alarm
							};

			AlarmMessage();
			AlarmMessage(ObjectId id,  std::string str, 	ObjectId node = conf->getLocalNode(), 
						Character ch = AlarmMessage::Alarm, 
						Priority prior = Message::Medium, ObjectId cons=UniSetTypes::DefaultObjectId);

			AlarmMessage(ObjectId id,  std::string str, MessageCode ccode, 
							ObjectId node = conf->getLocalNode(), 				
							Character ch = AlarmMessage::Alarm, 
							Priority prior = Message::Medium, ObjectId cons=UniSetTypes::DefaultObjectId);

			AlarmMessage(ObjectId id,  MessageCode acode, MessageCode ccode, 
							ObjectId node=conf->getLocalNode(), 
							Character ch=AlarmMessage::Alarm, 
							Priority prior=Message::Medium, 
							ObjectId cons=UniSetTypes::DefaultObjectId);

			AlarmMessage(const VoidMessage *msg);

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
			
			ObjectId id;					/*!< от кого */
			MessageCode alarmcode;			/*!< код сообщения */
			MessageCode causecode;			/*!< код причины */
			Character character;			/*!< характер */
			bool broadcast;					/*!< флаг рассылки на другие узлы */

			/*! 
				признак, что сообщение является пересланным. 
				(т.е. в БД второй раз сохранять не надо, пересылать
				 второй раз тоже не надо). 
			*/
			bool route;


			// т.к. размер транспортных сообщений ограничен, то 
			// мы можем высчитать максимальную длину текстового сообщения
			// считаем размер текстового сообщения по формуле
			// размерТраснпортного - РазмерБазового-суммаВсехВспомПолейДляДанногоСообщения-1
			// всё это конечно не очень хорошо, но пока так.
			// sizeof(UniSetTypes::RawDataOfTransportMessage)-sizeof(Message)-sizeof(ObjectId)-2*sizeof(MessageCode)-sizeof(Character)-2*sizeof(bool)-1

			// пока делаем размер постоянным
			static const unsigned int size_of_alarm_message = 55;
			char message[size_of_alarm_message];	/*!< сообщение */			
	};

	/*! Собщение DBServer-у */
	class DBMessage : public Message
	{
		public:
			enum TypeOfQuery
			{
				Query,
				Update,
				Insert
			};
			
			DBMessage();

			DBMessage(TypeOfQuery qtype, const std::string query, TypeOfMessage tblid,
						Priority prior=Message::Low, 
						ObjectId cons=UniSetTypes::DefaultObjectId);
			DBMessage(const VoidMessage *msg);

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
			DBMessage::TypeOfQuery qtype;	// тип данных в запросе
			TypeOfMessage tblid;			// идентификатор таблицы


			// т.к. размер трансопртынх сообщений ограничен, то 
			// мы можем высчитать максимальнйю длину текстового запроса
			// считаем размер текстового сообщения по формуле
			// размерТраснпортного - РазмерБазового-суммаВсехВспомПолейДляДанногоСообщения-1
			// всё это конечно не очень хорошо, но пока так.
			// sizeof(UniSetTypes::RawDataOfTransportMessage)-sizeof(Message)-sizeof(DBMessage::TypeOfQuery)-sizeof(TypeOfMessage)-1

			static const unsigned int size_of_query = 55;
			char data[size_of_query];	
	};
	
	/*! Собщение о срабатывании таймера */
	class TimerMessage : public Message
	{
		public:
			TimerMessage();
			TimerMessage(UniSetTypes::TimerId id, Priority prior = Message::High,
							ObjectId cons=UniSetTypes::DefaultObjectId);
			TimerMessage(const VoidMessage *msg);
			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
			
			UniSetTypes::TimerId id; /*!< id сработавшего таймера */
	};

	/*! Подтверждение(квитирование) сообщения */
	class ConfirmMessage: public Message
	{
		public:

			ConfirmMessage(const InfoMessage& msg, Priority prior = Message::High,
							ObjectId cons=UniSetTypes::DefaultObjectId);

			ConfirmMessage(const AlarmMessage& msg, Priority prior = Message::High,
							ObjectId cons=UniSetTypes::DefaultObjectId);
			ConfirmMessage(const VoidMessage *msg);

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
			
			MessageCode code; 		/*!< id подтвержденного события */
			MessageCode orig_cause;	/*!< причина */
			timeval orig_tm;		/*!< время сообщения */
			int orig_type;			/*!< тип подтвержденного сообщения */
			ObjectId orig_node;		/*!< узел */
			ObjectId orig_id;		/*!< от кого оно было */
			bool broadcast;

			/*! 
				признак, что сообщение является пересланным. 
				(т.е. в БД второй раз сохранять не надо, пересылать
				 второй раз тоже не надо). 
			*/
			bool route;

		protected:
			ConfirmMessage();
	};

}
// --------------------------------------------------------------------------
#endif // MessageType_H_
