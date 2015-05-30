#ifndef DBServer_PostgreSQL_H_
#define DBServer_PostgreSQL_H_
// --------------------------------------------------------------------------
#include <map>
#include <queue>
#include "UniSetTypes.h"
#include "PostgreSQLInterface.h"
#include "DBServer.h"
//------------------------------------------------------------------------------------------
class DBServer_PostgreSQL:
	public DBServer
{
	public:
		DBServer_PostgreSQL( UniSetTypes::ObjectId id, const std::string& prefix );
		DBServer_PostgreSQL();
		virtual ~DBServer_PostgreSQL();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<DBServer_PostgreSQL> init_dbserver( int argc, const char* const* argv, const std::string& prefix = "pgsql" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		inline std::shared_ptr<LogAgregator> getLogAggregator()
		{
			return loga;
		}
		inline std::shared_ptr<DebugStream> log()
		{
			return dblog;
		}

	protected:
		typedef std::map<int, std::string> DBTableMap;

		virtual void initDBServer() override;
		virtual void initDB( std::shared_ptr<PostgreSQLInterface>& db ) {};
		virtual void initDBTableMap(DBTableMap& tblMap) {};

		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual void confirmInfo( const UniSetTypes::ConfirmMessage* cmsg ) override;
		virtual void sigterm( int signo ) override;

		bool writeToBase( const string& query );
		void createTables( std::shared_ptr<PostgreSQLInterface>& db );

		inline const char* tblName(int key)
		{
			return tblMap[key].c_str();
		}

		enum Timers
		{
			PingTimer,        /*!< таймер на переодическую проверку соединения  с сервером БД */
			ReconnectTimer,   /*!< таймер на повторную попытку соединения с сервером БД (или восстановления связи) */
			lastNumberOfTimer
		};

		std::shared_ptr<PostgreSQLInterface> db;
		int PingTime;
		int ReconnectTime;
		bool connect_ok;     /*! признак наличия соеднинения с сервером БД */

		bool activate;

		typedef std::queue<std::string> QueryBuffer;

		QueryBuffer qbuf;
		unsigned int qbufSize; // размер буфера сообщений.
		bool lastRemove;

		void flushBuffer();
		UniSetTypes::uniset_mutex mqbuf;

	private:
		DBTableMap tblMap;

};
//------------------------------------------------------------------------------------------
#endif
