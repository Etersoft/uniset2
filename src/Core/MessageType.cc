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

namespace uniset
{
    //--------------------------------------------------------------------------------------------
    std::string strTypeOfMessage( int type )
    {
        if( type == Message::SensorInfo )
            return "SensorInfo";

        if( type == Message::SysCommand )
            return "SysCommand";

        if( type == Message::Confirm )
            return "Confirm";

        if( type == Message::Timer )
            return "Timer";

        if( type == Message::TextMessage )
            return "TextMessage";

        if( type == Message::Unused )
            return "Unused";

        return "Unkown";
    }

    std::ostream& operator<<( std::ostream& os, const Message::TypeOfMessage& t )
    {
        return os << strTypeOfMessage(t);
    }
    //--------------------------------------------------------------------------------------------
    Message::Message() noexcept:
        type(Unused), priority(Medium),
        node( uniset::uniset_conf() ? uniset::uniset_conf()->getLocalNode() : DefaultObjectId ),
        supplier(DefaultObjectId),
        consumer(DefaultObjectId)
    {
        tm = uniset::now_to_timespec();
    }

    //--------------------------------------------------------------------------------------------
    VoidMessage::VoidMessage( const TransportMessage& tm ) noexcept:
        Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
    {
        assert(sizeof(VoidMessage) >= sizeof(uniset::RawDataOfTransportMessage));
        memcpy(this, &tm.data, sizeof(tm.data));
        consumer = tm.consumer;
    }

    VoidMessage::VoidMessage() noexcept
    {

        assert(sizeof(VoidMessage) >= sizeof(uniset::RawDataOfTransportMessage));
    }

    //--------------------------------------------------------------------------------------------
    SensorMessage::SensorMessage() noexcept:
        id(DefaultObjectId),
        value(0),
        undefined(false),
        sensor_type(UniversalIO::DI),
        threshold(false),
        tid(uniset::DefaultThresholdId)
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
                                 UniversalIO::IOType st, ObjectId consumer) noexcept:
        id(id),
        value(value),
        undefined(false),
        sensor_type(st),
        ci(ci),
        threshold(false),
        tid(uniset::DefaultThresholdId)
    {
        type            = Message::SensorInfo;
        this->priority     = priority;
        this->consumer     = consumer;
        sm_tv = tm;
    }

    SensorMessage::SensorMessage( int dummy ) noexcept:
        Message(1), // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
        ci(IOController_i::CalibrateInfo())
    {
        type    = Message::SensorInfo;
    }

    SensorMessage::SensorMessage(const VoidMessage* msg) noexcept:
        Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
    {
        memcpy(this, msg, sizeof(*this));
        assert(this->type == Message::SensorInfo);
    }
    //--------------------------------------------------------------------------------------------
    SystemMessage::SystemMessage() noexcept:
        command(SystemMessage::Unknown)
    {
        memset(data, 0, sizeof(data));
        type = Message::SysCommand;
    }

    SystemMessage::SystemMessage(Command command, Priority priority, ObjectId consumer) noexcept:
        command(command)
    {
        type = Message::SysCommand;
        this->priority = priority;
        this->consumer = consumer;
        memset(data, 0, sizeof(data));
    }

    SystemMessage::SystemMessage(const VoidMessage* msg) noexcept:
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
        id(uniset::DefaultTimerId)
    {
        type = Message::Timer;
    }

    TimerMessage::TimerMessage(uniset::TimerId id, Priority prior, ObjectId cons):
        id(id)
    {
        type = Message::Timer;
        this->consumer = cons;
    }

    TimerMessage::TimerMessage(uniset::TimerId _id, uniset::timeout_t _interval_msec, Priority prior, ObjectId cons):
            id(_id),
            interval_msec(_interval_msec)
    {
        type = Message::Timer;
        this->consumer = cons;
    }

    TimerMessage::TimerMessage(const VoidMessage* msg) noexcept:
        Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
    {
        memcpy(this, msg, sizeof(*this));
        assert(this->type == Message::Timer);
    }
    //--------------------------------------------------------------------------------------------
    ConfirmMessage::ConfirmMessage( const VoidMessage* msg ) noexcept:
        Message(1) // вызываем dummy-конструктор, который не инициализирует данные (оптимизация)
    {
        memcpy(this, msg, sizeof(*this));
        assert(this->type == Message::Confirm);
    }
    //--------------------------------------------------------------------------------------------
    ConfirmMessage::ConfirmMessage(uniset::ObjectId in_sensor_id,
                                   const double& in_sensor_value,
                                   const timespec& in_sensor_time,
                                   const timespec& in_confirm_time,
                                   Priority in_priority ) noexcept:
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
    TextMessage::TextMessage( const VoidMessage* vmsg ) noexcept
        : VoidMessage(1) // dummy constructor
    {
        assert(vmsg->type == Message::TextMessage);

        auto m = static_cast<const TextMessage*>(vmsg);

        if( m )
        {
            type = m->type;
            priority = m->priority;
            node = m->node;
            tm = m->tm;
            consumer = m->consumer;
            supplier = m->supplier;
            mtype = m->mtype;
            txt = m->txt;
        }
    }
    //--------------------------------------------------------------------------------------------
    TextMessage::TextMessage() noexcept
    {
        type = Message::TextMessage;
    }

    TextMessage::TextMessage(const char* msg,
                             int _mtype,
                             const uniset::Timespec& tm,
                             const ::uniset::ProducerInfo& pi,
                             Priority prior,
                             ObjectId cons) noexcept
    {
        type = Message::TextMessage;
        this->node = pi.node;
        this->supplier = pi.id;
        this->priority = prior;
        this->consumer = cons;
        this->tm.tv_sec = tm.sec;
        this->tm.tv_nsec = tm.nsec;
        this->txt = std::string(msg);
        this->mtype = _mtype;
    }
    //--------------------------------------------------------------------------------------------
    std::shared_ptr<VoidMessage> TextMessage::toLocalVoidMessage() const
    {
        uniset::ProducerInfo pi;
        pi.id = supplier;
        pi.node = node;

        uniset::Timespec ts;
        ts.sec = tm.tv_sec;
        ts.nsec = tm.tv_nsec;

        auto tmsg = std::make_shared<TextMessage>(txt.c_str(), mtype, ts, pi, priority, consumer);
        return std::static_pointer_cast<VoidMessage>(tmsg);
    }
    //--------------------------------------------------------------------------------------------
} // end of namespace uniset
//--------------------------------------------------------------------------------------------
