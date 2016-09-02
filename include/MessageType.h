/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
#include <time.h> // for timespec
#include <cstring>
#include <ostream>
#include "UniSetTypes.h"
#include "IOController_i.hh"
// --------------------------------------------------------------------------
namespace UniSetTypes
{
	class Message
	{
		public:
			enum TypeOfMessage
			{
				Unused,        // Сообщение не содержит информации
				SensorInfo,
				SysCommand, // Сообщение содержит системную команду
				Confirm,    // Сообщение содержит подтверждение
				Timer,        // Сообщения о срабатывании таймера
				TheLastFieldOfTypeOfMessage // Обязательно оставьте последним
			};

			int type = { Unused };    // Содержание сообщения (тип)

			enum Priority
			{
				Low,
				Medium,
				High
			};

			Priority priority = { Medium };
			ObjectId node = { UniSetTypes::DefaultObjectId };      // откуда
			ObjectId supplier = { UniSetTypes::DefaultObjectId };  // от кого
			ObjectId consumer = { UniSetTypes::DefaultObjectId };  // кому
			struct timespec tm = { 0, 0 };

			Message( Message&& ) = default;
			Message& operator=(Message&& ) = default;
			Message( const Message& ) = default;
			Message& operator=(const Message& ) = default;

			Message();

			// для оптимизации, делаем конструктор который не будет инициализировать свойства класса
			// это необходимо для VoidMessage, который конструируется при помощи memcpy
			Message( int dummy_init ) {}

			template<class In>
			static const TransportMessage transport(const In& msg)
			{
				TransportMessage tmsg;
				assert(sizeof(UniSetTypes::RawDataOfTransportMessage) >= sizeof(msg));
				std::memcpy(&tmsg.data, &msg, sizeof(msg));
				tmsg.consumer = msg.consumer;
				return std::move(tmsg);
			}
	};

	std::ostream& operator<<( std::ostream& os, const Message::TypeOfMessage& t );

	// ------------------------------------------------------------------------
	class VoidMessage : public Message
	{
		public:

			VoidMessage( VoidMessage&& ) = default;
			VoidMessage& operator=(VoidMessage&& ) = default;
			VoidMessage( const VoidMessage& ) = default;
			VoidMessage& operator=( const VoidMessage& ) = default;

			// для оптимизации, делаем конструктор который не будет инициализировать свойства класса
			// это необходимо для VoidMessage, который конструируется при помощи memcpy
			VoidMessage( int dummy ): Message(dummy) {}

			VoidMessage( const TransportMessage& tm );
			VoidMessage();
			inline bool operator < ( const VoidMessage& msg ) const
			{
				if( priority != msg.priority )
					return priority < msg.priority;

				if( tm.tv_sec != msg.tm.tv_sec )
					return tm.tv_sec >= msg.tm.tv_sec;

				return tm.tv_nsec >= msg.tm.tv_nsec;
			}

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			UniSetTypes::ByteOfMessage data[sizeof(UniSetTypes::RawDataOfTransportMessage) - sizeof(Message)];
	};

	// ------------------------------------------------------------------------
	/*! Сообщение об изменении состояния датчика */
	class SensorMessage : public Message
	{
		public:

			ObjectId id;
			long value;
			bool undefined;

			// время изменения состояния датчика
			struct timespec sm_tv;

			UniversalIO::IOType sensor_type;
			IOController_i::CalibrateInfo ci;

			// для пороговых датчиков
			bool threshold;  /*!< TRUE - сработал порог, FALSE - порог отключился */
			UniSetTypes::ThresholdId tid;

			SensorMessage( SensorMessage&& m) = default;
			SensorMessage& operator=(SensorMessage&& m) = default;
			SensorMessage( const SensorMessage& ) = default;
			SensorMessage& operator=( const SensorMessage& ) = default;

			SensorMessage();
			SensorMessage(ObjectId id, long value, const IOController_i::CalibrateInfo& ci = IOController_i::CalibrateInfo(),
						  Priority priority = Message::Medium,
						  UniversalIO::IOType st = UniversalIO::AI,
						  ObjectId consumer = UniSetTypes::DefaultObjectId);

			SensorMessage(const VoidMessage* msg);
			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}
	};

	// ------------------------------------------------------------------------
	/*! Системное сообщение */
	class SystemMessage : public Message
	{
		public:
			enum Command
			{
				Unknown,
				StartUp,    /*! начать работу */
				FoldUp,     /*! нет связи с главной станцией */
				Finish,     /*! завершить работу */
				WatchDog,   /*! контроль состояния */
				ReConfiguration,        /*! обновились параметры конфигурации */
				NetworkInfo,            /*! обновилась информация о состоянии узлов в сети
                                            поля
											data[0] - кто
                                            data[1] - новое состояние(true - connect,  false - disconnect)
                                         */
				LogRotate,    /*! переоткрыть файлы логов */
				TheLastFieldOfCommand
			};

			SystemMessage( SystemMessage&& ) = default;
			SystemMessage& operator=(SystemMessage&& ) = default;
			SystemMessage( const SystemMessage& ) = default;
			SystemMessage& operator=( const SystemMessage& ) = default;

			SystemMessage();
			SystemMessage(Command command, Priority priority = Message::High,
						  ObjectId consumer = UniSetTypes::DefaultObjectId);
			SystemMessage(const VoidMessage* msg);

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			int command;
			long data[2];
	};
	std::ostream& operator<<( std::ostream& os, const SystemMessage::Command& c );

	// ------------------------------------------------------------------------

	/*! Собщение о срабатывании таймера */
	class TimerMessage : public Message
	{
		public:
			TimerMessage( TimerMessage&& ) = default;
			TimerMessage& operator=(TimerMessage&& ) = default;
			TimerMessage( const TimerMessage& ) = default;
			TimerMessage& operator=( const TimerMessage& ) = default;

			TimerMessage();
			TimerMessage(UniSetTypes::TimerId id, Priority prior = Message::High,
						 ObjectId cons = UniSetTypes::DefaultObjectId);
			TimerMessage(const VoidMessage* msg);
			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			UniSetTypes::TimerId id; /*!< id сработавшего таймера */
	};

	// ------------------------------------------------------------------------

	/*! Подтверждение(квитирование) сообщения */
	class ConfirmMessage: public Message
	{
		public:

			inline TransportMessage transport_msg() const
			{
				return transport(*this);
			}

			ConfirmMessage( const VoidMessage* msg );

			ConfirmMessage(ObjectId in_sensor_id,
						   const double& in_sensor_value,
						   const timespec& in_sensor_time,
						   const timespec& in_confirm_time,
						   Priority in_priority = Message::Medium);

			ConfirmMessage( ConfirmMessage&& ) = default;
			ConfirmMessage& operator=(ConfirmMessage&& ) = default;
			ConfirmMessage( const ConfirmMessage& ) = default;
			ConfirmMessage& operator=( const ConfirmMessage& ) = default;

			ObjectId sensor_id;   /* ID датчика (события) */
			double sensor_value;  /* значение датчика (события) */
			struct timespec sensor_time;	/* время срабатывания датчика(события), который квитируем */
			struct timespec confirm_time;   /* * время прошедшее до момента квитирования */

			bool broadcast;

			/*!
			    признак, что сообщение является пересланным.
			    (т.е. в БД второй раз сохранять не надо, пересылать
			     второй раз тоже не надо).
			*/
			bool forward;

		protected:
			ConfirmMessage();
	};

}
// --------------------------------------------------------------------------
#endif // MessageType_H_
