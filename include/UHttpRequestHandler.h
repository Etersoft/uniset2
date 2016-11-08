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
#ifndef UHttpRequesrHandler_H_
#define UHttpRequesrHandler_H_
// -------------------------------------------------------------------------
#include <memory>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/URI.h>
#include "json.hpp"
#include "DebugStream.h"
// -------------------------------------------------------------------------
/*! \page UHttpServer API
 *
 * Формат запроса: /api/version/xxx
 *
 * Пока поддерживается только метод GET
 * Ответ в формате JSON
 * В текущем API не подразумеваются запросы глубже '/api/version/ObjectName/xxxx'
 *
 * Версия API: v01
 * /api/version/list              - Получение списка доступных объектов
 * /api/version/help              - Получение списка доступных команд
 * /api/version/ObjectName        - получение информации об объекте ObjectName
 * /api/version/ObjectName/help   - получение списка доступных команд для объекта ObjectName
 * /api/version/ObjectName/xxxx   - 'xxx' запрос к объекту ObjectName
 *
 * \todo подумать над /api/version/tree - получение "дерева" объектов (древовидный список с учётом подчинения Manager/Objects)
*/
// -------------------------------------------------------------------------
namespace uniset
{
	namespace UHttp
	{
		// текущая версия API
		const std::string UHTTP_API_VERSION="v01";

		/*! интерфейс для объекта выдающего json-данные */
		class IHttpRequest
		{
			public:
				IHttpRequest(){}
				virtual ~IHttpRequest(){}

				// throw SystemError
				virtual nlohmann::json httpGet( const Poco::URI::QueryParameters& p ) = 0;
				virtual nlohmann::json httpHelp( const Poco::URI::QueryParameters& p ) = 0;

				// не обязательная функция.
				virtual nlohmann::json httpRequest( const std::string& req, const Poco::URI::QueryParameters& p );
		};
		// -------------------------------------------------------------------------
		/*! интерфейс для обработки запросов к объектам */
		class IHttpRequestRegistry
		{
			public:
				IHttpRequestRegistry(){}
				virtual ~IHttpRequestRegistry(){}

				// throw SystemError, NameNotFound
				virtual nlohmann::json httpGetByName( const std::string& name, const Poco::URI::QueryParameters& p ) = 0;

				// throw SystemError
				virtual nlohmann::json httpGetObjectsList( const Poco::URI::QueryParameters& p ) = 0;
				virtual nlohmann::json httpHelpByName( const std::string& name, const Poco::URI::QueryParameters& p ) = 0;
				virtual nlohmann::json httpRequestByName( const std::string& name, const std::string& req, const Poco::URI::QueryParameters& p ) = 0;
		};

		// -------------------------------------------------------------------------
		class UHttpRequestHandler:
			public Poco::Net::HTTPRequestHandler
		{
			public:
				UHttpRequestHandler( std::shared_ptr<IHttpRequestRegistry> _registry );

				virtual void handleRequest( Poco::Net::HTTPServerRequest &req, Poco::Net::HTTPServerResponse &resp ) override;

			private:

				std::shared_ptr<IHttpRequestRegistry> registry;
				std::shared_ptr<DebugStream> log;
		};
		// -------------------------------------------------------------------------
		class UHttpRequestHandlerFactory:
			public Poco::Net::HTTPRequestHandlerFactory
		{
			public:

				UHttpRequestHandlerFactory( std::shared_ptr<IHttpRequestRegistry>& _registry );

				virtual Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest & ) override;

			private:
				std::shared_ptr<IHttpRequestRegistry> registry;
		};
	}
// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UHttpRequesrHandler_H_
// -------------------------------------------------------------------------
