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
namespace uniset
{
	//----------------------------------------------------------------------------------------
	class UniSetActivator;
	typedef std::shared_ptr<UniSetActivator> UniSetActivatorPtr;
	//----------------------------------------------------------------------------------------
	/*! \class UniSetActivator
	 *    Создает POA менеджер и регистрирует в нем объекты.
	 *    Для обработки CORBA-запросов создается поток или передаются ресурсы
	 *        главного потока см. void activate(bool thread)
	 *    \warning Активатор может быть создан только один. Для его создания используйте код:
	  \code
	     ...
	     auto act = UniSetActivator::Instance()
	     ...
	\endcode
	 * Активатор в свою очередь сам является менеджером(и объектом) и обладает всеми его свойствами
	 *
	 * \section act_HttpAPI REST API
	 * UniSetActivator выступает в роли http-сервера и реализует первичную обработку запросов
	 * и перенаправление их указанным объектам. Помимо этого UniSetActivator реализует обработку команд /conf/..
	 * Для запуска http-сервера необходимо в аргументах командной строки указать  --activator-run-httpserver
	 * Помимо этого можно задать параметры --activator-httpserver-host и --activator-httpserver-port.
	 * --activator-httpserver-cors-allow addr - (CORS): Access-Control-Allow-Origin. Default: *.
	 *
	*/
	class UniSetActivator:
		public UniSetManager
#ifndef DISABLE_REST_API
		, public uniset::UHttp::IHttpRequestRegistry
#endif
	{
		public:

			static UniSetActivatorPtr Instance();

			virtual ~UniSetActivator();

			// запуск системы
			// async = true - асинхронный запуск (создаётся отдельный поток).
			// terminate_control = true - управление процессом завершения (обработка сигналов завершения)
			void run( bool async, bool terminate_control = true );

			// штатное завершение работы
			void shutdown();

			// ожидание завершения (если был запуск run(true))
			void join();

			// прерывание работы
			void terminate();

			virtual uniset::ObjectType getType() override
			{
				return uniset::ObjectType("UniSetActivator");
			}


#ifndef DISABLE_REST_API
			// Поддержка REST API (IHttpRequestRegistry)
			virtual Poco::JSON::Object::Ptr httpGetByName( const std::string& name , const Poco::URI::QueryParameters& p ) override;
			virtual Poco::JSON::Array::Ptr httpGetObjectsList( const Poco::URI::QueryParameters& p ) override;
			virtual Poco::JSON::Object::Ptr httpHelpByName( const std::string& name, const Poco::URI::QueryParameters& p ) override;
			virtual Poco::JSON::Object::Ptr httpRequestByName( const std::string& name, const std::string& req, const Poco::URI::QueryParameters& p ) override;
#endif

		protected:

			void mainWork();

			// уносим в protected, т.к. Activator должен быть только один..
			UniSetActivator();

			static std::shared_ptr<UniSetActivator> inst;

		private:
			void init();
			static void on_finish_timeout();
			static void set_signals( bool set );

			std::shared_ptr< OmniThreadCreator<UniSetActivator> > orbthr;

			CORBA::ORB_var orb;
			bool termControl = { true };

#ifndef DISABLE_REST_API
			std::shared_ptr<uniset::UHttp::UHttpServer> httpserv;
			std::string httpHost = { "" };
			int httpPort = { 0 };
			std::string httpCORS_allow = { "*" };
#endif
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
//----------------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------------
