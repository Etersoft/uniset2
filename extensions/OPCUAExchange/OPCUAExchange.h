/*
 * Copyright (c) 2023 Pavel Vainerman.
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
#ifndef OPCUAExchange_H_
#define OPCUAExchange_H_
// -----------------------------------------------------------------------------
#include <vector>
#include <memory>
#include <deque>
#include <string>
#include "UniXML.h"
#include "ThreadCreator.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "IONotifyController.h"
#include "UniSetObject.h"
#include "Mutex.h"
#include "MessageType.h"
#include "SMInterface.h"
#include "IOBase.h"
#include "SharedMemory.h"
#include "LogServer.h"
#include "DebugStream.h"
#include "LogAgregator.h"
#include "OPCUAClient.h"
// -------------------------------------------------------------------------
#ifndef vmonit
#define vmonit( var ) vmon.add( #var, var )
#endif
// -------------------------------------------------------------------------
namespace uniset
{
    // ---------------------------------------------------------------------
    /*!
    \page page_OPCUAExchange (OPCUAExchange) Обмен с OPC UA серверами

    - \ref sec_OPCUAExchange_Comm
    - \ref sec_OPCUAExchange_Conf
    - \ref sec_OPCUAExchange_Sensors_Conf

    \section sec_OPCUAExchange_Comm Общее описание OPCUAExchange
    Данная реализация построена на использовании проекта https://github.com/open62541/open62541

    Процесс обмена выступает в качестве клиента для OPC UA серверов. Поддерживает обмен по нескольким каналам (адресам) с переключением
    между ними в случае пропадания связи. Процесс может работать напрямую с SM через указатель или как отдельно запускаемый
    процесс работающий с SM через uniset RPC (remote call procedure).

    В текущей версии на запись поддерживается только два типа переменных Boolean и Int32.
    При этом чтение поддерживаются и другие базовые знаковые и беззнаковые типы (int64,int32,int16, int8), но все они преобразуются к int32.

    \section sec_OPCUAExchange_Conf Настройка OPCUAExchange
    При старте, в конфигурационном файле ищется секция с названием объекта,
    в которой указываются настроечные параметры по умолчанию.
    Пример:
    \code
    <OPCUAExchange name="OPCUAExchange"
     addr1="opc.tcp://localhost:4840"
     addr2="opc.tcp://localhost:4841"
     timeout="1000"
    />
    \endcode
    К основным параметрам относятся
    - \b addr1, addr2 - адреса OPC серверов. Если addr2 не будет задан, обмен будет происходить только по одному каналу.
    - \b timeout - Таймаует на определение отсутствия связи и переключения на другой канал обмена (если задан)

    Если подключение к серверу защищено паролем, до задаются поля
     - \b user1, pass1 - Имя пользователя и пароль для канала N1
     - \b user2, pass2 - Имя пользователя и пароль для канала N2

     Помимо этого в конфигурационной секции поддерживаются следующие настройки
     - \b polltime - msec, Период цикла обмена
     - \b updatetime - msec, Период цикла обновления данных в SM
     - \b reconnectPause - msec, Пауза между попытками переподключения к серверу, в случае отсутсвия связи

    См. так же help \a uniset2-opcua-exchange -h

    \section sec_OPCUAExchange_Sensors_Conf Конфигурирование переменных участвающих в обмене с OPC UA
    Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
    Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
     - \b --xxx-s-filter-field  - задаёт фильтрующее поле для датчиков
     - \b --xxx-s-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

    Пример конфигурационных параметров для запуска с параметрами
    `--opcua-s-filter-field opcua --opcua-s-filter-value 1`
    \code
    <sensors name="Sensors">
     ...
     <item name="MySensor_S" textname="my sesnsor" iotype="DI" opcua="1" opcua_nodeid="AttrName1"/>
     <item name="OPCUA_Command_C" textname="opc ua command" iotype="DI" opcua="1" opcua_nodeid="ns=1;s=AttrName2" opcua_tick="2"/>
     ...
    </sensors>
    \endcode
     - \b opcua_nodeid - Адрес переменной на OPCUA сервере
     - \b opcua_tick - Как часто опрашивать датчик. Не обязательный параметр, по умолчанию - опрос на каждом цикле.
     Если задать "2" - то опрос будет производиться на каждом втором цикле и т.п. Циклы завязаны на polltime.

     Пример поддерживаемого формата для opcua_nodeid:
     - "AttrName" (aka "s=AttrName")
     - "i=13" (integer)
     - "ns=10;i=1" (integer)
     - "ns=10;s=AttrName2" (string)
     - "g=09087e75-8e5e-499b-954f-f2a9603db28a" (uuid)
     - "ns=1;b=b3BlbjYyNTQxIQ=="  (base64)

     \bs "ns" - namespace index
    */
    // ---------------------------------------------------------------------
    /*! Процесс опроса OPC UA сервера */
    class OPCUAExchange:
        public UniSetObject
    {
        public:
            OPCUAExchange( uniset::ObjectId id, uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& shm = nullptr, const std::string& prefix = "opcua" );
            virtual ~OPCUAExchange();

            static std::shared_ptr<OPCUAExchange> init_opcuaexchange(int argc, const char* const* argv,
                    uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "opcua");

            static void help_print( int argc, const char* const* argv );

            virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

            using Tick = uint8_t;

            static const size_t numChannels = 2;
            struct ReadGroup
            {
                std::vector<OPCUAClient::Result32> results;
                std::vector<UA_ReadValueId> ids;
            };
            struct WriteGroup
            {
                std::vector<UA_WriteValue> ids;
            };
            /*! Информация о входе/выходе */
            struct OPCAttribute:
                public IOBase
            {
                // т.к. IOBase содержит rwmutex с запрещённым конструктором копирования
                // приходится здесь тоже объявлять разрешенными только операции "перемещения"
                OPCAttribute(const OPCAttribute& r ) = delete;
                OPCAttribute& operator=(const OPCAttribute& r) = delete;
                OPCAttribute(OPCAttribute&& r ) = default;
                OPCAttribute& operator=(OPCAttribute&& r) = default;
                OPCAttribute() = default;

                uniset::uniset_rwmutex vmut;
                long val { 0 };
                Tick tick = { 0 }; // на каждом ли тике работать с этим аттрибутом
                std::string attrName = {""};
                struct RdValue
                {
                    std::shared_ptr<ReadGroup> gr;
                    size_t grIndex = {0};
                    int32_t get();
                    bool statusOk();
                    UA_StatusCode status();
                };
                RdValue rval[numChannels];

                struct WrValue
                {
                    std::shared_ptr<WriteGroup> gr;
                    size_t grIndex = {0};
                    bool set( int32_t val );
                    bool statusOk();
                    UA_StatusCode status();
                    const UA_WriteValue& ref();
                };
                WrValue wval[numChannels];

                friend std::ostream& operator<<(std::ostream& os, const OPCAttribute& inf );
                friend std::ostream& operator<<(std::ostream& os, const std::shared_ptr<OPCAttribute>& inf );
            };

        protected:

            enum Timers
            {
                tmUpdates
            };

            struct Channel;
            void channel1Thread();
            void channel2Thread();
            void channelThread( Channel* ch );
            bool prepare();
            void channelExchange( Tick tick, Channel* ch );
            void updateFromChannel( Channel* ch );
            void updateToChannel( Channel* ch );
            void updateFromSM();
            void writeToSM();

            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            virtual void askSensors( UniversalIO::UIOCommand cmd );
            virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
            virtual void timerInfo( const uniset::TimerMessage* sm ) override;
            virtual bool activateObject() override;
            virtual bool deactivateObject() override;

            // чтение файла конфигурации
            void readConfiguration();
            bool initIOItem( UniXML::iterator& it );
            bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );
            bool waitSM();
            bool tryConnect(Channel* ch);
            void initOutputs();

            xmlNode* confnode = { 0 }; /*!< xml-узел в настроечном файле */
            timeout_t polltime = { 150 };   /*!< периодичность обновления данных, [мсек] */
            timeout_t updatetime = { 150 };   /*!< периодичность обновления данных в SM, [мсек] */

            typedef std::vector< std::shared_ptr<OPCAttribute> > IOList;
            IOList iolist;    /*!< список входов/выходов */
            size_t maxItem = { 0 };

            struct Channel
            {
                size_t num;
                size_t idx;
                std::shared_ptr<OPCUAClient> client;
                uniset::Trigger trStatus;
                uniset::PassiveTimer ptTimeout;
                std::atomic_bool status = { true };
                std::string addr;
                std::string user;
                std::string pass;
                std::unordered_map<Tick, std::shared_ptr<ReadGroup>> readValues;
                std::unordered_map<Tick, std::shared_ptr<WriteGroup>> writeValues;
            };
            Channel channels[numChannels];
            uniset::Trigger noConnections;
            std::atomic_uint32_t currentChannel = { 0 };

            uniset::timeout_t reconnectPause = { 10000 };
            int filtersize = { 0 };
            float filterT = { 0.0 };

            std::string s_field;
            std::string s_fvalue;

            std::shared_ptr<SMInterface> shm;
            std::string prefix;
            std::string prop_prefix;

            PassiveTimer ptHeartBeat;
            uniset::ObjectId sidHeartBeat;
            int maxHeartBeat = { 10 };
            IOController::IOStateList::iterator itHeartBeat;

            bool force = { false };            /*!< флаг, означающий, что надо сохранять в SM, даже если значение не менялось */
            bool force_out = { false };        /*!< флаг, включающий принудительное чтение/запись датчиков в SM */
            timeout_t smReadyTimeout = { 15000 };    /*!< время ожидания готовности SM к работе, мсек */

            std::atomic_bool activated = { false };
            std::atomic_bool cancelled = { false };
            std::atomic_bool readconf_ok = { false };
            int activateTimeout;
            uniset::ObjectId sidTestSMReady = { uniset::DefaultObjectId };

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> opclog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            std::shared_ptr< ThreadCreator<OPCUAExchange> > thrChannel[numChannels];

            VMonitor vmon;

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // OPCUAExchange_H_
// -----------------------------------------------------------------------------
