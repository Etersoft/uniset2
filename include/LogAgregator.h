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
#ifndef LogAgregator_H_
#define LogAgregator_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <regex>
#include <list>
#include <vector>
#include <unordered_map>
#include "DebugStream.h"
#include "LogServerTypes.h"
// -------------------------------------------------------------------------
namespace uniset
{
	/*!
		\page page_LogAgregator Агрегатор логов (LogAgregator)

		- \ref sec_LogA_Comm
		- \ref sec_LogA_Hierarchy
		- \ref sec_LogA_Regexp

		\section sec_LogA_Comm Общее описание
		LogAgregator это класс предназначенный для объединения нескольких DebugStream
		в один поток. При этом LogAgregator сам является DebugStream и обладает всеми его свойствами.

		LogAgregator позволяет управлять каждым из своих логов в отдельности по имени лога.
		\code
		std::shared_ptr<DebugStream> dlog1 = std::make_shared<DebugStream>();
		std::shared_ptr<DebugStream> dlog2 = std::make_shared<DebugStream>();

		std::shared_ptr<DebugStream> la1 = make_shared<LogAgregator>("la1");
		la1->add(dlog1,"dlog1");
		la1->add(dlog1,"dlog2");

		// Работа с логами через агрегатор
		la1->addLevel("dlog1",Debug::INFO);
		\endcode

		Помимо этого при помощи агрегатора можно сразу создавать дочерние логи
		\code
		auto log10 = la1->create("dlog10");
		log10->warn() << "WARNING MESSAGE" << endl;
		\endcode

		\section sec_LogA_Hierarchy Иерархия агрегаторов

		Агрегатор позволяет строить иерархии из агрегаторов.
		При добавлении дочернего агрегатора, все обращения к подчинённым логам другого агрегатора происходят
		по имени включающим в себя имя дочернего агрегатора.
		Пример:
		\code
		// Где-то в программе есть DebugStream (логи)
		std::shared_ptr<DebugStream> dlog1 = std::make_shared<DebugStream>();
		std::shared_ptr<DebugStream> dlog2 = std::make_shared<DebugStream>();
		std::shared_ptr<DebugStream> dlog3 = std::make_shared<DebugStream>();
		std::shared_ptr<DebugStream> dlog4 = std::make_shared<DebugStream>();

		std::shared_ptr<LogAgregator> la1 = make_shared<LogAgregator>("la1");
		la1->add(dlog1,"dlog1");
		la1->add(dlog1,"dlog2");

		// Работа с логами через агрегатор
		la1->addLevel("dlog1",Debug::INFO);
		..
		// При этом можно выводить и напрямую в агрегатор
		la1->info() << "INFO MESSAGE" << endl;
		...
		std::shared_ptr<LogAgregator> la2 = make_shared<LogAgregator>("la2");
		la2->add(dlog1,"dlog3");
		la2->add(dlog1,"dlog4");
		// работа напрямую...
		la2->addLevel("dlog4",Debug::WARN);

		// Добавление второго агрегатора в первый..
		la1->add(la2);
		...
		// теперь для обращения к логам второго агрегатора через первый..
		// нужно добавлять его имя и разделитель "/" (LogAgregator::sep)
		la1->addLevel("la2/dlog4",Debug::CRIT);
		\endcode

		\note Все эти свойства в полной мере используются при управлении логами через LogServer.
		В обычной "жизни" агрегатор вряд ли особо нужен.


		\section sec_LogA_Regexp Управление логами с использованием регулярных выражений
		Агрегатор позволяет получать список подчинённых ему потоков (DebugStream) по именам удовлетворяющим
		заданному регулярному выражению. Пример:
		Предположим что иерархия подчинённых потоков выглядит следующим образом
		\code
		log1
		log2
		log3
		loga2/log1
		loga2/log2
		loga2/loga3/log1
		loga2/loga3/log2
		loga2/loga3/log3
		...
		\endode
		Для управления логами можно получить например список всех подчинённых потоков для loga3
		\code
		auto lst = la->getLogList(".*loga3.*");
		for( auto&& l: lst )
		{
			..что-то делать..
		}
		\endcode
		\note Полнота и формат поддерживаемых регулярных выражений зависит от поддержки компилятором стандарта с++11 (класс <regex>).
	*/
	// -------------------------------------------------------------------------
	/* Т.к. в других агрегаторах может тоже встречаться такие же логи, приходится отдельно вести
	 * учёт подключений (conmap) и подключаться к потокам напрямую, а не к агрегатору
	 * иначе будет происходить дублирование информации на экране (логи смешиваются от разных агрегаторов)
	*/
	class LogAgregator:
		public DebugStream
	{
		public:

			static const std::string sep; /*< разделитель для имён подчинённых агрегаторов ('/') */

			explicit LogAgregator( const std::string& name, Debug::type t );
			explicit LogAgregator( const std::string& name = "" );

			virtual ~LogAgregator();

			virtual void logFile( const std::string& f, bool truncate = false ) override;

			void add( std::shared_ptr<LogAgregator> log, const std::string& lname = "" );
			void add( std::shared_ptr<DebugStream> log, const std::string& lname = "" );

			std::shared_ptr<DebugStream> create( const std::string& logname );

			// Управление "подчинёнными" логами
			void addLevel( const std::string& logname, Debug::type t );
			void delLevel( const std::string& logname, Debug::type t );
			void level( const std::string& logname, Debug::type t );
			void offLogFile( const std::string& logname );
			void onLogFile( const std::string& logname );

			// найти лог.. (по полному составному имени)
			std::shared_ptr<DebugStream> getLog( const std::string& logname );
			bool logExist( std::shared_ptr<DebugStream>& l ) const;
			std::shared_ptr<DebugStream> findByLogName( const std::string& logname ) const;
			// -------------------------------------------------------------------------

			struct iLog
			{
				iLog( const std::shared_ptr<DebugStream>& l, const std::string& nm ): log(l), name(nm) {}
				std::shared_ptr<DebugStream> log;
				std::string name;

				// для сортировки "по алфавиту"
				inline bool operator < ( const iLog& r ) const
				{
					return name < r.name;
				}
			};

			std::list<iLog> getLogList() const;
			std::list<iLog> getLogList( const std::string& regexp_str ) const;

			friend std::ostream& operator<<(std::ostream& os, LogAgregator& la );
			friend std::ostream& operator<<(std::ostream& os, std::shared_ptr<LogAgregator> la );

			static std::vector<std::string> splitFirst( const std::string& lname, const std::string s = "/" );

			std::ostream& printLogList( std::ostream& os, const std::string& regexp_str = "" ) const;
			static std::ostream& printLogList( std::ostream& os, std::list<iLog>& lst );

		protected:
			void logOnEvent( const std::string& s );
			void addLog( std::shared_ptr<DebugStream> l, const std::string& lname, bool connect );
			void addLogAgregator( std::shared_ptr<LogAgregator> la, const std::string& lname );

			// поиск лога по составному логу.."agregator/agregator2/.../logname"
			std::shared_ptr<DebugStream> findLog( const std::string& lname ) const;

			// вывод в виде "дерева"
			std::ostream& printTree(std::ostream& os, const std::string& g_tab = "") const;

			// получить список с именами (длинными) и с указателями на логи
			std::list<iLog> makeLogNameList( const std::string& prefix ) const;

			// получить список по именам удовлетворяющим регулярному выражению (рекурсивная функция)
			void getListByLogNameWithRule( std::list<iLog>& lst, const std::regex& rule, const std::string& prefix ) const;

		private:
			typedef std::unordered_map<std::string, std::shared_ptr<DebugStream>> LogMap;
			LogMap lmap;

			typedef std::unordered_map<std::shared_ptr<DebugStream>, sigc::connection> ConnectionMap;
			ConnectionMap conmap;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // LogAgregator_H_
// -------------------------------------------------------------------------
