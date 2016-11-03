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
 * \brief Активатор объектов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UniSetActivator_H_
#define UniSetActivator_H_
// --------------------------------------------------------------------------
#include <deque>
#include <memory>
#include <omniORB4/CORBA.h>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "UniSetManager.h"
#include "OmniThreadCreator.h"
#include "UHttpRequestHandler.h"
#include "UHttpServer.h"
//----------------------------------------------------------------------------------------
class UniSetActivator;
typedef std::shared_ptr<UniSetActivator> UniSetActivatorPtr;
//----------------------------------------------------------------------------------------
/*! \class UniSetActivator
 *    Создает POA менеджер и регистрирует в нем объекты.
 *    Для обработки CORBA-запросов создается поток или передаются ресурсы
 *        главного потока см. void activate(bool thread)
 *    \warning Авктиватор может быть создан только один. Для его создания используйте код:
  \code
     ...
     auto act = UniSetActivator::Instance()
     ...
\endcode
 * Активатор в свою очередь сам является менеджером(и объектом) и обладает всеми его свойствами
 *
 * --uniset-no-use-gdb-for-stacktrace - НЕ ИСПОЛЬЗОВАТЬ gdb для stacktrace
 * --uniset-abort-script  - скрипт запускаемый при вылете, в качестве аргумента передаётся имя программы и pid
*/
class UniSetActivator:
	public UniSetManager,
	public UniSetTypes::UHttp::IHttpRequestRegistry
{
	public:

		static UniSetActivatorPtr Instance();
		void Destroy();

		std::shared_ptr<UniSetActivator> get_aptr();
		// ------------------------------------
		virtual ~UniSetActivator();

		virtual void run(bool thread);
		virtual void stop();
		virtual void uaDestroy(int signo = 0);

		virtual UniSetTypes::ObjectType getType() override
		{
			return UniSetTypes::ObjectType("UniSetActivator");
		}

		typedef sigc::signal<void, int> TerminateEvent_Signal;
		TerminateEvent_Signal signal_terminate_event();

		inline bool noUseGdbForStackTrace() const
		{
			return _noUseGdbForStackTrace;
		}

		inline const std::string getAbortScript()
		{
			return abortScript;
		}

		// Поддрежка REST API (IHttpRequestRegistry)
		virtual nlohmann::json httpGetByName( const std::string& name , const Poco::URI::QueryParameters& p ) override;
		virtual nlohmann::json httpGetObjectsList( const Poco::URI::QueryParameters& p ) override;
		virtual nlohmann::json httpHelpByName( const std::string& name, const Poco::URI::QueryParameters& p ) override;
		virtual nlohmann::json httpRequestByName( const std::string& name, const std::string& req, const Poco::URI::QueryParameters& p ) override;

	protected:

		virtual void work();

		CORBA::ORB_ptr getORB();

		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;

		// уносим в protected, т.к. Activator должен быть только один..
		UniSetActivator();

		static std::shared_ptr<UniSetActivator> inst;

	private:
		friend void terminate_thread();
		friend void finished_thread();
		friend std::shared_ptr<UniSetTypes::Configuration> UniSetTypes::uniset_init( int argc, const char* const* argv, const std::string& xmlfile );

		static void terminated(int signo);
		static void normalexit();
		static void normalterminate();
		static void set_signals(bool ask);
		void term( int signo );
		void init();

		std::shared_ptr< OmniThreadCreator<UniSetActivator> > orbthr;

		CORBA::ORB_var orb;
		TerminateEvent_Signal s_term;

		std::atomic_bool omDestroy;
		pid_t thid = { 0 }; // id orb потока

		bool _noUseGdbForStackTrace = { false };

		std::string abortScript = { "" }; // скрипт вызываемый при прерывании программы (SIGSEGV,SIGABRT)

		std::shared_ptr<UniSetTypes::UHttp::UHttpServer> httpserv;
		std::string httpHost = { "" };
		int httpPort = { 0 };
};
//----------------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------------
