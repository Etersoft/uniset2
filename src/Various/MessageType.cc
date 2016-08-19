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
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------

#include <chrono>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "Configuration.h"
#include "UniSetTypes.h"
#include "MessageType.h"

namespace UniSetTypes
{
	//--------------------------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const Message::TypeOfMessage& t )
	{
		if( t == Message::Unused )
			return os << "Unused";

		if( t == Message::SensorInfo )
			return os << "SensorInfo";

		if( t == Message::SysCommand )
			return os << "SysCommand";

		if( t == Message::Confirm )
			return os << "Confirm";

		if( t == Message::Timer )
			return os << "Timer";

		return os << "Unkown";
	}
	//--------------------------------------------------------------------------------------------
	Message::Message():
		type(Unused), priority(Medium),
		node( UniSetTypes::uniset_conf() ? UniSetTypes::uniset_conf()->getLocalNode() : DefaultObjectId ),
		supplier(DefaultObjectId),
		consumer(DefaultObjectId)
	{
		tm = UniSetTypes::now_to_timespec();
	}

	//--------------------------------------------------------------------------------------------

	VoidMessage::VoidMessage( const TransportMessage& tm ):
		Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
	{
		assert(sizeof(VoidMessage) >= sizeof(UniSetTypes::RawDataOfTransportMessage));
		memcpy(this, &tm.data, sizeof(tm.data));
		consumer = tm.consumer;
	}

	VoidMessage::VoidMessage()
	{
		assert(sizeof(VoidMessage) >= sizeof(UniSetTypes::RawDataOfTransportMessage));
	}

	//--------------------------------------------------------------------------------------------
	SensorMessage::SensorMessage():
		id(DefaultObjectId),
		value(0),
		undefined(false),
		sensor_type(UniversalIO::DI),
		threshold(false),
		tid(UniSetTypes::DefaultThresholdId)
	{
		type    = Message::SensorInfo;
		sm_tv   = tm; // или инициализировать нулём ?
		ci.minRaw = 0;
		ci.maxRaw = 0;
		ci.minCal = 0;
		ci.maxCal = 0;
		ci.precision = 0;
	}

	SensorMessage::SensorMessage(ObjectId id, long value, const IOController_i::CalibrateInfo& ci,
								 Priority priority,
								 UniversalIO::IOType st, ObjectId consumer):
		id(id),
		value(value),
		undefined(false),
		sensor_type(st),
		ci(ci),
		threshold(false),
		tid(UniSetTypes::DefaultThresholdId)
	{
		type            = Message::SensorInfo;
		this->priority     = priority;
		this->consumer     = consumer;
		sm_tv = tm;
	}

	SensorMessage::SensorMessage(const VoidMessage* msg):
		Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
	{
		memcpy(this, msg, sizeof(*this));
		assert(this->type == Message::SensorInfo);
	}
	//--------------------------------------------------------------------------------------------
	SystemMessage::SystemMessage():
		command(SystemMessage::Unknown)
	{
		memset(data, 0, sizeof(data));
		type = Message::SysCommand;
	}

	SystemMessage::SystemMessage(Command command, Priority priority, ObjectId consumer):
		command(command)
	{
		type = Message::SysCommand;
		this->priority = priority;
		this->consumer = consumer;
	}

	SystemMessage::SystemMessage(const VoidMessage* msg):
		Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
	{
		memcpy(this, msg, sizeof(*this));
		assert(this->type == Message::SysCommand);
	}
	//--------------------------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, const SystemMessage::Command& c )
	{
		if( c == SystemMessage::Unknown )
			return os << "Unknown";

		if( c == SystemMessage::StartUp )
			return os << "StartUp";

		if( c == SystemMessage::FoldUp )
			return os << "FoldUp";

		if( c == SystemMessage::Finish )
			return os << "Finish";

		if( c == SystemMessage::WatchDog )
			return os << "WatchDog";

		if( c == SystemMessage::ReConfiguration )
			return os << "ReConfiguration";

		if( c == SystemMessage::NetworkInfo )
			return os << "NetworkInfo";

		if( c == SystemMessage::LogRotate )
			return os << "LogRotate";

		return os;
	}
	//--------------------------------------------------------------------------------------------
	TimerMessage::TimerMessage():
		id(UniSetTypes::DefaultTimerId)
	{
		type = Message::Timer;
	}

	TimerMessage::TimerMessage(UniSetTypes::TimerId id, Priority prior, ObjectId cons):
		id(id)
	{
		type = Message::Timer;
		this->consumer = cons;
	}

	TimerMessage::TimerMessage(const VoidMessage* msg):
		Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
	{
		memcpy(this, msg, sizeof(*this));
		assert(this->type == Message::Timer);
	}
	//--------------------------------------------------------------------------------------------
	ConfirmMessage::ConfirmMessage( const VoidMessage* msg ):
		Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
	{
		memcpy(this, msg, sizeof(*this));
		assert(this->type == Message::Confirm);
	}
	//--------------------------------------------------------------------------------------------
	ConfirmMessage::ConfirmMessage(UniSetTypes::ObjectId in_sensor_id,
									double in_sensor_value,
									const timespec& in_sensor_time,
									const timespec& in_confirm_time,
									Priority in_priority ):
		sensor_id(in_sensor_id),
		sensor_value(in_sensor_value),
		sensor_time(in_sensor_time),
		confirm_time(in_confirm_time),
		broadcast(false),
		forward(false)
	{
		type = Message::Confirm;
		priority = in_priority;
	}

	//--------------------------------------------------------------------------------------------
} // end of namespace UniSetTypes
//--------------------------------------------------------------------------------------------

