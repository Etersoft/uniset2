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
				Unused,		// Сообщение не содержит информации
				SensorInfo,
				SysCommand, // Сообщение содержит системную команду
				Confirm,	// Сообщение содержит подтверждение
				Timer,		// Сообщения о срабатывании таймера
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

            // для оптимизации, делаем конструктор который не будет инициализировать свойства класса
            // это необходимо для VoidMessage, который конструируется при помощи memcpy
            Message( int dummy_init ){}

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
			long value;
			bool undefined;

			// время изменения состояния датчика
			long sm_tv_sec;
			long sm_tv_usec;

			UniversalIO::IOType sensor_type;
			IOController_i::CalibrateInfo ci;
			
			// для пороговых датчиков
			bool threshold;  /*!< TRUE - сработал порог, FALSE - порог отключился */
			UniSetTypes::ThresholdId tid;

			SensorMessage();
			SensorMessage(ObjectId id, long value, const IOController_i::CalibrateInfo& ci=IOController_i::CalibrateInfo(),
							Priority priority = Message::Medium, 
							UniversalIO::IOType st = UniversalIO::AI,
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

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
			
			ConfirmMessage( const VoidMessage *msg );

			ConfirmMessage(long in_sensor_id,
					double in_value,
					time_t in_time,
					time_t in_time_usec,
					time_t in_confirm,
					Priority in_priority = Message::Medium);


			long sensor_id;   /* ID датчика */
			double value;     /* значение датчика */
			time_t time;      /* время, когда датчик получил сигнал */
			time_t time_usec; /* время в микросекундах */
			time_t confirm;   /* время, когда произошло квитирование */

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
