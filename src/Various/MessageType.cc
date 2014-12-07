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
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 

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
    node(UniSetTypes::uniset_conf()->getLocalNode()),
    supplier(DefaultObjectId),
    consumer(DefaultObjectId)
{
    struct timezone tz;
    tm.tv_sec = 0;
    tm.tv_usec = 0;
    gettimeofday(&tm,&tz);
}

/*
template<class In>
TransportMessage Message::transport(const In &msg)
{
    TransportMessage tmsg;
    assert(sizeof(UniSetTypes::RawDataOfTransportMessage)>=sizeof(msg));
    memcpy(&tmsg.data,&msg,sizeof(msg));
    return tmsg;
}
*/
//--------------------------------------------------------------------------------------------

VoidMessage::VoidMessage( const TransportMessage& tm ):
    Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
{
    assert(sizeof(VoidMessage)>=sizeof(UniSetTypes::RawDataOfTransportMessage));
    memcpy(this,&tm.data,sizeof(tm.data));
}

VoidMessage::VoidMessage()
{
    assert(sizeof(VoidMessage)>=sizeof(UniSetTypes::RawDataOfTransportMessage));
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
    type        = Message::SensorInfo;
    sm_tv_sec   = tm.tv_sec;
    sm_tv_usec  = tm.tv_usec;

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
    sm_tv_sec         = tm.tv_sec;
    sm_tv_usec         = tm.tv_usec;
}

SensorMessage::SensorMessage(const VoidMessage *msg):
    Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
{
    memcpy(this,msg,sizeof(*this));
    assert(this->type == Message::SensorInfo);
}
//--------------------------------------------------------------------------------------------
SystemMessage::SystemMessage():
    command(SystemMessage::Unknown)
{
	memset(data,0,sizeof(data));
    type = Message::SysCommand;
}

SystemMessage::SystemMessage(Command command, Priority priority, ObjectId consumer):
    command(command)
{
    type = Message::SysCommand;
    this->priority = priority;
    this->consumer = consumer;
}

SystemMessage::SystemMessage(const VoidMessage *msg):
    Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
{
    memcpy(this,msg,sizeof(*this));
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

	return os << "";
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

TimerMessage::TimerMessage(const VoidMessage *msg):
    Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
{
    memcpy(this,msg,sizeof(*this));
    assert(this->type == Message::Timer);
}
//--------------------------------------------------------------------------------------------    
ConfirmMessage::ConfirmMessage( const VoidMessage *msg ):
    Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
{
    memcpy(this,msg,sizeof(*this));
    assert(this->type == Message::Confirm);
}
//--------------------------------------------------------------------------------------------
ConfirmMessage::ConfirmMessage(long in_sensor_id,
                    double in_value,
                    time_t in_time,
                    time_t in_time_usec,
                    time_t in_confirm,
                    Priority in_priority ):
   sensor_id(in_sensor_id),
   value(in_value),
   time(in_time),
   time_usec(in_time_usec),
   confirm(in_confirm),
   broadcast(false),
   route(false)
{
   type = Message::Confirm;
   priority = in_priority;
}

//--------------------------------------------------------------------------------------------    
} // end of namespace UniSetTypes
//--------------------------------------------------------------------------------------------    

