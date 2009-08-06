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
 *  \date   $Date: 2007/06/17 21:30:58 $
 *  \version $Id: MessageType.cc,v 1.15 2007/06/17 21:30:58 vpashka Exp $
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
Message::Message():
	type(Unused), priority(Medium),
	node(UniSetTypes::conf->getLocalNode()),
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

VoidMessage::VoidMessage( const TransportMessage& tm )
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
state(false),
value(0),
undefined(false),
sensor_type(UniversalIO::DigitalInput),
threshold(false),
tid(UniSetTypes::DefaultThresholdId)
{
	type		= Message::SensorInfo;
	sm_tv_sec 	= tm.tv_sec;
	sm_tv_usec 	= tm.tv_usec;

	ci.minRaw = 0;
	ci.maxRaw = 0;
	ci.minCal = 0;
	ci.maxCal = 0;
	ci.sensibility = 0;
	ci.precision = 0;
}

SensorMessage::SensorMessage(ObjectId id, bool state, Priority priority, 
							UniversalIO::IOTypes st, ObjectId consumer):
id(id),
state(state),
value(0),
undefined(false),
sensor_type(st),
threshold(false),
tid(UniSetTypes::DefaultThresholdId)
{
	value = state ? 1:0;

	type			= Message::SensorInfo;
	this->priority	= priority;
	this->consumer 	= consumer;
	sm_tv_sec 		= tm.tv_sec;
	sm_tv_usec 		= tm.tv_usec;

	ci.minRaw = 0;
	ci.maxRaw = 0;
	ci.minCal = 0;
	ci.maxCal = 0;
	ci.sensibility = 0;
	ci.precision = 0;
}

SensorMessage::SensorMessage(ObjectId id, long value, IOController_i::CalibrateInfo ci,
							Priority priority, 
							UniversalIO::IOTypes st, ObjectId consumer):
id(id),
state(false),
value(value),
undefined(false),
sensor_type(st),
ci(ci),
threshold(false),
tid(UniSetTypes::DefaultThresholdId)
{
	state = value != 0;
	type			= Message::SensorInfo;
	this->priority 	= priority;
	this->consumer 	= consumer;	
	sm_tv_sec 		= tm.tv_sec;
	sm_tv_usec 		= tm.tv_usec;
}

SensorMessage::SensorMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::SensorInfo);
}
//--------------------------------------------------------------------------------------------
SystemMessage::SystemMessage():
	command(SystemMessage::ReConfiguration)
{
	type = Message::SysCommand;
}

SystemMessage::SystemMessage(Command command, Priority priority, ObjectId consumer):
	command(command)
{
	type = Message::SysCommand;
	this->priority = priority;
	this->consumer = consumer;
}

SystemMessage::SystemMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::SysCommand);
}
//--------------------------------------------------------------------------------------------

InfoMessage::InfoMessage():
id(DefaultObjectId),
infocode( UniSetTypes::DefaultMessageCode ),
character(InfoMessage::Normal),
broadcast(false),
route(false)
{
	type = Message::Info;
}
InfoMessage::InfoMessage(ObjectId id,  string str, ObjectId node, Character ch, 
							Priority priority, ObjectId consumer):
id(id),
infocode( UniSetTypes::DefaultMessageCode ),
character(ch),
broadcast(true),
route(false)
{
	assert(str.size() < size_of_info_message);
	this->node = node;
	type = Message::Info;
	this->priority = priority;
	strcpy(message, str.c_str());
	this->consumer = consumer;
}

InfoMessage::InfoMessage(ObjectId id, MessageCode icode, ObjectId node, 
						Character ch, Priority priority, ObjectId consumer):
id(id),
infocode(icode),
character(ch),
broadcast(true),
route(false)
{
	this->node = node;
	type = Message::Info;
	this->priority = priority;
	this->consumer = consumer;
}

InfoMessage::InfoMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::Info);
}
//--------------------------------------------------------------------------------------------
AlarmMessage::AlarmMessage():
id(DefaultObjectId),
alarmcode(UniSetTypes::DefaultMessageCode),
causecode(UniSetTypes::DefaultMessageCode),
character(AlarmMessage::Normal),
broadcast(false),
route(false)
{
	type = Message::Alarm;
}

AlarmMessage::AlarmMessage(ObjectId id,  string str, ObjectId node, 
						Character ch, Priority prior, ObjectId cons):
id(id),
alarmcode(UniSetTypes::DefaultMessageCode),
causecode(UniSetTypes::DefaultMessageCode),
character(ch),
broadcast(true),
route(false)
{
	assert(str.size() < size_of_alarm_message);
	this->node = node;
	type = Message::Alarm;
	this->priority = prior;
	strcpy(message, str.c_str());
	this->consumer = cons;
}

AlarmMessage::AlarmMessage(ObjectId id,  string str, MessageCode ccode, 
							ObjectId node, Character ch, 
							Priority prior, ObjectId cons):
id(id),
alarmcode(UniSetTypes::DefaultMessageCode),
causecode(ccode),
character(ch),
broadcast(true),
route(false)
{
	assert(str.size() < size_of_alarm_message);
	this->node = node;
	type = Message::Alarm;
	this->priority = prior;
	strcpy(message, str.c_str());
	this->consumer = cons;
}

AlarmMessage::AlarmMessage(ObjectId id,  MessageCode acode, MessageCode ccode, 
							ObjectId node, Character ch, 
							Priority prior, ObjectId cons):
id(id),
alarmcode(acode),
causecode(ccode),
character(ch),
broadcast(true),
route(false)
{
	this->node = node;
	type = Message::Alarm;
	this->priority = prior;
	this->consumer = cons;
}

AlarmMessage::AlarmMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::Alarm);
}
//--------------------------------------------------------------------------------------------
DBMessage::DBMessage():
qtype(DBMessage::Query),
tblid(Unused)
{
	type = Message::DataBase;
}

DBMessage::DBMessage(DBMessage::TypeOfQuery qtype, const string query, TypeOfMessage tblid,
					Priority prior, ObjectId cons):
qtype(qtype),
tblid(tblid)
{
	assert(query.size() < size_of_query);
	type = Message::DataBase;
	priority = prior;				
	this->consumer = cons;
	strcpy(data, query.c_str());
}

DBMessage::DBMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::DataBase);
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
	this->priority = prior;
	this->consumer = cons;
}

TimerMessage::TimerMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::Timer);
}
//--------------------------------------------------------------------------------------------	

ConfirmMessage::ConfirmMessage():
code(UniSetTypes::DefaultMessageCode),
orig_cause(DefaultMessageCode),
orig_type(Message::Info),
orig_node(DefaultObjectId),
orig_id(UniSetTypes::DefaultMessageCode),
broadcast(false),
route(false)
{
}

ConfirmMessage::ConfirmMessage(const InfoMessage& msg, Priority prior, ObjectId cons):
code(msg.infocode),
orig_cause(DefaultMessageCode),
orig_tm(msg.tm),
orig_type(msg.type),
orig_node(msg.node),
orig_id(msg.id),
broadcast(msg.broadcast),
route(false)
{
	type = Message::Confirm;
	this->priority = prior;
	this->consumer = cons;
//	struct timeb tm;
}

ConfirmMessage::ConfirmMessage(const AlarmMessage& msg, Priority prior, ObjectId cons):
code(msg.alarmcode),
orig_cause(msg.causecode),
orig_tm(msg.tm),
orig_type(msg.type),
orig_node(msg.node),
orig_id(msg.id),
broadcast(msg.broadcast),
route(false)
{
	type = Message::Confirm;
	this->priority = prior;
	this->consumer = cons;
}

ConfirmMessage::ConfirmMessage(const VoidMessage *msg)
{
	memcpy(this,msg,sizeof(*this));
	assert(this->type == Message::Confirm);
}

//--------------------------------------------------------------------------------------------	
} // end of namespace UniSetTypes
//--------------------------------------------------------------------------------------------	

