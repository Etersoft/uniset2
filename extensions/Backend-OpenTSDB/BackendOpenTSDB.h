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
#include "extensions/Extensions.h"
#include "UTCPStream.h"
#include "USingleProcess.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
    \page page_BackendOpenTSDB (DBServer_OpenTSDB) Реализация шлюза к InfluxDB

      - \ref sec_OpenTSDB_Comm
      - \ref sec_OpenTSDB_Conf
      - \ref sec_OpenTSDB_Name
      - \ref sec_OpenTSDB_Queue
      - \ref sec_OpenTSDB_Query

    \section sec_OpenTSDB_Comm Общее описание шлюза к InfluxDB

    Класс реализует пересылку указанных (настроенных) датчиков в InfluxDB
    с использованием Line Protocol через TCP-соединение.

    Формат записи (InfluxDB Line Protocol):
    \code
    <measurement>,project=<project>,sensor=<name>[,tags...] value=<value> <timestamp_ms>
    \endcode

    Пример записи:
    \code
    sensors,project=theatre,sensor=FC8_Fault_S value=0 1762405075021
    sensors,project=theatre,sensor=Box1_Temperature_AS value=235 1762405075022
    \endcode

    Такой формат оптимален для InfluxDB:
    - Один measurement для всех датчиков (эффективные запросы)
    - Теги индексируются (быстрый поиск по project, sensor)
    - Timestamp в миллисекундах

    \section sec_OpenTSDB_Conf Настройка BackendOpenTSDB

    Пример секции конфигурации:
    \code
    <BackendOpenTSDB name="BackendOpenTSDB1" host="localhost" port="8089"
      filter_field="tsdb" filter_value="1"
      measurement="sensors"
      project="theatre"
      tags="host=server1,env=prod"/>
    \endcode

    Где:
    - \b host - host для связи с InfluxDB
    - \b port - port для связи с InfluxDB (TCP line protocol). Default: 4242
    - \b filter_field - поле у датчика, определяющее, что его нужно сохранять в БД
    - \b filter_value - значение \b filter_field, определяющее, что датчик нужно сохранять в БД
    - \b measurement - имя measurement в InfluxDB. Default: "sensors"
    - \b project - значение тега project (например имя подсистемы/проекта)
    - \b tags - дополнительные теги для каждой записи, через запятую (tag1=val1,tag2=val2)

    Конфигурация датчиков в секции \<sensors\>:
    \code
    <sensors>
    ...
    <item id="54" iotype="AI" name="Box1_Temperature_AS" textname="Temperature sensor" tsdb="1"/>
    <item id="55" iotype="DI" name="FC8_Fault_S" textname="Drive fault" tsdb="1" tsdb_tags="unit=drive"/>
    <item id="56" iotype="AI" name="Pressure_AS" tsdb="1" tsdb_name="MainPressure"/>
    ...
    </sensors>
    \endcode

    Где:
    - \b tsdb - поле фильтра для включения датчика в запись
    - \b tsdb_tags - дополнительные теги для конкретного датчика (через запятую)
    - \b tsdb_name - переопределение имени датчика в теге sensor

    Результат записи для примера выше:
    \code
    sensors,project=theatre,sensor=Box1_Temperature_AS,host=server1,env=prod value=235 1762405075021
    sensors,project=theatre,sensor=FC8_Fault_S,host=server1,env=prod,unit=drive value=0 1762405075022
    sensors,project=theatre,sensor=MainPressure,host=server1,env=prod value=101 1762405075023
    \endcode

    \section sec_OpenTSDB_Name Имя датчика в теге sensor
    По умолчанию в качестве имени берётся name, но при необходимости можно указать специальное имя.
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

    \section sec_OpenTSDB_Query Примеры запросов к InfluxDB

    Получение данных конкретного датчика:
    \code
    SELECT * FROM sensors WHERE sensor='FC8_Fault_S'
    SELECT time, value FROM sensors WHERE sensor='Box1_Temperature_AS' AND time > now() - 1h
    \endcode

    Фильтрация по проекту:
    \code
    SELECT * FROM sensors WHERE project='theatre'
    SELECT mean(value) FROM sensors WHERE project='theatre' GROUP BY sensor
    \endcode

    Просмотр доступных тегов:
    \code
    SHOW TAG VALUES FROM sensors WITH KEY = "sensor"
    SHOW TAG VALUES FROM sensors WITH KEY = "project"
    \endcode

    \todo Доделать возможность задать политику при переполнении буфера (удалять последние или первые, сколько чистить)
    */
    // -----------------------------------------------------------------------------
    /*! Реализация DBServer для OpenTSDB */
    class BackendOpenTSDB:
        private USingleProcess,
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

            std::string tsdbMeasurement = { "sensors" }; // measurement name for InfluxDB
            std::string tsdbProject; // project tag value (e.g. "theatre")
            std::string tsdbTags; // additional tags: TAG=VAL TAG2=VAL2 ...
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
