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
#ifndef LogDB_H_
#define LogDB_H_
// --------------------------------------------------------------------------
#include <queue>
#include <memory>
#include "UniSetTypes.h"
#include "LogAgregator.h"
#include "DebugStream.h"
#include "SQLiteInterface.h"
#include "LogReader.h"
// -------------------------------------------------------------------------
namespace uniset
{
	//------------------------------------------------------------------------------------------
	/*!
		  \page page_LogDB База логов

		  - \ref sec_LogDB_Comm
		  - \ref sec_LogDB_Conf
		  - \ref sec_LogDB_REST


		\section sec_LogDB_Comm Общее описание работы LogDB
			LogDB это сервис, работа которого заключается
		в подключении к указанным лог-серверам, получении от них логов
		и сохранении их в БД (sqlite). Помимо этого LogDB выступает в качестве
		REST сервиса, позволяющего получать логи за указанный период в виде json.

		\section sec_LogDB_Conf Конфигурирвание LogDB

		<LogDB name="LogDB" ...>
			<logserver name="" ip=".." port=".." cmd=".."/>
			<logserver name="" ip=".." port=".." cmd=".."/>
			<logserver name="" ip=".." port=".." cmd=".."/>
		</LogDB>


		\section sec_LogDB_REST LogDB REST API
	*/
	class LogDB
	{
		public:
			LogDB( const std::string& name, const std::string& prefix = "" );
			virtual ~LogDB();

			/*! глобальная функция для инициализации объекта */
			static std::shared_ptr<LogDB> init_logdb( int argc, const char* const* argv, const std::string& prefix = "logdb" );

			/*! глобальная функция для вывода help-а */
			static void help_print();

			inline std::shared_ptr<DebugStream> log()
			{
				return dblog;
			}

			void run( bool async );

		protected:

			std::string myname;
			std::unique_ptr<SQLiteInterface> db;

			bool activate = { false };

			typedef std::queue<std::string> QueryBuffer;
			QueryBuffer qbuf;
			size_t qbufSize = { 200 }; // размер буфера сообщений.

			void flushBuffer();
			uniset::uniset_rwmutex mqbuf;

			std::shared_ptr<DebugStream> dblog;

			struct Log
			{
				std::string name;
				std::string ip;
				int port = { 0 };
				std::string cmd;
			};

			std::vector<Log> logservers;

		private:
	};
	// ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
