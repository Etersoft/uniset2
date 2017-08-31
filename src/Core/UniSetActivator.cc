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
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sstream>
#include <fstream>

#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

// for stack trace
// --------------------
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <iomanip>
// --------------------

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "UniSetActivator.h"
#include "Debug.h"
#include "Configuration.h"
#include "Mutex.h"

// ------------------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// ------------------------------------------------------------------------------------------
namespace uniset
{
UniSetActivatorPtr UniSetActivator::inst;
// ---------------------------------------------------------------------------
UniSetActivatorPtr UniSetActivator::Instance()
{
	if( inst == nullptr )
		inst = shared_ptr<UniSetActivator>( new UniSetActivator() );

	return inst;
}

// ---------------------------------------------------------------------------
UniSetActivator::UniSetActivator():
	UniSetManager(uniset::DefaultObjectId)
{
	UniSetActivator::init();
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::init()
{
	if( getId() == DefaultObjectId )
		myname = "UniSetActivator";

	auto conf = uniset_conf();

#ifndef DISABLE_REST_API

	if( findArgParam("--activator-run-httpserver", conf->getArgc(), conf->getArgv()) != -1 )
	{
		httpHost = conf->getArgParam("--activator-httpserver-host", "localhost");
		ostringstream s;
		s << (getId() == DefaultObjectId ? 8080 : getId() );
		httpPort = conf->getArgInt("--activator-httpserver-port", s.str());
		ulog1 << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
	}

#endif

	orb = conf->getORB();
	CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
	PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);
	pman = root_poa->the_POAManager();
	const CORBA::PolicyList pl = conf->getPolicy();
	poa = root_poa->create_POA("my poa", pman, pl);

	if( CORBA::is_nil(poa) )
	{
		ucrit << myname << "(init): init poa failed!!!" << endl;
		std::terminate();
	}

	sigTERM.set(loop);
	sigINT.set(loop);
	sigABRT.set(loop);
	sigQUIT.set(loop);
	sigINT.set<&UniSetActivator::evsignal>();
	sigTERM.set<&UniSetActivator::evsignal>();
	sigABRT.set<&UniSetActivator::evsignal>();
	sigQUIT.set<&UniSetActivator::evsignal>();
}

// ------------------------------------------------------------------------------------------
UniSetActivator::~UniSetActivator()
{
	if( active )
		stop();
}
// ------------------------------------------------------------------------------------------
/*!
 *    Если thread=true то функция создает отдельный поток для обработки приходящих сообщений.
 *     И передает все ресурсы этого потока orb. А также регистрирует процесс в репозитории.
 *    \note Только после этого объект становится доступен другим процессам
 *    А далее выходит...
 *    Иначе все ресурсы основного потока передаются для обработки приходящих сообщений (и она не выходит)
 *
*/
void UniSetActivator::run( bool thread )
{
	ulogsys << myname << "(run): создаю менеджер " << endl;

	auto aptr = std::static_pointer_cast<UniSetActivator>(get_ptr());

	UniSetManager::initPOA(aptr);

	if( getId() == uniset::DefaultObjectId )
		offThread(); // отключение потока обработки сообщений, раз не задан ObjectId

	UniSetManager::activate(); // а там вызывается активация всех подчиненных объектов и менеджеров
	active = true;

	ulogsys << myname << "(run): активизируем менеджер" << endl;
	pman->activate();
	msleep(50);

	if( !thread )
		async_evrun();

#ifndef DISABLE_REST_API

	if( !httpHost.empty() )
	{
		try
		{
			auto reg = dynamic_pointer_cast<UHttp::IHttpRequestRegistry>(shared_from_this());
			httpserv = make_shared<UHttp::UHttpServer>(reg, httpHost, httpPort);
			httpserv->start();
		}
		catch( std::exception& ex )
		{
			uwarn << myname << "(run): init http server error: " << ex.what() << endl;
		}
	}

#endif

	if( thread )
	{
		uinfo << myname << "(run): запускаемся с созданием отдельного потока...  " << endl;
		orbthr = make_shared< OmniThreadCreator<UniSetActivator> >( aptr, &UniSetActivator::work);
		orbthr->start();
	}
	else
	{
		uinfo << myname << "(run): запускаемся без создания отдельного потока...  " << endl;
		work();
		evstop();
	}
}
// ------------------------------------------------------------------------------------------
/*!
 *    Функция останавливает работу orb и завершает поток. А так же удаляет ссылку из репозитория.
 *    \note Объект становится недоступен другим процессам
*/
void UniSetActivator::stop()
{
	if( !active )
		return;

	active = false;

	ulogsys << myname << "(stop): deactivate...  " << endl;

	deactivate();

	ulogsys << myname << "(stop): deactivate ok.  " << endl;
	ulogsys << myname << "(stop): discard request..." << endl;

	pman->discard_requests(true);

	ulogsys << myname << "(stop): discard request ok." << endl;

#ifndef DISABLE_REST_API

	if( httpserv )
		httpserv->stop();

#endif

	ulogsys << myname << "(stop): pman deactivate... " << endl;
	pman->deactivate(false, true);
	ulogsys << myname << "(stop): pman deactivate ok. " << endl;

	try
	{
		ulogsys << myname << "(stop): shutdown orb...  " << endl;

		if( orb )
			orb->shutdown(true);

		ulogsys << myname << "(stop): shutdown ok." << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		ucrit << myname << "(stop): : поймали omniORB::fatalException:" << endl;
		ucrit << myname << "(stop):   file: " << fe.file() << endl;
		ucrit << myname << "(stop):   line: " << fe.line() << endl;
		ucrit << myname << "(stop):   mesg: " << fe.errmsg() << endl;
	}
	catch( const std::exception& ex )
	{
		ucrit << myname << "(stop): " << ex.what() << endl;
	}

	if( orbthr )
		orbthr->join();
}

// ------------------------------------------------------------------------------------------
void UniSetActivator::evsignal( ev::sig& signal, int signo )
{
	auto act = UniSetActivator::Instance();
	act->stop();
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::evprepare()
{
	sigINT.start(SIGINT);
	sigTERM.start(SIGTERM);
	sigABRT.start(SIGABRT);
	sigQUIT.start(SIGQUIT);
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::evfinish()
{
	sigINT.stop();
	sigTERM.stop();
	sigABRT.stop();
	sigQUIT.stop();
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::work()
{
	ulogsys << myname << "(work): запускаем orb на обработку запросов..." << endl;

	try
	{
		omniORB::setMainThread();
		orb->run();
	}
	catch( const CORBA::SystemException& ex )
	{
		ucrit << myname << "(work): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& ex )
	{
		ucrit << myname << "(work): поймали CORBA::Exception." << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		ucrit << myname << "(work): : поймали omniORB::fatalException:" << endl;
		ucrit << myname << "(work):   file: " << fe.file() << endl;
		ucrit << myname << "(work):   line: " << fe.line() << endl;
		ucrit << myname << "(work):   mesg: " << fe.errmsg() << endl;
	}
	catch( std::exception& ex )
	{
		ucrit << myname << "(work): catch: " << ex.what() << endl;
	}

	ulogsys << myname << "(work): orb thread stopped!" << endl << flush;
}
// ------------------------------------------------------------------------------------------
#ifndef DISABLE_REST_API
Poco::JSON::Object::Ptr UniSetActivator::httpGetByName( const string& name, const Poco::URI::QueryParameters& p )
{
	if( name == myname )
		return httpGet(p);

	auto obj = deepFindObject(name);

	if( obj )
		return obj->httpGet(p);

	ostringstream err;
	err << "Object '" << name << "' not found";

	throw uniset::NameNotFound(err.str());
}
// ------------------------------------------------------------------------------------------
Poco::JSON::Array::Ptr UniSetActivator::httpGetObjectsList( const Poco::URI::QueryParameters& p )
{
	Poco::JSON::Array::Ptr jdata = new Poco::JSON::Array();

	std::vector<std::shared_ptr<UniSetObject>> vec;
	vec.reserve(objectsCount());

	//! \todo Доделать обработку параметров beg,lim на случай большого количества объектов (и частичных запросов)
	size_t lim = 1000;
	getAllObjectsList(vec, lim);

	for( const auto& o : vec )
		jdata->add(o->getName());

	return jdata;
}
// ------------------------------------------------------------------------------------------
Poco::JSON::Object::Ptr UniSetActivator::httpHelpByName( const string& name, const Poco::URI::QueryParameters& p )
{
	if( name == myname )
		return httpHelp(p);

	auto obj = deepFindObject(name);

	if( obj )
		return obj->httpHelp(p);

	ostringstream err;
	err << "Object '" << name << "' not found";
	throw uniset::NameNotFound(err.str());
}
// ------------------------------------------------------------------------------------------
Poco::JSON::Object::Ptr UniSetActivator::httpRequestByName( const string& name, const std::string& req, const Poco::URI::QueryParameters& p)
{
	if( name == myname )
		return httpRequest(req, p);

	// а вдруг встретится объект с именем "conf" а мы перекрываем имя?!
	// (пока считаем что такого не будет)
	if( name == "conf" )
		return request_conf(req, p);

	auto obj = deepFindObject(name);

	if( obj )
		return obj->httpRequest(req, p);

	ostringstream err;
	err << "Object '" << name << "' not found";
	throw uniset::NameNotFound(err.str());
}
// ------------------------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
// ------------------------------------------------------------------------------------------
} // end of namespace uniset
// ------------------------------------------------------------------------------------------
