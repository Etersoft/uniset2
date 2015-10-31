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
#include <unordered_map>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <iomanip>
#include <pthread.h>
#include <sys/types.h>
#include <sstream>
#include <chrono>

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "ObjectRepository.h"
#include "UInterface.h"
#include "UniSetObject.h"
#include "UniSetManager.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;

#define CREATE_TIMER    make_shared<PassiveCondTimer>();
// new PassiveSysTimer();

// ------------------------------------------------------------------------------------------
UniSetObject::UniSetObject():
	msgpid(0),
	regOK(false),
	active(0),
	threadcreate(false),
	myid(UniSetTypes::DefaultObjectId),
	oref(0),
	SizeOfMessageQueue(1000),
	MaxCountRemoveOfMessage(10),
	stMaxQueueMessages(0),
	stCountOfQueueFull(0)
{
	ui = make_shared<UInterface>(UniSetTypes::DefaultObjectId);

	tmr = CREATE_TIMER;
	myname = "noname";
	section = "nonameSection";
	init_object();
}
// ------------------------------------------------------------------------------------------
UniSetObject::UniSetObject( ObjectId id ):
	msgpid(0),
	regOK(false),
	active(0),
	threadcreate(true),
	myid(id),
	oref(0),
	SizeOfMessageQueue(1000),
	MaxCountRemoveOfMessage(10),
	stMaxQueueMessages(0),
	stCountOfQueueFull(0)
{
	ui = make_shared<UInterface>(id);
	tmr = CREATE_TIMER;

	if (myid >= 0)
	{
		string myfullname = ui->getNameById(id);
		myname = ORepHelpers::getShortName(myfullname);
		section = ORepHelpers::getSectionName(myfullname);
	}
	else
	{
		threadcreate = false;
		myid = UniSetTypes::DefaultObjectId;
		myname = "UnknownUniSetObject";
		section = "UnknownSection";
	}

	init_object();
}


UniSetObject::UniSetObject( const string& name, const string& section ):
	msgpid(0),
	regOK(false),
	active(0),
	threadcreate(true),
	myid(UniSetTypes::DefaultObjectId),
	oref(0),
	SizeOfMessageQueue(1000),
	MaxCountRemoveOfMessage(10),
	stMaxQueueMessages(0),
	stCountOfQueueFull(0)
{
	ui = make_shared<UInterface>(UniSetTypes::DefaultObjectId);

	/*! \warning UniverslalInterface не инициализируется идентификатором объекта */
	tmr = CREATE_TIMER;
	myname = section + "/" + name;
	myid = ui->getIdByName(myname);

	if( myid == DefaultObjectId )
	{
		uwarn << "name: my ID not found!" << endl;
		throw Exception(name + ": my ID not found!");
	}

	init_object();
	ui->initBackId(myid);
}

// ------------------------------------------------------------------------------------------
UniSetObject::~UniSetObject()
{
	try
	{
		deactivate();
	}
	catch(...) {}

	try
	{
		tmr->terminate();
	}
	catch(...) {}

	if( thr )
	{
		try
		{
			thr->stop();

			if( thr->isRunning() )
				thr->join();
		}
		catch(...) {}
	}
}
// ------------------------------------------------------------------------------------------
void UniSetObject::init_object()
{
	a_working = ATOMIC_VAR_INIT(0);
	active = ATOMIC_VAR_INIT(0);

	qmutex.setName(myname + "_qmutex");
	refmutex.setName(myname + "_refmutex");
	//    mutex_act.setName(myname + "_mutex_act");

	auto conf = uniset_conf();

	SizeOfMessageQueue = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 1000);
	MaxCountRemoveOfMessage = conf->getArgPInt("--uniset-object-maxcount-remove-message", conf->getField("MaxCountRemoveOfMessage"), SizeOfMessageQueue / 4);
	//    workingTerminateTimeout = conf->getArgPInt("--uniset-object-working-terminate-timeout",conf->getField("WorkingTerminateTimeout"),2000);

	uinfo << myname << "(init): SizeOfMessageQueue=" << SizeOfMessageQueue
		  << " MaxCountRemoveOfMessage=" << MaxCountRemoveOfMessage
		  << endl;
}
// ------------------------------------------------------------------------------------------

/*!
 *    \param om - указазтель на менджер управляющий объектом
 *    \return Возращает \a true если инициализация прошда успешно, и \a false если нет
*/
bool UniSetObject::init( const std::weak_ptr<UniSetManager>& om )
{
	uinfo << myname << ": init..." << endl;
	this->mymngr = om;
	uinfo << myname << ": init ok..." << endl;
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setID( UniSetTypes::ObjectId id )
{
	if( myid != UniSetTypes::DefaultObjectId )
		throw ObjectNameAlready("ObjectId already set(setID)");

	string myfullname = ui->getNameById(id);
	myname = ORepHelpers::getShortName(myfullname);
	section = ORepHelpers::getSectionName(myfullname);
	myid = id;
	ui->initBackId(myid);
}

// ------------------------------------------------------------------------------------------
/*!
 *    \param  vm - указатель на структуру, которая заполняется если есть сообщение
 *    \return Возвращает \a true если сообщение есть, и \a false если нет
*/
bool UniSetObject::receiveMessage( VoidMessage& vm )
{
	{
		// lock
		uniset_rwmutex_wrlock mlk(qmutex);

		if( !queueMsg.empty() )
		{
			// контроль переполнения
			if( queueMsg.size() > SizeOfMessageQueue )
			{
				ucrit << myname << "(receiveMessages): messages queue overflow!" << endl << flush;
				cleanMsgQueue(queueMsg);
				// обновляем статистику по переполнениям
				stCountOfQueueFull++;
				stMaxQueueMessages = 0;
			}

			if( !queueMsg.empty() )
			{
				vm = queueMsg.top(); // получили сообщение
				queueMsg.pop(); // удалили сообщение из очереди
				return true;
			}
		}
	} // unlock queue

	return false;
}

// ------------------------------------------------------------------------------------------
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

// ------------------------------------------------------------------------------------------
bool UniSetObject::waitMessage(VoidMessage& vm, timeout_t timeMS)
{
	if( receiveMessage(vm) )
		return true;

	tmr->wait(timeMS);
	return receiveMessage(vm);
}
// ------------------------------------------------------------------------------------------
void UniSetObject::registered()
{
	uinfo << myname << ": registration..." << endl;

	if( myid == UniSetTypes::DefaultObjectId )
	{
		uinfo << myname << "(registered): myid=DefaultObjectId \n";
		return;
	}

	auto m = mymngr.lock();

	if( !m )
	{
		uwarn << myname << "(registered): unknown my manager" << endl;
		string err(myname + ": unknown my manager");
		throw ORepFailed(err);
	}

	{
		UniSetTypes::uniset_rwmutex_rlock lock(refmutex);

		if( !oref )
		{
			ucrit << myname << "(registered): oref is NULL!..." << endl;
			return;
		}
	}

	try
	{
		for( unsigned int i = 0; i < 2; i++ )
		{
			try
			{
				ui->registered(myid, getRef(), true);
				break;
			}
			catch( ObjectNameAlready& al )
			{
				/*!
				    \warning По умолчанию объекты должны быть уникальны! Поэтому если идёт попытка повторной регистрации.
				    Мы чистим существующую ссылку и заменяем её на новую.
				    Это сделано для более надежной работы, иначе может получится, что если объект перед завершением
				    не очистил за собой ссылку(не разрегистрировался), то больше он никогда не сможет вновь зарегистрироваться.
				    Т.к. \b надёжной функции проверки "жив" ли объект пока нет...
				    (так бы можно было проверить и если "не жив", то смело заменять ссылку на новую). Но существует обратная сторона:
				    если заменяемый объект "жив" и завершит свою работу, то он может почистить за собой ссылку и это тогда наш(новый)
				    объект станет недоступен другим, а знать об этом не будет!!!
				*/
				ucrit << myname << "(registered): replace object (ObjectNameAlready)" << endl;
				regOK = true;
				unregister();
			}
		}
	}
	catch( ORepFailed )
	{
		string err(myname + ": don`t registration in object reposotory");
		throw ORepFailed(err);
	}
	catch( const Exception& ex )
	{
		uwarn << myname << "(registered):  " << ex << endl;
		string err(myname + ": don`t registration in object reposotory");
		throw ORepFailed(err);
	}

	regOK = true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::unregister()
{
	if( myid < 0 ) // || !reg )
		return;

	if( myid == UniSetTypes::DefaultObjectId )
	{
		uinfo << myname << "(unregister): myid=DefaultObjectId \n";
		regOK = false;
		return;
	}

	{
		UniSetTypes::uniset_rwmutex_rlock lock(refmutex);

		if( !oref )
		{
			uwarn << myname << "(unregister): oref NULL!" << endl;
			regOK = false;
			return;
		}
	}


	try
	{
		uinfo << myname << ": unregister " << endl;
		ui->unregister(myid);
		uinfo << myname << ": unregister ok. " << endl;
	}
	catch(...)
	{
		uwarn << myname << ": don`t registration in object repository" << endl;
	}

	regOK = false;
}
// ------------------------------------------------------------------------------------------
CORBA::Boolean UniSetObject::exist()
{
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::termWaiting()
{
	if( tmr )
		tmr->terminate();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setThreadPriority( int p )
{
	if( thr )
		thr->setPriority(p);
}
// ------------------------------------------------------------------------------------------
void UniSetObject::push( const TransportMessage& tm )
{
	{
		// lock
		uniset_rwmutex_wrlock mlk(qmutex);

		// контроль переполнения
		if( !queueMsg.empty() && queueMsg.size() > SizeOfMessageQueue )
		{
			ucrit << myname << "(push): message queue overflow!" << endl << flush;
			cleanMsgQueue(queueMsg);

			// обновляем статистику
			stCountOfQueueFull++;
			stMaxQueueMessages = 0;
		}

		VoidMessage v(tm);
		queueMsg.push(v);

		// максимальное число ( для статистики )
		if( queueMsg.size() > stMaxQueueMessages )
			stMaxQueueMessages = queueMsg.size();

	} // unlock

	termWaiting();
}
// ------------------------------------------------------------------------------------------
struct tmpConsumerInfo
{
	tmpConsumerInfo() {}

	unordered_map<UniSetTypes::KeyType, VoidMessage> smap;
	unordered_map<int, VoidMessage> tmap;
	unordered_map<int, VoidMessage> sysmap;
	std::map<CInfo, VoidMessage> cmap;
	list<VoidMessage> lstOther;
};

void UniSetObject::cleanMsgQueue( MessagesQueue& q )
{
	ucrit << myname << "(cleanMsgQueue): msg queue cleaning..." << endl << flush;
	ucrit << myname << "(cleanMsgQueue): current size of queue: " << q.size() << endl << flush;

	// проходим по всем известным нам типам(базовым)
	// ищем все совпадающие сообщения и оставляем только последние...
	VoidMessage m;
	unordered_map<UniSetTypes::ObjectId, tmpConsumerInfo> consumermap;

	//        while( receiveMessage(vm) );
	//        while нельзя использовать потому-что, из параллельного потока
	//        могут запихивать в очередь ещё сообщения.. И это цикл никогда не прервётся...

	while( !q.empty() )
	{
		m = q.top();
		q.pop();

		switch( m.type )
		{
			case Message::SensorInfo:
			{
				SensorMessage sm(&m);
				UniSetTypes::KeyType k(key(sm.id, sm.node));
				// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
				// то достаточно просто сохранять последнее сообщение для одинаковых Key
				consumermap[sm.consumer].smap[k] = m;
			}
			break;

			case Message::Timer:
			{
				TimerMessage tm(&m);
				// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
				// то достаточно просто сохранять последнее сообщение для одинаковых TimerId
				consumermap[tm.consumer].tmap[tm.id] = m;
			}
			break;

			case Message::SysCommand:
			{
				SystemMessage sm(&m);
				consumermap[sm.consumer].sysmap[sm.command] = m;
			}
			break;

			case Message::Confirm:
			{
				ConfirmMessage cm(&m);
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
				consumermap[m.consumer].lstOther.push_front(m);
				break;

		}
	}

	ucrit << myname << "(cleanMsgQueue): ******** cleanup RESULT ********" << endl;

	for( auto& c : consumermap )
	{
		ucrit << myname << "(cleanMsgQueue): CONSUMER=" << c.first << endl;
		ucrit << myname << "(cleanMsgQueue): after clean SensorMessage: " << c.second.smap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean TimerMessage: " << c.second.tmap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean SystemMessage: " << c.second.sysmap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean ConfirmMessage: " << c.second.cmap.size() << endl;
		ucrit << myname << "(cleanMsgQueue): after clean other: " << c.second.lstOther.size() << endl;

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

	ucrit << myname
		  << "(cleanMsgQueue): ******* result size of queue: "
		  << q.size()
		  << " < " << getMaxSizeOfMessageQueue() << endl;

	if( q.size() >= getMaxSizeOfMessageQueue() )
	{
		ucrit << myname << "(cleanMsgQueue): clean failed. size > " << q.size() << endl;
		ucrit << myname << "(cleanMsgQueue): remove " << getMaxCountRemoveOfMessage() << " old messages " << endl;

		for( unsigned int i = 0; i < getMaxCountRemoveOfMessage(); i++ )
		{
			q.top();
			q.pop();

			if( q.empty() )
				break;
		}

		ucrit << myname << "(cleanMsgQueue): result size=" << q.size() << endl;
	}
}
// ------------------------------------------------------------------------------------------
unsigned int UniSetObject::countMessages()
{
	{
		// lock
		uniset_rwmutex_rlock mlk(qmutex);
		return queueMsg.size();
	}
}
// ------------------------------------------------------------------------------------------
void UniSetObject::sigterm( int signo )
{
}
// ------------------------------------------------------------------------------------------
bool UniSetObject::deactivate()
{
	if( !isActive() )
	{
		try
		{
			deactivateObject();
		}
		catch(...) {}

		return true;
	}

	setActive(false); // завершаем поток обработки сообщений

	if( tmr )
		tmr->terminate();

	if( thr )
	{
		std::unique_lock<std::mutex> lk(m_working);

		//        cv_working.wait_for(lk, std::chrono::milliseconds(workingTerminateTimeout), [&](){ return (a_working == false); } );
		if( a_working )
			cv_working.wait(lk);

		if( a_working )
			thr->stop();
	}

	// Очищаем очередь
	{
		// lock
		uniset_rwmutex_wrlock mlk(qmutex);

		while( !queueMsg.empty() )
			queueMsg.pop();
	}

	try
	{
		uinfo << myname << "(deactivate): ..." << endl;

		auto m = mymngr.lock();

		if( m )
		{
			PortableServer::POA_var poamngr = m->getPOA();

			if( !PortableServer::POA_Helper::is_nil(poamngr) )
			{
				try
				{
					deactivateObject();
				}
				catch(...) {}

				unregister();
				PortableServer::ObjectId_var oid = poamngr->servant_to_id(static_cast<PortableServer::ServantBase*>(this));
				poamngr->deactivate_object(oid);
				uinfo << myname << "(disacivate): finished..." << endl;
				return true;
			}
		}

		uwarn << myname << "(deactivate): manager already destroyed.." << endl;
	}
	catch( const CORBA::TRANSIENT& )
	{
		uwarn << myname << "(deactivate): isExist: нет связи..." << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		uwarn << myname << "(deactivate): " << "поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& ex )
	{
		uwarn << myname << "(deactivate): " << "поймали CORBA::Exception." << endl;
	}
	catch( const Exception& ex )
	{
		uwarn << myname << "(deactivate): " << ex << endl;
	}
	catch(...)
	{
		uwarn << myname << "(deactivate): " << " catch ..." << endl;
	}

	return false;
}

// ------------------------------------------------------------------------------------------
bool UniSetObject::activate()
{
	uinfo << myname << ": activate..." << endl;

	auto m = mymngr.lock();

	if( !m )
	{
		ostringstream err;
		err << myname << "(activate): mymngr=NULL!!! activate failure...";
		ucrit << err.str() << endl;
		throw SystemError(err.str());
	}

	PortableServer::POA_var poa = m->getPOA();

	if( poa == NULL || CORBA::is_nil(poa) )
	{
		string err(myname + ": не задан менеджер");
		throw ORepFailed(err);
	}

	if( uniset_conf()->isTransientIOR() )
	{
		// activate witch generate id
		poa->activate_object(static_cast<PortableServer::ServantBase*>(this));
	}
	else
	{
		// А если myid==UniSetTypes::DefaultObjectId
		// то myname = noname. ВСЕГДА!
		if( myid == UniSetTypes::DefaultObjectId )
		{
			ucrit << myname << "(activate): Не задан ID!!! activate failure..." << endl;
			// вызываем на случай если она переопределена в дочерних классах
			// Например в UniSetManager, если здесь не вызвать, то не будут инициализированы подчинённые объекты.
			// (см. UniSetManager::activateObject)
			activateObject();
			return false;
		}

		// Always use the same object id.
		PortableServer::ObjectId_var oid =
			PortableServer::string_to_ObjectId(myname.c_str());

		//        cerr << myname << "(activate): " << _refcount_value() << endl;

		// Activate object...
		poa->activate_object_with_id(oid, this);
	}

	{
		UniSetTypes::uniset_rwmutex_wrlock lock(refmutex);
		oref = poa->servant_to_reference(static_cast<PortableServer::ServantBase*>(this) );
	}

	registered();
	// Запускаем поток обработки сообщений
	setActive(true);

	if( myid != UniSetTypes::DefaultObjectId && threadcreate )
	{
		thr = make_shared< ThreadCreator<UniSetObject> >(this, &UniSetObject::work);
		thr->setCancel(ost::Thread::cancelDeferred);

		std::unique_lock<std::mutex> locker(m_working);
		a_working = true;
		thr->start();
	}
	else
	{
		uinfo << myname << ": ?? не задан ObjectId...("
			  << "myid=" << myid << " threadcreate=" << threadcreate
			  << ")" << endl;
		thread(false);
	}

	activateObject();
	uinfo << myname << ": activate ok." << endl;
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::work()
{
	uinfo << myname << ": thread processing messages running..." << endl;

	if( thr )
		msgpid = thr->getTID();

	{
		std::unique_lock<std::mutex> locker(m_working);
		a_working = true;
	}

	while( isActive() )
		callback();

	uinfo << myname << ": thread processing messages stopped..." << endl;

	{
		std::unique_lock<std::mutex> locker(m_working);
		a_working = false;
	}

	cv_working.notify_all();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::callback()
{
	try
	{
		if( waitMessage(msg) )
			processingMessage(&msg);
	}
	catch( const Exception& ex )
	{
		ucrit << ex << endl;
	}
	catch( const std::exception& ex )
	{
		ucrit << ex.what() << endl;
	}
}
// ------------------------------------------------------------------------------------------
void UniSetObject::processingMessage( UniSetTypes::VoidMessage* msg )
{
	try
	{
		switch( msg->type )
		{
			case Message::SensorInfo:
				sensorInfo( reinterpret_cast<SensorMessage*>(msg) );
				break;

			case Message::Timer:
				timerInfo( reinterpret_cast<TimerMessage*>(msg) );
				break;

			case Message::SysCommand:
				sysCommand( reinterpret_cast<SystemMessage*>(msg) );
				break;

			default:
				break;
		}
	}
	catch( const Exception& ex )
	{
		ucrit  << myname << "(processingMessage): " << ex << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		ucrit << myname << "(processingMessage): CORBA::SystemException: " << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& ex )
	{
		uwarn << myname << "(processingMessage): CORBA::Exception: " << ex._name() << endl;
	}
	catch( omniORB::fatalException& fe )
	{
		if( ulog()->is_crit() )
		{
			ulog()->crit() << myname << "(processingMessage): Caught omniORB::fatalException:" << endl;
			ulog()->crit() << myname << "(processingMessage): file: " << fe.file()
						   << " line: " << fe.line()
						   << " mesg: " << fe.errmsg() << endl;
		}
	}
	catch( const std::exception& ex )
	{
		ucrit << myname << "(processingMessage): " << ex.what() << endl;
	}

	/*
	    catch( ... )
	    {
	        std::exception_ptr p = std::current_exception();
	        ucrit <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
	    }
	*/
}
// ------------------------------------------------------------------------------------------

UniSetTypes::SimpleInfo* UniSetObject::getInfo()
{
	ostringstream info;
	info.setf(ios::left, ios::adjustfield);
	info << "(" << myid << ")" << setw(40) << myname << "\n==================================================\n";
	info << "tid=" << setw(10);

	if( threadcreate )
	{
		if(thr)
		{
			msgpid = thr->getId();    // заодно(на всякий) обновим и внутреннюю информацию
			info << msgpid;
		}
		else
			info << "не запущен";
	}
	else
		info << "откл.";

	info << "\tcount=" << countMessages();
	info << "\tmaxMsg=" << stMaxQueueMessages;
	info << "\tqFull(" << SizeOfMessageQueue << ")=" << stCountOfQueueFull;
	//    info << "\n";

	SimpleInfo* res = new SimpleInfo();
	res->info =  info.str().c_str(); // CORBA::string_dup(info.str().c_str());
	res->id   =  myid;

	return res; // ._retn();
}
// ------------------------------------------------------------------------------------------
ostream& operator<<(ostream& os, UniSetObject& obj )
{
	SimpleInfo_var si = obj.getInfo();
	return os << si->info;
}
// ------------------------------------------------------------------------------------------

bool UniSetObject::PriorVMsgCompare::operator()(const UniSetTypes::VoidMessage& lhs,
		const UniSetTypes::VoidMessage& rhs) const
{
	if( lhs.priority == rhs.priority )
	{
		if( lhs.tm.tv_sec == rhs.tm.tv_sec )
			return lhs.tm.tv_usec >= rhs.tm.tv_usec;

		return lhs.tm.tv_sec >= rhs.tm.tv_sec;
	}

	return lhs.priority < rhs.priority;
}
// ------------------------------------------------------------------------------------------
#undef CREATE_TIMER
