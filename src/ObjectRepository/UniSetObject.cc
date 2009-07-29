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
 *  \date   $Date: 2007/11/29 22:19:54 $
 *  \version $Id: UniSetObject.cc,v 1.27 2007/11/29 22:19:54 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#include <unistd.h>
#include <signal.h>
#include <iomanip>
#include <pthread.h>
#include <sys/types.h>
#include <sstream>

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "ObjectRepository.h"
#include "UniversalInterface.h"
#include "UniSetObject.h"
#include "ObjectsManager.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;

#define CREATE_TIMER	new ThrPassiveTimer(); 	
// new PassiveSysTimer();

// ------------------------------------------------------------------------------------------
UniSetObject::UniSetObject():
ui(UniSetTypes::DefaultObjectId),
mymngr(NULL),
msgpid(0),
reg(false),
active(false),
threadcreate(false),
tmr(NULL),
myid(UniSetTypes::DefaultObjectId),
oref(0),
thr(NULL),
SizeOfMessageQueue(1000),
MaxCountRemoveOfMessage(10),
stMaxQueueMessages(0),
stCountOfQueueFull(0)
{
	tmr = CREATE_TIMER;
	myname = "noname";
	section = "nonameSection";
	
	SizeOfMessageQueue = atoi(conf->getField("SizeOfMessageQueue").c_str());
	if( SizeOfMessageQueue <= 0 )
		SizeOfMessageQueue = 1000;
	
	MaxCountRemoveOfMessage = atoi(conf->getField("MaxCountRemoveOfMessage").c_str());
	if( MaxCountRemoveOfMessage <= 0 )
		MaxCountRemoveOfMessage = SizeOfMessageQueue / 4;
	if( MaxCountRemoveOfMessage <= 0 )
		MaxCountRemoveOfMessage = 10;
	recvMutexTimeout = atoi(conf->getField("RecvMutexTimeout").c_str());
	if( recvMutexTimeout <= 0 )
		recvMutexTimeout = 10000;

	pushMutexTimeout = atoi(conf->getField("PushMutexTimeout").c_str());
	if( pushMutexTimeout <= 0 )
		pushMutexTimeout = 9000;
}
// ------------------------------------------------------------------------------------------
UniSetObject::UniSetObject( ObjectId id ):
ui(id),
mymngr(NULL),
msgpid(0),
reg(false),
active(false),
threadcreate(true),
tmr(NULL),
myid(id),
oref(0),
thr(NULL),
SizeOfMessageQueue(1000),
MaxCountRemoveOfMessage(10),
stMaxQueueMessages(0),
stCountOfQueueFull(0)
{
	SizeOfMessageQueue = atoi(conf->getField("SizeOfMessageQueue").c_str());
	if( SizeOfMessageQueue <= 0 )
		SizeOfMessageQueue = 1000;
	
	MaxCountRemoveOfMessage = atoi(conf->getField("MaxCountRemoveOfMessage").c_str());
	if( MaxCountRemoveOfMessage <= 0 )
		MaxCountRemoveOfMessage = SizeOfMessageQueue / 4;
	if( MaxCountRemoveOfMessage <= 0 )
		MaxCountRemoveOfMessage = 10;

	tmr = CREATE_TIMER;
	if (myid >=0)
	{
		string myfullname = ui.getNameById(id);
		myname = ORepHelpers::getShortName(myfullname.c_str());
		section = ORepHelpers::getSectionName(myfullname.c_str());
	}
	else
	{
		threadcreate = false;
		myid=UniSetTypes::DefaultObjectId;
		myname = "noname";
		section = "nonameSection";
	}
	recvMutexTimeout = atoi(conf->getField("RecvMutexTimeout").c_str());
	if( recvMutexTimeout <= 0 )
		recvMutexTimeout = 10000;

	pushMutexTimeout = atoi(conf->getField("PushMutexTimeout").c_str());
	if( pushMutexTimeout <= 0 )
		pushMutexTimeout = 9000;

}


UniSetObject::UniSetObject(const string name, const string section):
ui(UniSetTypes::DefaultObjectId),
mymngr(NULL),
msgpid(0),
reg(false),
active(false),
threadcreate(true),
tmr(NULL),
myid(UniSetTypes::DefaultObjectId),
oref(0),
thr(NULL),
SizeOfMessageQueue(1000),
MaxCountRemoveOfMessage(10),
stMaxQueueMessages(0),
stCountOfQueueFull(0)
{
	/*! \warning UniverslalInterface не инициализируется идентификатором объекта */
	tmr = CREATE_TIMER;
	myname = section+"/"+name;
	myid = ui.getIdByName(myname.c_str());
	if( myid == DefaultObjectId )
	{
		unideb[Debug::WARN] << "name: НЕ НАЙДЕН ИДЕНТИФИКАТОР В ObjectsMap!!!" << endl;
		throw Exception(name+": НЕ НАЙДЕН ИДЕНТИФИКАТОР В ObjectsMap!!!");
	}
	SizeOfMessageQueue = atoi(conf->getField("SizeOfMessageQueue").c_str());
	if( SizeOfMessageQueue <= 0 )
		SizeOfMessageQueue = 1000;
	
	MaxCountRemoveOfMessage = atoi(conf->getField("MaxCountRemoveOfMessage").c_str());
	if( MaxCountRemoveOfMessage <= 0 )
		MaxCountRemoveOfMessage = SizeOfMessageQueue / 4;
	if( MaxCountRemoveOfMessage <= 0 )
		MaxCountRemoveOfMessage = 10;

	recvMutexTimeout = atoi(conf->getField("RecvMutexTimeout").c_str());
	if( recvMutexTimeout <= 0 )
		recvMutexTimeout = 10000;

	pushMutexTimeout = atoi(conf->getField("PushMutexTimeout").c_str());
	if( pushMutexTimeout <= 0 )
		pushMutexTimeout = 9000;

	ui.initBackId(myid);
}

// ------------------------------------------------------------------------------------------
UniSetObject::~UniSetObject() 
{
	disactivate();
	delete tmr;
	if(thr)
		delete thr;
}
// ------------------------------------------------------------------------------------------
/*!
 *	\param om - указазтель на менджер управляющий объектом
 *	\return Возращает \a true если инициализация прошда успешно, и \a false если нет
*/
bool UniSetObject::init( ObjectsManager* om )
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": инициализация ..." << endl;
	this->mymngr = om;
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": ok..." << endl;
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setID( UniSetTypes::ObjectId id )
{
	if( myid!=UniSetTypes::DefaultObjectId )
		throw ObjectNameAlready("ObjectId уже задан (setID)");

	string myfullname = ui.getNameById(id);
	myname = ORepHelpers::getShortName(myfullname.c_str()); 
	section = ORepHelpers::getSectionName(myfullname.c_str());
	myid = id;
	ui.initBackId(myid);
}

// ------------------------------------------------------------------------------------------
/*!
 *	\param VoidMessage msg - указатель на структуру, которая заполняется если есть сообщение
 *	\return Возращает \a true если сообщение есть, и \a false если нет
*/
bool UniSetObject::receiveMessage( VoidMessage& vm )
{
	{	// lock
		uniset_mutex_lock mlk(qmutex, recvMutexTimeout);
			
		if( !queueMsg.empty() )
		{
			// контроль переполнения
			if( queueMsg.size() > SizeOfMessageQueue ) 
			{
				unideb[Debug::CRIT] << myname <<": переполнение очереди сообщений!" << endl << flush;
				cleanMsgQueue(queueMsg);
				// обновляем статистику по переполнениям
				stCountOfQueueFull++;
				stMaxQueueMessages=0;	
			}

			if( !queueMsg.empty() )
			{
				vm = queueMsg.top(); // получили сообщение
//				Проверка на последовательное вынимание			
//				cout << myname << ": receive message....tm=" << vm.time << " msec=" << vm.time_msec << "\tprior="<< vm.priority << endl;
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
	acode(DefaultMessageCode),
	ccode(DefaultMessageCode),
	ch(0),
	node(DefaultObjectId)
	{
//		struct timezone tz;
		tm.tv_sec = 0;
		tm.tv_usec = 0;
//		gettimeofday(&tm,&tz);
	}

	MsgInfo( AlarmMessage& am ):
	type(am.type),
	id(am.id),
	acode(am.alarmcode),
	ccode(am.causecode),
	ch(am.character),
	tm(am.tm),
	node(am.node)
	{}

	MsgInfo( InfoMessage& am ):
	type(am.type),
	id(am.id),
	acode(am.infocode),
	ccode(0),
	ch(am.character),
	tm(am.tm),
	node(am.node)
	{}

	MsgInfo( ConfirmMessage& am ):
	type(am.orig_type),
	id(am.orig_id),
	acode(am.code),
	ccode(am.orig_cause),
	ch(0),
	tm(am.orig_tm),
	node(am.orig_node)
	{}

	int type;
	ObjectId id;		// от кого
	MessageCode acode;	// код сообщения
	MessageCode ccode;	// код причины
	int ch;				// характер
	struct timeval tm;	// время
	ObjectId node;		// откуда

   	inline bool operator < ( const MsgInfo& mi ) const
	{
		if( type != mi.type )
			return type < mi.type; 

		if( id != mi.id )
			return id < mi.id;

		if( node != mi.node )
			return node < mi.node; 

		if( acode != mi.acode )
			return acode < mi.acode;

		if( ch != mi.ch )
			return ch < mi.ch;

		if( tm.tv_sec != mi.tm.tv_sec )
			return tm.tv_sec < mi.tm.tv_sec;

		return tm.tv_usec < mi.tm.tv_usec;
	}	
	
};

// ------------------------------------------------------------------------------------------
bool UniSetObject::waitMessage(VoidMessage& vm, int timeMS)
{
	if( receiveMessage(vm) )
		return true;
	tmr->wait(timeMS);
	return receiveMessage(vm);
}
// ------------------------------------------------------------------------------------------
void UniSetObject::registered()
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": регистрация ..." << endl;

	if( myid == UniSetTypes::DefaultObjectId )
	{
		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << myname << "(registered): myid=DefaultObjectId \n";
		return;
	}

	if( !mymngr )
	{
		unideb[Debug::WARN] << myname << "(registered): не задан менеджер" << endl;
		string err(myname+": не задан менеджер");
		throw ORepFailed(err.c_str());
	}

	if( !oref )
	{
		unideb[Debug::CRIT] << myname << "(registered): нулевая ссылка oref!!!..." << endl;
		return;
	}

	try
	{
		for( int i=0; i<2; i++ )
		{		
			try
			{
				ui.registered(myid, getRef(),true);
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
				unideb[Debug::CRIT] << myname << "(registered): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
				reg = true;
				unregister();
//				unideb[Debug::CRIT] << myname << "(registered): не смог зарегестрироваться в репозитории объектов (ObjectNameAlready)" << endl;
//				throw al;
			}
		}
	}
	catch( ORepFailed )
	{
		string err(myname+": не смог зарегестрироваться в репозитории объектов");
		throw ORepFailed(err.c_str());
	}
	catch(Exception& ex)
	{
		unideb[Debug::WARN] << myname << "(registered):  " << ex << endl;
		string err(myname+": не смог зарегестрироваться в репозитории объектов");
		throw ORepFailed(err.c_str());
	}
	reg = true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::unregister()
{
	if( myid<0 ) // || !reg )
		return;

	if( myid == UniSetTypes::DefaultObjectId )
	{
		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << myname << "(unregister): myid=DefaultObjectId \n";
		reg = false;
		return;
	}

	if( !oref )
	{
		unideb[Debug::WARN] << myname << "(unregister): нулевая ссылка oref!!!..." << endl;
		reg = false;
		return;
	}


	try
	{
		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << myname << ": unregister "<< endl;

		ui.unregister(myid);

		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << myname << ": unregister ok. "<< endl;
	}
	catch(...)
	{
		unideb[Debug::WARN] << myname << ": не смог разрегестрироваться в репозитории объектов" << endl;
	}
	
	reg = false;
}
// ------------------------------------------------------------------------------------------
CORBA::Boolean UniSetObject::exist()
{
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::termWaiting()
{
    if( tmr!=NULL )
		tmr->terminate();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setRecvMutexTimeout( unsigned long msec )
{
	recvMutexTimeout = msec;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setPushMutexTimeout( unsigned long msec )
{
	pushMutexTimeout = msec;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::push(const TransportMessage& tm)
{
	{ // lock
		uniset_mutex_lock mlk(qmutex,pushMutexTimeout);
		// контроль переполнения
		if( !queueMsg.empty() && queueMsg.size()>SizeOfMessageQueue )
		{
			
			unideb[Debug::CRIT] << myname <<": переполнение очереди сообщений!\n";
			cleanMsgQueue(queueMsg);

			// обновляем статистику
			stCountOfQueueFull++;
			stMaxQueueMessages=0;	
		}

		queueMsg.push(VoidMessage(tm));
		
		// максимальное число ( для статистики )
		if( queueMsg.size() > stMaxQueueMessages )
			stMaxQueueMessages = queueMsg.size();

	} // unlock

	termWaiting();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::cleanMsgQueue( MessagesQueue& q )
{
	unideb[Debug::CRIT] << myname << "(cleanMsgQueue): ОЧИСТКА ОЧЕРЕДИ СООБЩЕНИЙ...\n";

	// проходим по всем известным нам типам(базовым)
	// ищем все совпадающие сообщения и оставляем только последние...
	unideb[Debug::CRIT] << myname << "(cleanMsgQueue): ТЕКУЩИЙ размер очереди сообщений: " << q.size() << endl;

	VoidMessage vm;
	map<UniSetTypes::KeyType,VoidMessage> smap;
	map<int,VoidMessage> tmap;
	map<int,VoidMessage> sysmap;
	list<VoidMessage> lstOther;
	map<MsgInfo,VoidMessage> amap;
	map<MsgInfo,VoidMessage> imap;
	map<MsgInfo,VoidMessage> cmap;

//		while( receiveMessage(vm) );
//		while нельзя использовать потому-что, из параллельного потока
//		могут запихивать в очередь ещё сообщения.. И это цикл никогда не прервётся...

		int max = q.size();
		for( int i=0; i<=max; i++ )
		{
			vm = q.top();
			q.pop();
			
			switch( vm.type )
			{
				case Message::SensorInfo:
				{
					SensorMessage sm(&vm);
					UniSetTypes::KeyType k(key(sm.id,sm.node));
					// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
					// то достаточно просто сохранять последнее сообщение для одинаковых Key
					smap[k] = vm;
					break;
				}

				case Message::Timer:
				{
					TimerMessage tm(&vm);
					// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
					// то достаточно просто сохранять последнее сообщение для одинаковых TimerId
					tmap[tm.id] = vm;
					break;
				}		

				case Message::SysCommand:
				{
					SystemMessage sm(&vm);
					sysmap[sm.command] = vm;
				}
				break;

				case Message::Alarm:
				{
					AlarmMessage am(&vm);
					MsgInfo mi(am);
					// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
					// то достаточно просто сохранять последнее сообщение для одинаковых MsgInfo
					amap[mi] = vm;
				}
				break;

				case Message::Info:
				{
					InfoMessage im(&vm);
					MsgInfo mi(im);
					// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
					// то достаточно просто сохранять последнее сообщение для одинаковых MsgInfo
					imap[mi] = vm;
				}
				break;
				
				case Message::Confirm:
				{
					ConfirmMessage cm(&vm);
					MsgInfo mi(cm);
					// т.к. из очереди сообщений сперва вынимаются самые старые, потом свежее и т.п.
					// то достаточно просто сохранять последнее сообщение для одинаковых MsgInfo
					cmap[mi] = vm;
				}
				break;

				case Message::Unused:
					// просто выкидываем (игнорируем)
				break;
				
				default:
					// сразу пизаем
					lstOther.push_front(vm);
				break;

			}
		}	

		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по SensorMessage: " << smap.size() << endl;
		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по TimerMessage: " << tmap.size() << endl;
		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по SystemMessage: " << sysmap.size() << endl;
		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по AlarmMessage: " << amap.size() << endl;
		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по InfoMessage: " << imap.size() << endl;
		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по ConfirmMessage: " << cmap.size() << endl;
		unideb[Debug::CRIT] << myname << "(cleanMsgQueue): осталось по Остальным: " << lstOther.size() << endl;

		// теперь ОСТАВШИЕСЯ запихиваем обратно в очередь...

		map<UniSetTypes::KeyType,VoidMessage>::iterator it=smap.begin();
		for( ; it!=smap.end(); ++it )
		{
			q.push(it->second);
		}

		map<int,VoidMessage>::iterator it1=tmap.begin();
		for( ; it1!=tmap.end(); ++it1 )
		{
			q.push(it1->second);
		}

		map<int,VoidMessage>::iterator it2=sysmap.begin();
		for( ; it2!=sysmap.end(); ++it2 )
		{
			q.push(it2->second);
		}

		map<MsgInfo,VoidMessage>::iterator it3=amap.begin();
		for( ; it3!=amap.end(); ++it3 )
		{
			q.push(it3->second);
		}

		map<MsgInfo,VoidMessage>::iterator it4=imap.begin();
		for( ; it4!=imap.end(); ++it4 )
		{
			q.push(it4->second);
		}

		map<MsgInfo,VoidMessage>::iterator it5=cmap.begin();
		for( ; it5!=cmap.end(); ++it5 )
		{
			q.push(it5->second);
		}

		list<VoidMessage>::iterator it6=lstOther.begin();
		for( ; it6!=lstOther.end(); ++it6 )
		{
			q.push(*it6);
		}
		
		unideb[Debug::CRIT] << myname 
					<< "(cleanMsgQueue): РЕЗУЛЬТАТ размер очереди сообщений: " 
					<< countMessages()
					<< " < " << getMaxSizeOfMessageQueue() << endl;
		
		if( q.size() >= getMaxSizeOfMessageQueue() )
		{
			unideb[Debug::CRIT] << myname << "(cleanMsgQueue): НЕ ПОМОГЛО!!! Сообщений осталось больше > " << q.size() << endl;
			unideb[Debug::CRIT] << myname << "(cleanMsgQueue): Просто удаляем " << getMaxCountRemoveOfMessage() << " старых сообщений\n";
			for( unsigned int i=0; i<getMaxCountRemoveOfMessage() && !q.empty(); i++ )
			{
				q.top(); 
				q.pop(); 
			}
		}
}
// ------------------------------------------------------------------------------------------
unsigned int UniSetObject::countMessages()
{
	{ // lock
		uniset_mutex_lock mlk(qmutex, 200);
		return queueMsg.size();
	}
}
// ------------------------------------------------------------------------------------------
bool UniSetObject::disactivate()
{
	if( !active )
	{
		try
		{
			disactivateObject();
		}
		catch(...){}
		return true;
	}

	active=false; // завершаем поток обработки сообщений
	tmr->stop();

	// Очищаем очередь
	{ // lock
		uniset_mutex_lock mlk(qmutex, 200);
		while( !queueMsg.empty() )
			queueMsg.pop(); 
	}

	try
	{
		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << "disactivateObject..." << endl;

		PortableServer::POA_var poamngr = mymngr->getPOA();
		if( !PortableServer::POA_Helper::is_nil(poamngr) )
		{
			try
			{
				disactivateObject();
			}
			catch(...){}
			unregister();
			PortableServer::ObjectId_var oid = poamngr->servant_to_id(static_cast<PortableServer::ServantBase*>(this));
			poamngr->deactivate_object(oid);
			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << "ok..." << endl;
			return true;
		}
		unideb[Debug::WARN] << "manager уже уничтожен..." << endl;
	}
	catch(CORBA::TRANSIENT)
	{
		unideb[Debug::WARN] << "isExist: нет связи..."<< endl;
	}
	catch( CORBA::SystemException& ex )
    {
		unideb[Debug::WARN] << "UniSetObject: "<<"поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
    }
    catch(CORBA::Exception& ex)
    {
		unideb[Debug::WARN] << "UniSetObject: "<<"поймали CORBA::Exception." << endl;
    }
	catch(Exception& ex)
    {
		unideb[Debug::WARN] << "UniSetObject: "<< ex << endl;
    }
    catch(...)
    {
		unideb[Debug::WARN] << "UniSetObject: "<<" catch ..." << endl;
    }

	return false;
}

// ------------------------------------------------------------------------------------------
bool UniSetObject::activate()
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": activate..." << endl;

	if( mymngr == NULL )
	{
		unideb[Debug::CRIT] << myname << "(activate): mymngr=NULL!!! activate failure..." << endl;
		return false;
	}

	PortableServer::POA_var poa = mymngr->getPOA();
	if( poa == NULL || CORBA::is_nil(poa) )
	{
		string err(myname+": не задан менеджер");
		throw ORepFailed(err.c_str());
	}

	if( conf->isTransientIOR() )
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
			unideb[Debug::CRIT] << myname << "(activate): Не задан ID!!! activate failure..." << endl;
			// вызываем на случай если она переопределена в дочерних классах
			// Например в ObjectsManager, если здесь не вызвать, то не будут инициализированы подчинённые объекты.
			// (см. ObjectsManager::activateObject)
			activateObject();
			return false;
		}

	    // Always use the same object id.
    	PortableServer::ObjectId_var oid =
		PortableServer::string_to_ObjectId(myname.c_str());

//		cerr << myname << "(activate): " << _refcount_value() << endl;

    	// Activate object...
	    poa->activate_object_with_id(oid, this);
	}
	

	
	oref = poa->servant_to_reference(static_cast<PortableServer::ServantBase*>(this) );

	registered();
	// Запускаем поток обработки сообщений
	active=true;

	if( myid!=UniSetTypes::DefaultObjectId && threadcreate )
	{
		thr = new ThreadCreator<UniSetObject>(this, &UniSetObject::work);
		thr->start();
	}
	else 
	{
		if( unideb.debugging(Debug::INFO) )
		{
			unideb[Debug::INFO] << myname << ": ?? не задан ObjectId...(" 
					<< "myid=" << myid << " threadcreate=" << threadcreate 
					<< ")" << endl;
		}
		thread(false);
	}

	activateObject();
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": activate ok." << endl;
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::work()
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": thread processing messages run..." << endl;
	if( thr )
		msgpid = thr->getTID();
	while( active )
	{
		callback();
	}
	unideb[Debug::WARN] << myname << ": thread processing messages stop..." << endl;	
}
// ------------------------------------------------------------------------------------------
void UniSetObject::callback()
{
	try
	{
		if( waitMessage(msg) )
			processingMessage(&msg);
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------
void UniSetObject::processingMessage( UniSetTypes::VoidMessage *msg )
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << ": default processing messages..." << endl;	
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
			msgpid = thr->getTID();	// заодно(на всякий) обновим и внутреннюю информацию
			info << msgpid;  
		}
		else
			info << "не запущен";
	}
	else
		info << "откл.";  
	
	info << "\tcount=" << countMessages();
	info << "\tmaxMsg=" << stMaxQueueMessages;
	info << "\tqFull("<< SizeOfMessageQueue << ")=" << stCountOfQueueFull;
//	info << "\n";
	
	SimpleInfo* res = new SimpleInfo();
	res->info 	=  info.str().c_str(); // CORBA::string_dup(info.str().c_str());
	res->id 	=  myid;
	
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
