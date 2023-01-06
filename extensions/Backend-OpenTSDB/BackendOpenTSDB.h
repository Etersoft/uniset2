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
// -----------------------------------------------------------------------------
#ifndef _BackendOpenTSDB_H_
#define _BackendOpenTSDB_H_
// -----------------------------------------------------------------------------
#include <deque>
#include <memory>
#include <unordered_map>
#include <chrono>
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "UTCPStream.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
    \page page_BackendOpenTSDB (DBServer_OpenTSDB) Реализация шлюза к БД поддерживающей интерфейс OpenTSDB

      - \ref sec_OpenTSDB_Comm
      - \ref sec_OpenTSDB_Conf
      - \ref sec_OpenTSDB_Name
      - \ref sec_OpenTSDB_Queue

    \section sec_OpenTSDB_Comm Общее описание шлюза к OpenTSDB

    "OpenTSDB" - time series database. Специальная БД оптимизированная
    для хранения временных рядов (по простому: данных с временными метками).
    Класс реализует пересылку указанных (настроенных) датчиков в БД поддерживающую
    интерфейс совместимый с OpenTSDB. В текущей реализации используется посылка
    строк в формате Telnet
    \code
    put <metric> <timestamp>.msec <value> <tagk1=tagv1[ tagk2=tagv2 ...tagkN=tagvN]>
    \endcode
     См. http://opentsdb.net/docs/build/html/user_guide/writing/index.html

    \section sec_OpenTSDB_Conf Настройка BackendOpenTSDB

    Пример секции конфигурации:
    \code
    <BackendOpenTSDB name="BackendOpenTSDB1" host="localhost" port="4242"
      filter_field="tsdb" filter_value="1"
      prefix="uniset"
      tags="TAG1=VAL1 TAG2=VAL2 ..."/>

    \endcode
    Где:
    - \b host - host для связи с TSDB
    - \b port - port для связи с TSDB. Default: 4242
    - \b filter_field - поле у датчика, определяющее, что его нужно сохранять в БД
    - \b filter_value - значение \b filter_field, определяющее, что датчик нужно сохранять в БД
    - \b prefix - необязательный префикс дописываемый ко всем параметрам (prefix.parameter)
    - \b tags - теги которые будут записаны для каждой записи, перечисляемые через пробел.

    При этом в секции <sensors> у датчиков можно задать дополнительные теги.
    Помимо этого можно переопределить название метрики (tsdb_name="...").
    \code
    <sensors>
    ...
    <item id="54" iotype="AI" name="AI54_S" textname="AI sensor 54" tsdb="1" tsdb_tags=""/>
    <item id="55" iotype="AI" name="AI55_S" textname="AI sensor 55" tsdb="1" tsdb_tags="" tsdb_name="MySpecName"/>
    ...
    </sensors>
    \endcode

    \section sec_OpenTSDB_Name Имя значения сохраняемое в БД.
    По умолчанию в качестве имени берётся name, но при необходимости можно указать определиться специальное имя.
    Для этого достаточно задать поле tsdb_name="...".

    \section sec_OpenTSDB_Queue Буфер на запись в БД
    В данной реализации встроен специальный буфер, который накапливает данные и скидывает их
    пачкой в БД. Так же он является защитным механизмом на случай если БД временно недоступна.
    Параметры буфера задаются аргументами командной строки или в конфигурационном файле.
    Доступны следующие параметры:
    - \b bufSize - размер буфера, при заполнении которого происходит посылка данных в БД
    - \b bufMaxSize - максимальный размер буфера, при котором начинают откидываться новые сообщения (потеря сообщений)
    - \b bufSyncTimeout - период сброса данных в БД
    - \b reconnectTime - время на повторную попытку подключения к БД
    - \b sizeOfMessageQueue - Размер очереди сообщений для обработки изменений по датчикам.
     При большом количестве отслеживаемых датчиков, размер должен быть достаточным, чтобы не терять изменения.

    \todo Нужна ли поддержка авторизации для TSDB (возможно придётся перейти на HTTP REST API)
    \todo Доделать возможность задать политику при переполнении буфера (удалять последние или первые, сколько чистить)
    */
    // -----------------------------------------------------------------------------
    /*! Реализация DBServer для OpenTSDB */
    class BackendOpenTSDB:
        public UObject_SK
    {
        public:
            BackendOpenTSDB( uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                             const std::string& prefix = "opentsdb" );
            virtual ~BackendOpenTSDB();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<BackendOpenTSDB> init_opendtsdb( int argc, const char* const* argv,
                    uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "opentsdb" );

            /*! глобальная функция для вывода help-а */
            static void help_print( int argc, const char* const* argv );

            inline std::shared_ptr<LogAgregator> getLogAggregator()
            {
                return loga;
            }
            inline std::shared_ptr<DebugStream> log()
            {
                return mylog;
            }

            enum Timers
            {
                tmFlushBuffer,
                tmReconnect,
                tmLastNumberOfTimer
            };

        protected:
            BackendOpenTSDB();

            // переопределяем callback, чтобы оптимизировать
            // обработку большого количества сообщений
            // и убрать не нужную в данном процессе обработку (включая sleep_msec)
            virtual void callback() noexcept override;

            virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
            virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
            virtual void timerInfo( const uniset::TimerMessage* tm ) override;
            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            virtual std::string getMonitInfo() const override;

            void init( xmlNode* cnode );
            bool flushBuffer();
            bool reconnect();

            std::shared_ptr<SMInterface> shm;

            struct ParamInfo
            {
                const std::string name;
                const std::string tags;

                ParamInfo( const std::string& _name, const std::string& _tags ):
                    name(_name), tags(_tags) {}
            };

            std::string tsdbPrefix;
            std::string tsdbTags; // теги в виде строки TAG=VAL TAG2=VAL2 ...
            std::unordered_map<uniset::ObjectId, ParamInfo> tsdbParams;

            timeout_t bufSyncTime = { 5000 };
            size_t bufSize = { 500 };
            size_t bufMaxSize = { 5000 }; // drop messages
            bool timerIsOn = { false };
            timeout_t reconnectTime = { 5000 };
            std::string lastError;

            // буфер mutex-ом можно не защищать
            // т.к. к нему идёт обращение только из основного потока обработки
            // (sensorInfo, timerInfo)
            std::deque<std::string> buf;

            // работа с OpenTSDB
            std::shared_ptr<UTCPStream> tcp;
            std::string host = { "localhost" };
            int port = { 4242 };

        private:

            std::string prefix;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _BackendOpenTSDB_H_
// -----------------------------------------------------------------------------
