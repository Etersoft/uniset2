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
#ifndef DBServer_PostgreSQL_H_
#define DBServer_PostgreSQL_H_
// --------------------------------------------------------------------------
#include <unordered_map>
#include <queue>
#include "UniSetTypes.h"
#include "PostgreSQLInterface.h"
#include "DBServer.h"
// -------------------------------------------------------------------------
namespace uniset
{
    //------------------------------------------------------------------------------------------
    /*!
     * \brief The DBServer_PostgreSQL class
     * Реализация работы с PostgreSQL.
     *
     * Т.к. основная работа сервера - это частая запись данных, то сделана следующая оптимизация:
     * Создаётся insert-буфер настраиваемого размера (ibufMaxSize).
     * Как только буфер заполняется, он пишется в БД одним "оптимизированным" запросом.
     * Помимо этого буфер скидывается, если прошло ibufSyncTimeout мсек или если пришёл запрос
     * на UPDATE данных.
     *
     * В случае если буфер переполняется (например нет связи с БД), то он чистится. При этом сколько
     * записей удалять определяется коэффициентом ibufOverflowCleanFactor={0...1}.
     * А также флаг lastRemove определяет удалять с конца или начала очереди.
     *
     * \warning Следует иметь ввиду, что чтобы не было постоянных "перевыделений памяти" буфер сделан на основе
     * vector и в начале работы в памяти сразу(!) резервируется место под буфер.
     * Во первых надо иметь ввиду, что буфер - это то, что потеряется если вдруг произойдёт сбой
     * по питанию или программа вылетит. Поэтому если он большой, то будет потеряно много данных.
     * И второе, т.к. это vector - то идёт выделение "непрерывного куска памяти", поэтому у ОС могут
     * быть проблемы найти "большой непрерывный кусок".
     * Тем не менее реализация сделана на vector-е чтобы избежать лишних "перевыделений" (и сегментации) памяти во время работы.
     *
     * \warning Временно, для обратной совместимости поле 'time_usec' в таблицах оставлено с таким названием,
     * хотя фактически туда сейчас сохраняется значение в наносекундах!
     */
    class DBServer_PostgreSQL:
        public DBServer
    {
        public:
            DBServer_PostgreSQL( uniset::ObjectId id, const std::string& prefix );
            DBServer_PostgreSQL();
            virtual ~DBServer_PostgreSQL();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<DBServer_PostgreSQL> init_dbserver( int argc, const char* const* argv, const std::string& prefix = "pgsql" );

            /*! глобальная функция для вывода help-а */
            static void help_print( int argc, const char* const* argv );

            inline std::shared_ptr<LogAgregator> logAggregator()
            {
                return loga;
            }
            inline std::shared_ptr<DebugStream> log()
            {
                return dblog;
            }

            bool isConnectOk() const;

        protected:
            typedef std::unordered_map<int, std::string> DBTableMap;

            virtual void initDBServer() override;
            virtual void initDB( std::unique_ptr<PostgreSQLInterface>& db ) {};
            virtual void initDBTableMap( DBTableMap& tblMap ) {};

            virtual void timerInfo( const uniset::TimerMessage* tm ) override;
            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
            virtual void confirmInfo( const uniset::ConfirmMessage* cmsg ) override;
            virtual void onTextMessage( const uniset::TextMessage* msg ) override;
            virtual bool deactivateObject() override;
            virtual std::string getMonitInfo( const std::string& params ) override;

            bool writeToBase( const std::string& query );
            void createTables( const std::shared_ptr<PostgreSQLInterface>& db );

            inline std::string tblName(int key)
            {
                return tblMap[key];
            }

            enum Timers
            {
                PingTimer,        /*!< таймер на пере одическую проверку соединения  с сервером БД */
                ReconnectTimer,   /*!< таймер на повторную попытку соединения с сервером БД (или восстановления связи) */
                FlushInsertBuffer, /*!< таймер на сброс Insert-буфера */
                lastNumberOfTimer
            };

            std::unique_ptr<PostgreSQLInterface> db;
            typedef std::queue<std::string> QueryBuffer;

            void flushBuffer();

            // writeBuffer
            const std::vector<std::string> tblcols = { "date", "time", "time_usec", "sensor_id", "value", "node" };

            typedef std::vector<PostgreSQLInterface::Record> InsertBuffer;
            void flushInsertBuffer();
            virtual void addRecord( const PostgreSQLInterface::Record&& rec );
            virtual bool writeInsertBufferToDB( const std::string& table
                                                , const std::vector<std::string>& colname
                                                , const InsertBuffer& ibuf );

        private:
            DBTableMap tblMap;

            int PingTime = { 15000 };
            int ReconnectTime = { 30000 };

            bool connect_ok = { false }; /*! признак наличия соединения с сервером БД */

            QueryBuffer qbuf;
            size_t qbufSize = { 200 }; // размер буфера сообщений.
            bool lastRemove = { false };
            std::mutex mqbuf;

            InsertBuffer ibuf;
            size_t ibufSize = { 0 };
            size_t ibufMaxSize = { 2000 };
            timeout_t ibufSyncTimeout = { 15000 };
            float ibufOverflowCleanFactor = { 0.5 }; // коэффициент {0...1} чистки буфера при переполнении
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
