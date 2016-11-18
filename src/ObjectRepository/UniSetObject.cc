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
#include <unordered_map>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <iomanip>
#include <pthread.h>
#include <sys/types.h>
#include <sstream>
#include <chrono>
#include <Poco/Process.h>

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "ObjectRepository.h"
#include "UInterface.h"
#include "UniSetObject.h"
#include "UniSetManager.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace std;
namespace uniset
{

#define CREATE_TIMER    make_shared<PassiveCondTimer>();
// new PassiveSysTimer();

// ------------------------------------------------------------------------------------------
UniSetObject::UniSetObject():
	msgpid(0),
	regOK(false),
	active(0),
	threadcreate(false),
	myid(uniset::DefaultObjectId),
	oref(0)
{
	ui = make_shared<UInterface>(uniset::DefaultObjectId);

	tmr = CREATE_TIMER;
	myname = "noname";
	section = "nonameSection";
	initObject();
}
// ------------------------------------------------------------------------------------------
UniSetObject::UniSetObject( ObjectId id ):
	msgpid(0),
	regOK(false),
	active(0),
	threadcreate(true),
	myid(id),
	oref(0)
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
		myid = uniset::DefaultObjectId;
		myname = "UnknownUniSetObject";
		section = "UnknownSection";
	}

	initObject();
}


UniSetObject::UniSetObject( const string& name, const string& section ):
	msgpid(0),
	regOK(false),
	active(0),
	threadcreate(true),
	myid(uniset::DefaultObjectId),
	oref(0)
{
	ui = make_shared<UInterface>(uniset::DefaultObjectId);

	/*! \warning UniverslalInterface не инициализируется идентификатором объекта */
	tmr = CREATE_TIMER;
	myname = section + "/" + name;
	myid = ui->getIdByName(myname);

	if( myid == DefaultObjectId )
	{
		uwarn << "name: my ID not found!" << endl;
		throw Exception(name + ": my ID not found!");
	}

	initObject();
	ui->initBackId(myid);
}

// ------------------------------------------------------------------------------------------
UniSetObject::~UniSetObject()
{
}
// ------------------------------------------------------------------------------------------
std::shared_ptr<UniSetObject> UniSetObject::get_ptr()
{
	return shared_from_this();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::initObject()
{
	a_working = ATOMIC_VAR_INIT(0);
	active = ATOMIC_VAR_INIT(0);

	refmutex.setName(myname + "_refmutex");
	//    mutex_act.setName(myname + "_mutex_act");

	auto conf = uniset_conf();

	if( !conf )
	{
		ostringstream err;
		err << myname << "(initObject): Unknown configuration!!";
		throw SystemError(err.str());
	}

	int sz = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 1000);

	if( sz > 0 )
		setMaxSizeOfMessageQueue(sz);

	uinfo << myname << "(init): SizeOfMessageQueue=" << getMaxSizeOfMessageQueue() << endl;
}
// ------------------------------------------------------------------------------------------

/*!
 *    \param om - указатель на менеджер управляющий объектом
 *    \return Возращает \a true если инициализация прошла успешно, и \a false если нет
*/
bool UniSetObject::init( const std::weak_ptr<UniSetManager>& om )
{
	uinfo << myname << ": init..." << endl;
	this->mymngr = om;
	uinfo << myname << ": init ok..." << endl;
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setID( uniset::ObjectId id )
{
	if( myid != uniset::DefaultObjectId )
		throw ObjectNameAlready("ObjectId already set(setID)");

	string myfullname = ui->getNameById(id);
	myname = ORepHelpers::getShortName(myfullname);
	section = ORepHelpers::getSectionName(myfullname);
	myid = id;
	ui->initBackId(myid);
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setMaxSizeOfMessageQueue( size_t s )
{
	mqueueMedium.setMaxSizeOfMessageQueue(s);
	mqueueLow.setMaxSizeOfMessageQueue(s);
	mqueueHi.setMaxSizeOfMessageQueue(s);
}
// ------------------------------------------------------------------------------------------
size_t UniSetObject::getMaxSizeOfMessageQueue() const
{
	return mqueueMedium.getMaxSizeOfMessageQueue();
}
// ------------------------------------------------------------------------------------------
bool UniSetObject::isActive() const
{
	return active;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setActive(bool set)
{
	active = set;
}
// ------------------------------------------------------------------------------------------
/*!
 *    \param  vm - указатель на структуру, которая заполняется если есть сообщение
 *    \return Возвращает указатель VoidMessagePtr если сообщение есть, и shared_ptr(nullptr) если нет
*/
VoidMessagePtr UniSetObject::receiveMessage()
{
	if( !mqueueHi.empty() )
		return mqueueHi.top();

	if( !mqueueMedium.empty() )
		return mqueueMedium.top();

	return mqueueLow.top();
}
// ------------------------------------------------------------------------------------------
VoidMessagePtr UniSetObject::waitMessage( timeout_t timeMS )
{
	auto m = receiveMessage();

	if( m )
		return m;

	tmr->wait(timeMS);
	return receiveMessage();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::registered()
{
	ulogrep << myname << ": registration..." << endl;

	if( myid == uniset::DefaultObjectId )
	{
		ulogrep << myname << "(registered): Don`t registration. myid=DefaultObjectId \n";
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
		uniset::uniset_rwmutex_rlock lock(refmutex);

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
	catch( const uniset::Exception& ex )
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

	if( myid == uniset::DefaultObjectId )
	{
		uinfo << myname << "(unregister): myid=DefaultObjectId \n";
		regOK = false;
		return;
	}

	{
		uniset::uniset_rwmutex_rlock lock(refmutex);

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
ObjectId UniSetObject::getId()
{
	return myid;
}
// ------------------------------------------------------------------------------------------
const ObjectId UniSetObject::getId() const
{
	return myid;
}
// ------------------------------------------------------------------------------------------
string UniSetObject::getName() const
{
	return myname;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::termWaiting()
{
	if( tmr )
		tmr->terminate();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::setThreadPriority( Poco::Thread::Priority p )
{
	if( thr )
		thr->setPriority(p);
}
// ------------------------------------------------------------------------------------------
void UniSetObject::push( const TransportMessage& tm )
{
	auto vm = make_shared<VoidMessage>(tm);

	if( vm->priority == Message::Medium )
		mqueueMedium.push(vm);
	else if( vm->priority == Message::High )
		mqueueHi.push(vm);
	else if( vm->priority == Message::Low )
		mqueueLow.push(vm);
	else // на всякий по умолчанию medium
		mqueueMedium.push(vm);

	termWaiting();
}
// ------------------------------------------------------------------------------------------
#ifndef DISABLE_REST_API
nlohmann::json UniSetObject::httpGet( const Poco::URI::QueryParameters& p )
{
	nlohmann::json jret;
	auto& jdata = jret[myname];

	jdata["name"] = myname;
	jdata["id"] = getId();
	jdata["msgCount"] = countMessages();
	jdata["lostMessages"] = getCountOfLostMessages();
	jdata["maxSizeOfMessageQueue"] = getMaxSizeOfMessageQueue();
	jdata["isActive"] = isActive();
	jdata["objectType"] = getType();

	return jret;
}
// ------------------------------------------------------------------------------------------
nlohmann::json UniSetObject::httpHelp( const Poco::URI::QueryParameters& p )
{
	nlohmann::json jdata;
	jdata[myname]["help"] = {};
	return jdata;
}
#endif
// ------------------------------------------------------------------------------------------
ObjectPtr UniSetObject::getRef() const
{
	uniset::uniset_rwmutex_rlock lock(refmutex);
	return (uniset::ObjectPtr)CORBA::Object::_duplicate(oref);
}
// ------------------------------------------------------------------------------------------
size_t UniSetObject::countMessages()
{
	return (mqueueMedium.size() + mqueueLow.size() + mqueueHi.size());
}
// ------------------------------------------------------------------------------------------
size_t UniSetObject::getCountOfLostMessages() const
{
	return (mqueueMedium.getCountOfLostMessages() +
			mqueueLow.getCountOfLostMessages() +
			mqueueHi.getCountOfLostMessages() );
}
// ------------------------------------------------------------------------------------------
bool UniSetObject::activateObject()
{
	return true;
}
// ------------------------------------------------------------------------------------------
bool UniSetObject::deactivateObject()
{
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::sigterm( int signo )
{
}
// ------------------------------------------------------------------------------------------
void UniSetObject::terminate()
{
	deactivate();
}
// ------------------------------------------------------------------------------------------
void UniSetObject::thread(bool create)
{
	threadcreate = create;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::offThread()
{
	threadcreate = false;
}
// ------------------------------------------------------------------------------------------
void UniSetObject::onThread()
{
	threadcreate = true;
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
		catch( std::exception& ex )
		{
			uwarn << myname << "(deactivate): " << ex.what() << endl;
		}

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
				catch( std::exception& ex )
				{
					uwarn << myname << "(deactivate): " << ex.what() << endl;
				}

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
	catch( const uniset::Exception& ex )
	{
		uwarn << myname << "(deactivate): " << ex << endl;
	}
	catch( std::exception& ex )
	{
		uwarn << myname << "(deactivate): " << ex.what() << endl;
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
		// А если myid==uniset::DefaultObjectId
		// то myname = noname. ВСЕГДА!
		if( myid == uniset::DefaultObjectId )
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
		uniset::uniset_rwmutex_wrlock lock(refmutex);
		oref = poa->servant_to_reference(static_cast<PortableServer::ServantBase*>(this) );
	}

	registered();
	// Запускаем поток обработки сообщений
	setActive(true);

	if( myid != uniset::DefaultObjectId && threadcreate )
	{
		thr = make_shared< ThreadCreator<UniSetObject> >(this, &UniSetObject::work);
		//thr->setCancel(ost::Thread::cancelDeferred);

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

	msgpid = thr ? thr->getTID() : Poco::Process::id();

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
	// При реализации с использованием waitMessage() каждый раз при вызове askTimer() необходимо
	// проверять возвращаемое значение на UniSetTimers::WaitUpTime и вызывать termWaiting(),
	// чтобы избежать ситуации, когда процесс до заказа таймера 'спал'(в функции waitMessage()) и после
	// заказа продолжит спать(т.е. обработчик вызван не будет)...
	try
	{
		auto m = waitMessage(sleepTime);

		if( m )
			processingMessage(m.get());

		sleepTime = checkTimers(this);
	}
	catch( const uniset::Exception& ex )
	{
		ucrit << myname << "(callback): " << ex << endl;
	}
}
// ------------------------------------------------------------------------------------------
void UniSetObject::processingMessage( const uniset::VoidMessage* msg )
{
	try
	{
		switch( msg->type )
		{
			case Message::SensorInfo:
				sensorInfo( reinterpret_cast<const SensorMessage*>(msg) );
				break;

			case Message::Timer:
				timerInfo( reinterpret_cast<const TimerMessage*>(msg) );
				break;

			case Message::SysCommand:
				sysCommand( reinterpret_cast<const SystemMessage*>(msg) );
				break;

			default:
				break;
		}
	}
	catch( const uniset::Exception& ex )
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
timeout_t UniSetObject::askTimer( TimerId timerid, timeout_t timeMS, clock_t ticks, Message::Priority p )
{
	timeout_t tsleep = LT_Object::askTimer(timerid, timeMS, ticks, p);

	if( tsleep != UniSetTimer::WaitUpTime )
		termWaiting();

	return tsleep;
}
// ------------------------------------------------------------------------------------------

uniset::SimpleInfo* UniSetObject::getInfo( ::CORBA::Long userparam )
{
	ostringstream info;
	info.setf(ios::left, ios::adjustfield);
	info << "(" << myid << ")" << setw(40) << myname << "\n==================================================\n";
	info << "pid=" << setw(10) << Poco::Process::id();
	info << " tid=" << setw(10);

	if( threadcreate )
	{
		if(thr)
		{
			msgpid = thr->getTID();    // заодно(на всякий) обновим и внутреннюю информацию
			info << msgpid;
		}
		else
			info << "не запущен";
	}
	else
		info << "откл.";

	info << "\tcount=" << countMessages()
		 << "\t medium: "
		 << " maxMsg=" << mqueueMedium.getMaxQueueMessages()
		 << " qFull(" << mqueueMedium.getMaxSizeOfMessageQueue() << ")=" << mqueueMedium.getCountOfLostMessages()
		 << "\t     hi: "
		 << " maxMsg=" << mqueueHi.getMaxQueueMessages()
		 << " qFull(" << mqueueHi.getMaxSizeOfMessageQueue() << ")=" << mqueueHi.getCountOfLostMessages()
		 << "\t    low: "
		 << " maxMsg=" << mqueueLow.getMaxQueueMessages()
		 << " qFull(" << mqueueLow.getMaxSizeOfMessageQueue() << ")=" << mqueueLow.getCountOfLostMessages();

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
#undef CREATE_TIMER
} // end of namespace uniset
