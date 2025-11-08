/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#ifndef _BackendClickHouse_H_
#define _BackendClickHouse_H_
// -----------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include <chrono>
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "extensions/Extensions.h"
#include "ClickHouseInterface.h"
#include "ClickHouseTagsConfig.h"
#include "USingleProcess.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
    \page page_ClickHouse BackendClickHouse: Реализация работы с ClickHouse.
     - \ref sec_ClickHouse_Conf
     - \ref sec_ClickHouse_Queue
     - \ref sec_ClickHouse_Tags
     - \ref sec_ClickHouse_Admin

    \section sec_ClickHouse_Conf Настройка BackendClickHouse

        Пример секции конфигурации:
    \code
            <BackendClickHouse name="BackendClickHouse1" dbhost="localhost" dbport="9000" dbuser="" dbpass=""
                      filter_field="clickhouse" filter_value="1"
                      tags="TAG1=VAL1 TAG2=VAL2 ..."/>

    \endcode
        Где:
        - \b dbhost - host для связи с ClickHouse
        - \b dbport - port для связи с ClickHouse. Default: 9000
        - \b dbuser - user
        - \b dbpass - password
        - \b dbtablename - Имя таблицы в которую писать данные. По умолчанию main_history.
        - \b filter_field - поле у датчика, определяющее, что его нужно сохранять в БД
        - \b filter_value - значение \b filter_field, определяющее, что датчик нужно сохранять в БД
        - \b tags - теги которые будут добавлены для каждой записи (перечисляются через пробел)

    Так же статические теги можно указывать непосредственно у датчиков
    \code
        <sensors>
            ...
            <item id="54" iotype="AI" name="AI54_S" textname="AI sensor 54" clickhouse="1" clickhouse_tags=""/>
            <item id="55" iotype="AI" name="AI55_S" textname="AI sensor 55" clickhouse="1" clickhouse_tags="tag1=val1 tag2=val2"/>
            ...
        </sensors>
    \endcode

    \section sec_ClickHouse_Queue Буфер на запись в БД
    В данной реализации встроен специальный буфер, который накапливает данные и скидывает их
    пачкой в БД. Так же он является защитным механизмом на случай если БД временно недоступна.
    Параметры буфера задаются аргументами командной строки или в конфигурационном файле.
    Доступны следующие параметры:
    - \b bufSize - размер буфера, при заполнении которого происходит запись данных в БД
    - \b bufMaxSize - максимальный размер буфера, при котором все данные теряются (буфер чиститься)
    - \b bufSyncTimeout - период принудительного сброса данных в БД, миллисек.
    - \b reconnectTime - время на повторную попытку подключения к БД, миллисек.
    - \b sizeOfMessageQueue - Размер очереди сообщений для обработки изменений по датчикам.
         При большом количестве отслеживаемых датчиков, размер должен быть достаточным, чтобы не терять изменения.

    \section sec_ClickHouse_Tags Настройка динамических тегов
    Значения тегов настраиваются в секции <clickhouse_tags>
    \code
            <BackendClickHouse ...>
                <clickhouse_tags>
                   ....
                </clickhouse_tags>
            </BackendClickHouse>
    \endcode
    см. также \ref page_ClickHouseTags

    \section sec_ClickHouse_Admin Впомогательная утилита uniset2-clickhouse-admin
     В состав проекта так же входит утилита uniset2-clickhouse-admin облегчающая взаимодействие с ClickHouse.
     В частности утилита позволяет сгенерировать csv-файлы словарей для подключения к проекту.

     \code
      Usage: uniset2-clickhouse-admin [command] config.xml

      uniset2-clickhouse-admin - generate data for clickhouse server

      Valid command are:
        -h, --help            - display help
        -d, --dicts           - Generate dictionaries data (csv files) for Clickhouse
        -c, --create [dbname] - Create database. Default dbname=uniset
        -s, --size            - Show table sizes
        --outdir dir          - write files to directory
     \endcode
     В проект входят пример файла для настройки и подключения csv-словарей к ClickHouse.
     В результате в БД будут доступны словари `dict_sensors`, `dict_objects`, `dict_nodes` позволяющие использовать имена датчиков
     и другие характеристики в запросах.
     Пример простого запроса выводящего историю изменения датчиков за последний час
     \code
     SELECT
        timestamp,
        value,
        dictGetString('dict_sensors', 'name', name_hid),
        dictGetString('dict_nodes', 'ip', node_hid)
     FROM main_history
     WHERE timestamp > now() - toIntervalHour(1)
     \endcode
     Или по конкретному датчику
     \code
     SELECT
         timestamp,
         value,
         dictGetString('dict_sensors', 'name', name_hid),
         dictGetString('dict_nodes', 'ip', node_hid)
     FROM main_history
     WHERE timestamp > now() - toIntervalHour(30) and name_hid = cityHash64('Sensor1_AS')
     \endcode

     Пример запрос с использованием тегов
     \code
     SELECT
        timestamp,
        value,
        tag,
        tagvalue
     FROM main_history
     ARRAY JOIN
        tags.name AS tag,
        tags.value AS tagvalue
     WHERE (tag = 'system') AND (tagvalue = 'temp')
     \endcode

     Описание доступных в словарях полей можно посмотреть в файле `clickhouse_uniset_dictionary.xml`
    */
    //------------------------------------------------------------------------------------------
    /*! Реализация DBServer для Clickhouse */
    class BackendClickHouse:
        private USingleProcess,
        public UObject_SK
    {
        public:
            BackendClickHouse( uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                               const std::string& prefix = "clickhouse" );
            virtual ~BackendClickHouse();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<BackendClickHouse> init_clickhouse( int argc, const char* const* argv,
                    uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "clickhouse" );

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
            BackendClickHouse();

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

            using Tag = std::pair<std::string, std::string>;
            using TagList = std::vector<Tag>;

            struct ParamInfo
            {
                const std::string name;
                TagList tags;

                ParamInfo( const std::string& _name, const TagList& _tags ):
                    name(_name), tags(_tags) {}
            };

            std::unordered_map<uniset::ObjectId, ParamInfo> clickhouseParams;
            TagList globalTags;

            timeout_t bufSyncTime = { 5000 };
            size_t bufSize = { 5000 };
            size_t bufMaxSize = { 100000 }; // drop messages
            bool timerIsOn = { false };
            timeout_t reconnectTime = { 5000 };
            std::string lastError;

            // работа с ClickHouse
            std::shared_ptr<clickhouse::ColumnDateTime64> colTimeStamp;
            std::shared_ptr<clickhouse::ColumnFloat64> colValue;
            std::shared_ptr<clickhouse::ColumnString> colName;
            std::shared_ptr<clickhouse::ColumnString> colNodeName;
            std::shared_ptr<clickhouse::ColumnString> colProducer;
            std::shared_ptr<clickhouse::ColumnArray> arrTagKeys;
            std::shared_ptr<clickhouse::ColumnArray> arrTagValues;

            void createColumns();
            void clearData();
            static TagList parseTags( const std::string& tags );

            std::unique_ptr<uniset::ClickHouseTagsConfig> dyntags;

            std::string fullTableName;
            std::unique_ptr<ClickHouseInterface> db;

            std::string dbhost;
            int dbport;
            std::string dbuser;
            std::string dbpass;
            std::string dbname;

        private:
            std::string prefix;
            bool connect_ok = { false };
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _BackendClickHouse_H_
// -----------------------------------------------------------------------------
