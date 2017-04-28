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
#include <string>
#include <ostream>
#include "UniSetTypes.h"
#include "IOController_i.hh"
// --------------------------------------------------------------------------
namespace uniset
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
			ObjectId node = { uniset::DefaultObjectId };      // откуда
			ObjectId supplier = { uniset::DefaultObjectId };  // от кого
			ObjectId consumer = { uniset::DefaultObjectId };  // кому
			struct timespec tm = { 0, 0 };

			Message( Message&& ) noexcept = default;
			Message& operator=(Message&& ) noexcept = default;
			Message( const Message& ) noexcept = default;
			Message& operator=(const Message& ) noexcept = default;

			Message() noexcept;

			// для оптимизации, делаем конструктор который не будет инициализировать свойства класса
			// это необходимо для VoidMessage, который конструируется при помощи memcpy
			explicit Message( int dummy_init ) noexcept {}

			template<class In>
			static const TransportMessage transport(const In& msg) noexcept
			{
				TransportMessage tmsg;
				assert(sizeof(uniset::RawDataOfTransportMessage) >= sizeof(msg));
				std::memcpy(&tmsg.data, &msg, sizeof(msg));
				tmsg.consumer = msg.consumer;
				return std::move(tmsg);
			}
	};

	std::string strTypeOfMessage( int type );
	std::ostream& operator<<( std::ostream& os, const Message::TypeOfMessage& t );

	// ------------------------------------------------------------------------
	class VoidMessage : public Message
	{
		public:

			VoidMessage( VoidMessage&& ) noexcept = default;
			VoidMessage& operator=(VoidMessage&& ) noexcept = default;
			VoidMessage( const VoidMessage& ) noexcept = default;
			VoidMessage& operator=( const VoidMessage& ) noexcept = default;

			// для оптимизации, делаем конструктор который не будет инициализировать свойства класса
			// это необходимо для VoidMessage, который конструируется при помощи memcpy
			VoidMessage( int dummy ) noexcept : Message(dummy) {}

			VoidMessage( const TransportMessage& tm ) noexcept;
			VoidMessage() noexcept;
			inline bool operator < ( const VoidMessage& msg ) const
			{
				if( priority != msg.priority )
					return priority < msg.priority;

				if( tm.tv_sec != msg.tm.tv_sec )
					return tm.tv_sec >= msg.tm.tv_sec;

				return tm.tv_nsec >= msg.tm.tv_nsec;
			}

			inline TransportMessage transport_msg() const noexcept
			{
				return transport(*this);
			}

			uniset::ByteOfMessage data[sizeof(uniset::RawDataOfTransportMessage) - sizeof(Message)];
	};

	// ------------------------------------------------------------------------
	/*! Сообщение об изменении состояния датчика */
	class SensorMessage : public Message
	{
		public:

			ObjectId id = { uniset::DefaultObjectId };
			long value = { 0 };
			bool undefined = { false };

			// время изменения состояния датчика
			struct timespec sm_tv = { 0, 0 };

			UniversalIO::IOType sensor_type = { UniversalIO::DI };
			IOController_i::CalibrateInfo ci;

			// для пороговых датчиков
			bool threshold = { false };  /*!< TRUE - сработал порог, FALSE - порог отключился */
			uniset::ThresholdId tid = { uniset::DefaultThresholdId };

			SensorMessage( SensorMessage&& m) noexcept = default;
			SensorMessage& operator=(SensorMessage&& m) noexcept = default;
			SensorMessage( const SensorMessage& ) noexcept = default;
			SensorMessage& operator=( const SensorMessage& ) noexcept = default;

			SensorMessage() noexcept;
			SensorMessage(ObjectId id, long value, const IOController_i::CalibrateInfo& ci = IOController_i::CalibrateInfo(),
						  Priority priority = Message::Medium,
						  UniversalIO::IOType st = UniversalIO::AI,
						  ObjectId consumer = uniset::DefaultObjectId) noexcept;

			// специальный конструктор, для оптимизации
			// он не инициализирует поля по умолчанию
			// и за инициализацию значений отвечает "пользователь"
			// например см. IONotifyController::localSetValue()
			explicit SensorMessage( int dummy ) noexcept;

			SensorMessage(const VoidMessage* msg) noexcept;
			inline TransportMessage transport_msg() const noexcept
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

			SystemMessage( SystemMessage&& ) noexcept = default;
			SystemMessage& operator=(SystemMessage&& ) noexcept = default;
			SystemMessage( const SystemMessage& ) noexcept = default;
			SystemMessage& operator=( const SystemMessage& ) noexcept = default;

			SystemMessage() noexcept;
			SystemMessage(Command command, Priority priority = Message::High,
						  ObjectId consumer = uniset::DefaultObjectId) noexcept;
			SystemMessage(const VoidMessage* msg) noexcept;

			inline TransportMessage transport_msg() const noexcept
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
			TimerMessage( TimerMessage&& ) noexcept = default;
			TimerMessage& operator=(TimerMessage&& ) noexcept = default;
			TimerMessage( const TimerMessage& ) noexcept = default;
			TimerMessage& operator=( const TimerMessage& ) noexcept = default;

			TimerMessage();
			TimerMessage(uniset::TimerId id, Priority prior = Message::High,
						 ObjectId cons = uniset::DefaultObjectId);
			TimerMessage(const VoidMessage* msg) noexcept ;
			inline TransportMessage transport_msg() const noexcept
			{
				return transport(*this);
			}

			uniset::TimerId id; /*!< id сработавшего таймера */
	};

	// ------------------------------------------------------------------------

	/*! Подтверждение(квитирование) сообщения */
	class ConfirmMessage: public Message
	{
		public:

			inline TransportMessage transport_msg() const noexcept
			{
				return transport(*this);
			}

			ConfirmMessage( const VoidMessage* msg ) noexcept;

			ConfirmMessage(ObjectId in_sensor_id,
						   const double& in_sensor_value,
						   const timespec& in_sensor_time,
						   const timespec& in_confirm_time,
						   Priority in_priority = Message::Medium) noexcept;

			ConfirmMessage( ConfirmMessage&& ) noexcept = default;
			ConfirmMessage& operator=(ConfirmMessage&& ) noexcept = default;
			ConfirmMessage( const ConfirmMessage& ) noexcept = default;
			ConfirmMessage& operator=( const ConfirmMessage& ) noexcept = default;

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
			ConfirmMessage() noexcept;
	};

}
// --------------------------------------------------------------------------
#endif // MessageType_H_
