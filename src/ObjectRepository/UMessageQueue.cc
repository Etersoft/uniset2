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
// -------------------------------------------------------------------------
#include <unordered_map>
#include <map>
#include "MessageType.h"
#include "UMessageQueue.h"
//--------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
//--------------------------------------------------------------------------
UMessageQueue::UMessageQueue( size_t qsize ):
	SizeOfMessageQueue(qsize)
{
	wQ = &mq1;
	rQ = &mq2;
}
//---------------------------------------------------------------------------
void UMessageQueue::push( const UniSetTypes::TransportMessage& tm )
{
	{
		// lock
		uniset_rwmutex_wrlock mlk(qmutex);

		// контроль переполнения
		if( !wQ->empty() && wQ->size() > SizeOfMessageQueue )
		{
			// ucrit << myname << "(push): message queue overflow!" << endl << flush;
			cleanMsgQueue(*wQ);

			// обновляем статистику
			stCountOfQueueFull++;
			stMaxQueueMessages = 0;
		}

		auto v = make_shared<VoidMessage>(tm);
		wQ->push(v);

		// максимальное число ( для статистики )
		if( wQ->size() > stMaxQueueMessages )
			stMaxQueueMessages = wQ->size();

	} // unlock
}
//---------------------------------------------------------------------------
VoidMessagePtr UMessageQueue::top()
{
	// здесь работаем со своей очередью без блокировки
	if( !rQ->empty() )
	{
		auto m = rQ->top(); // получили сообщение
		rQ->pop(); // удалили сообщение из очереди
		return m;
	}

	// Если своя очередь пуста
	// то смотрим вторую
	{
		// lock
		uniset_rwmutex_wrlock mlk(qmutex);

		if( !wQ->empty() )
		{
			// контроль переполнения
			if( wQ->size() > SizeOfMessageQueue )
			{
//				ucrit << myname << "(receiveMessages): messages queue overflow!" << endl << flush;
				cleanMsgQueue(*wQ);
				// обновляем статистику по переполнениям
				stCountOfQueueFull++;
				stMaxQueueMessages = 0;
			}

			if( !wQ->empty() )
			{
				auto m = wQ->top(); // получили сообщение
				wQ->pop(); // удалили сообщение из очереди

				// меняем очереди местами
				std::swap(rQ,wQ);
				return m;
			}
		}
	} // unlock queue

	return nullptr;
}
//---------------------------------------------------------------------------
size_t UMessageQueue::size()
{
	uniset_rwmutex_wrlock mlk(qmutex);
	return (wQ->size() + rQ->size());
}
//---------------------------------------------------------------------------
void UMessageQueue::setMaxSizeOfMessageQueue(size_t s)
{
	SizeOfMessageQueue = s;
}

size_t UMessageQueue::getMaxSizeOfMessageQueue()
{
	return SizeOfMessageQueue;
}
//---------------------------------------------------------------------------
void UMessageQueue::setMaxCountRemoveOfMessage( size_t m )
{
	MaxCountRemoveOfMessage = m;
}

size_t UMessageQueue::getMaxCountRemoveOfMessage()
{
	return MaxCountRemoveOfMessage;
}
//---------------------------------------------------------------------------
// структура определяющая минимальное количество полей
// по которым можно судить о схожести сообщений
// используется локально и только в функции очистки очереди сообщений
struct MsgInfo
{
	MsgInfo():
		type(Message::Unused),
		id(DefaultObjectId),
		node(DefaultObjectId)
	{
		//        struct timezone tz;
		tm.tv_sec = 0;
		tm.tv_usec = 0;
		//        gettimeofday(&tm,&tz);
	}

	int type;
	ObjectId id;        // от кого
	struct timeval tm;    // время
	ObjectId node;        // откуда

	inline bool operator < ( const MsgInfo& mi ) const
	{
		if( type != mi.type )
			return type < mi.type;

		if( id != mi.id )
			return id < mi.id;

		if( node != mi.node )
			return node < mi.node;

		if( tm.tv_sec != mi.tm.tv_sec )
			return tm.tv_sec < mi.tm.tv_sec;

		return tm.tv_usec < mi.tm.tv_usec;
	}

};
//---------------------------------------------------------------------------
// структура определяющая минимальное количество полей
// по которым можно судить о схожести сообщений
// используется локально и только в функции очистки очереди сообщений
struct CInfo
{
	CInfo():
		sensor_id(DefaultObjectId),
		value(0),
		time(0),
		time_usec(0),
		confirm(0)
	{
	}

	explicit CInfo( ConfirmMessage& cm ):
		sensor_id(cm.sensor_id),
		value(cm.value),
		time(cm.time),
		time_usec(cm.time_usec),
		confirm(cm.confirm)
	{}

	long sensor_id;   /* ID датчика */
	double value;     /* значение датчика */
	time_t time;      /* время, когда датчик получил сигнал */
	time_t time_usec; /* время в микросекундах */
	time_t confirm;   /* время, когда произошло квитирование */

	inline bool operator < ( const CInfo& mi ) const
	{
		if( sensor_id != mi.sensor_id )
			return sensor_id < mi.sensor_id;

		if( value != mi.value )
			return value < mi.value;

		if( time != mi.time )
			return time < mi.time;

		return time_usec < mi.time_usec;
	}
};
//---------------------------------------------------------------------------
struct tmpConsumerInfo
{
	tmpConsumerInfo() {}

	unordered_map<UniSetTypes::KeyType, VoidMessagePtr> smap;
	unordered_map<int, VoidMessagePtr> tmap;
	unordered_map<int, VoidMessagePtr> sysmap;
	std::map<CInfo, VoidMessagePtr> cmap;
	list<VoidMessagePtr> lstOther;
};

void UMessageQueue::cleanMsgQueue( UMessageQueue::MQueue& q )
{
#if 0
	ucrit << myname << "(cleanMsgQueue): msg queue cleaning..." << endl << flush;
	ucrit << myname << "(cleanMsgQueue): current size of queue: " << q.size() << endl << flush;
#endif
	// проходим по всем известным нам типам(базовым)
	// ищем все совпадающие сообщения и оставляем только последние...
	VoidMessage m;
	unordered_map<UniSetTypes::ObjectId, tmpConsumerInfo> consumermap;

	while( !q.empty() )
	{
		auto m = q.top();
		q.pop();

		switch( m->type )
		{
			case Message::SensorInfo:
			{
				SensorMessage sm(m.get());
				UniSetTypes::KeyType k(key(sm.id, sm.node));
				// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
				// то достаточно просто сохранять последнее сообщение для одинаковых Key
				consumermap[sm.consumer].smap[k] = m;
			}
			break;

			case Message::Timer:
			{
				TimerMessage tm(m.get());
				// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
				// то достаточно просто сохранять последнее сообщение для одинаковых TimerId
				consumermap[tm.consumer].tmap[tm.id] = m;
			}
			break;

			case Message::SysCommand:
			{
				SystemMessage sm(m.get());
				consumermap[sm.consumer].sysmap[sm.command] = m;
			}
			break;

			case Message::Confirm:
			{
				ConfirmMessage cm(m.get());
				CInfo ci(cm);
				// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
				// то достаточно просто сохранять последнее сообщение для одинаковых MsgInfo
				consumermap[cm.consumer].cmap[ci] = m;
			}
			break;

			case Message::Unused:
				// просто выкидываем (игнорируем)
				break;

			default:
				// сразу помещаем в очередь
				consumermap[m->consumer].lstOther.push_front(m);
				break;

		}
	}

//	ucrit << myname << "(cleanMsgQueue): ******** cleanup RESULT ********" << endl;

	for( auto& c : consumermap )
	{
#if 0
		ucrit << myname << "(cleanMsgQueue): CONSUMER=" << c.first << endl;
		ucrit << myname << "(cleanMsgQueue): after clean SensorMessage: " << c.second.smap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean TimerMessage: " << c.second.tmap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean SystemMessage: " << c.second.sysmap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean ConfirmMessage: " << c.second.cmap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean other: " << c.second.lstOther.size() << endl;
#endif

		// теперь ОСТАВШИЕСЯ запихиваем обратно в очередь...
		for( auto& v : c.second.smap )
			q.push(v.second);

		for( auto& v : c.second.tmap )
			q.push(v.second);

		for( auto& v : c.second.sysmap )
			q.push(v.second);

		for( auto& v : c.second.cmap )
			q.push(v.second);

		for( auto& v : c.second.lstOther )
			q.push(v);
	}

//	ucrit << myname
//		  << "(cleanMsgQueue): ******* result size of queue: "
//		  << q.size()
//		  << " < " << getMaxSizeOfMessageQueue() << endl;

	if( q.size() >= SizeOfMessageQueue )
	{
//		ucrit << myname << "(cleanMsgQueue): clean failed. size > " << q.size() << endl;
//		ucrit << myname << "(cleanMsgQueue): remove " << getMaxCountRemoveOfMessage() << " old messages " << endl;

		for( unsigned int i = 0; i < MaxCountRemoveOfMessage; i++ )
		{
			q.top();
			q.pop();

			if( q.empty() )
				break;
		}

//		ucrit << myname << "(cleanMsgQueue): result size=" << q.size() << endl;
	}
}
//---------------------------------------------------------------------------
bool UMessageQueue::VoidMessageCompare::operator()(const VoidMessagePtr& lhs, const VoidMessagePtr& rhs) const
{
	if( lhs->priority == rhs->priority )
	{
		if( lhs->tm.tv_sec == rhs->tm.tv_sec )
			return lhs->tm.tv_usec >= rhs->tm.tv_usec;

		return lhs->tm.tv_sec >= rhs->tm.tv_sec;
	}

	return lhs->priority < rhs->priority;
}
//---------------------------------------------------------------------------
