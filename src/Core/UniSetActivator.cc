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
#include <sstream>
#include <fstream>

#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

// for stack trace
// --------------------
//#include <execinfo.h>
//#include <cxxabi.h>
//#include <dlfcn.h>
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
static std::mutex              g_donemutex;
static std::condition_variable g_doneevent;
static std::shared_ptr<std::thread> g_finish_guard_thread;
static std::atomic_bool g_done = ATOMIC_VAR_INIT(0);

static const int TERMINATE_TIMEOUT_SEC = 15; //  время, отведённое на завершение процесса [сек]
// ------------------------------------------------------------------------------------------
struct ORBThreadDeleter
{
    void operator()( OmniThreadCreator<UniSetActivator>* p ) const
    {
        // не удаляем..
    }
};
// ---------------------------------------------------------------------------
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
            httpCORS_allow = conf->getArgParam("--activator-httpserver-cors-allow", "*");
            httpDefaultContentType = conf->getArgParam("--activator-httpserver-default-content-type", "text/json; charset=UTF-8");
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
    }

    // ------------------------------------------------------------------------------------------
    UniSetActivator::~UniSetActivator()
    {
        if( active )
        {
            try
            {
                shutdown();
            }
            catch(...)
            {
                std::exception_ptr p = std::current_exception();
                cerr << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
            }
        }
    }
    // ------------------------------------------------------------------------------------------

    void UniSetActivator::run( bool thread, bool terminate_control  )
    {
        ulogsys << myname << "(run): создаю менеджер " << endl;

        termControl = terminate_control;
        auto aptr = std::static_pointer_cast<UniSetActivator>(get_ptr());

        UniSetManager::initPOA(aptr);

        if( getId() == uniset::DefaultObjectId )
            offThread(); // отключение потока обработки сообщений, раз не задан ObjectId

        UniSetManager::activate(); // а там вызывается активация всех подчиненных объектов и менеджеров
        active = true;

        ulogsys << myname << "(run): активизируем менеджер" << endl;
        pman->activate();
        msleep(50);

        if( termControl )
            set_signals(true);

#ifndef DISABLE_REST_API

        if( !httpHost.empty() )
        {
            try
            {
                auto reg = dynamic_pointer_cast<UHttp::IHttpRequestRegistry>(shared_from_this());
                httpserv = make_shared<UHttp::UHttpServer>(reg, httpHost, httpPort);
                httpserv->setCORS_allow(httpCORS_allow);
                httpserv->setDefaultContentType(httpDefaultContentType);
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
            orbthr = std::shared_ptr<OmniThreadCreator<UniSetActivator>>(new OmniThreadCreator<UniSetActivator>(aptr, &UniSetActivator::mainWork), ORBThreadDeleter());
            orbthr->start();
        }
        else
        {
            uinfo << myname << "(run): запускаемся без создания отдельного потока...  " << endl;
            mainWork();
            shutdown();
        }
    }
    // ------------------------------------------------------------------------------------------
    /*!
     *    Функция останавливает работу orb и завершает поток, а также удаляет ссылку из репозитория.
     *    \note Объект становится недоступен другим процессам
    */
    void UniSetActivator::shutdown()
    {
        if( !active )
            return;

        active = false;

        if( termControl )
        {
            set_signals(false);
            {
                std::unique_lock<std::mutex> lk(g_donemutex);
                g_done = false;
                g_finish_guard_thread = make_shared<std::thread>(on_finish_timeout);
            }
        }

        ulogsys << myname << "(shutdown): deactivate...  " << endl;
        deactivate();
        ulogsys << myname << "(shutdown): deactivate ok.  " << endl;

        ulogsys << myname << "(shutdown): discard request..." << endl;
        pman->discard_requests(true);
        ulogsys << myname << "(shutdown): discard request ok." << endl;

#ifndef DISABLE_REST_API

        if( httpserv )
            httpserv->stop();

#endif

        ulogsys << myname << "(shutdown): pman deactivate... " << endl;
        pman->deactivate(false, true);
        ulogsys << myname << "(shutdown): pman deactivate ok. " << endl;

        try
        {
            ulogsys << myname << "(shutdown): shutdown orb...  " << endl;

            if( orb )
                orb->shutdown(true);

            ulogsys << myname << "(shutdown): shutdown ok." << endl;
        }
        catch( const omniORB::fatalException& fe )
        {
            ucrit << myname << "(shutdown): : поймали omniORB::fatalException:" << endl;
            ucrit << myname << "(shutdown):   file: " << fe.file() << endl;
            ucrit << myname << "(shutdown):   line: " << fe.line() << endl;
            ucrit << myname << "(shutdown):   mesg: " << fe.errmsg() << endl;
        }
        catch( const std::exception& ex )
        {
            ucrit << myname << "(shutdown): " << ex.what() << endl;
        }

        {
            std::unique_lock<std::mutex> lk(g_donemutex);
            g_done = true;
        }

        g_doneevent.notify_all();

        if( g_finish_guard_thread )
            g_finish_guard_thread->join();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::join()
    {
        if( g_done )
            return;

        ulogsys << myname << "(join): ..." << endl;

        std::unique_lock<std::mutex> lk(g_donemutex);
        g_doneevent.wait(lk, []()
        {
            return (g_done == true);
        } );
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::terminate()
    {
        ulogsys << myname << "(terminate): ..." << endl;
        kill(getpid(), SIGTERM);
    }
    // ------------------------------------------------------------------------------------------
    static void activator_terminate( int signo )
    {
        auto act = UniSetActivator::Instance();
        act->shutdown();
        ulogsys << "******** activator_terminate finished **** " << endl;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::on_finish_timeout()
    {
        std::unique_lock<std::mutex> lk(g_donemutex);

        if( g_done )
            return;

        ulogsys << "(FINISH GUARD THREAD): wait " << TERMINATE_TIMEOUT_SEC << " sec.." << endl << flush;

        g_doneevent.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::milliseconds(TERMINATE_TIMEOUT_SEC * 1000), []()
        {
            return (g_done == true);
        } );

        if( !g_done )
        {
            ulogsys << "(FINISH GUARD THREAD): WAIT TIMEOUT "
                    << TERMINATE_TIMEOUT_SEC << " sec..KILL *******" << endl << flush;
            set_signals(false);
            std::abort();
            return;
        }

        ulogsys << "(FINISH GUARD THREAD): [OK]..bye.." << endl;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::set_signals( bool ask )
    {
        struct sigaction act; // = { { 0 } };
        struct sigaction oact; // = { { 0 } };
        memset(&act, 0, sizeof(act));
        memset(&act, 0, sizeof(oact));

        sigemptyset(&act.sa_mask);
        sigemptyset(&oact.sa_mask);

        // добавляем сигналы, которые будут игнорироваться
        // при обработке сигнала
        sigaddset(&act.sa_mask, SIGINT);
        sigaddset(&act.sa_mask, SIGTERM);
        sigaddset(&act.sa_mask, SIGABRT );
        sigaddset(&act.sa_mask, SIGQUIT);

        if(ask)
            act.sa_handler = activator_terminate;
        else
            act.sa_handler = SIG_DFL;

        sigaction(SIGINT, &act, &oact);
        sigaction(SIGTERM, &act, &oact);
        sigaction(SIGABRT, &act, &oact);
        sigaction(SIGQUIT, &act, &oact);

        // SIGSEGV отдельно
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGSEGV);
        act.sa_flags = 0;
        //  act.sa_flags |= SA_RESTART;
        act.sa_flags |= SA_RESETHAND;

#if 0
        g_sigseg_stack.ss_sp = g_stack_body;
        g_sigseg_stack.ss_flags = SS_ONSTACK;
        g_sigseg_stack.ss_size = sizeof(g_stack_body);
        assert(!sigaltstack(&g_sigseg_stack, nullptr));
        act.sa_flags |= SA_ONSTACK;
#endif

        //  if(ask)
        //      act.sa_handler = activator_terminate_with_calltrace;
        //  else
        //      act.sa_handler = SIG_DFL;

        //  sigaction(SIGSEGV, &act, &oact);
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::mainWork()
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
        if( name == "configure" )
            return request_configure(req, p);

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
