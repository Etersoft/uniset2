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
#include <regex>
#include <optional>
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
    - \ref sec_OPCUAExchange_ExchangeMode

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
    - \b timeout - Таймаут на определение отсутствия связи и переключения на другой канал обмена (если задан)
    - \b sidRespond - датчик связи для данного канала

    Если подключение к серверу защищено паролем, до задаются поля
     - \b user1, pass1 - Имя пользователя и пароль для канала N1
     - \b user2, pass2 - Имя пользователя и пароль для канала N2

     Помимо этого в конфигурационной секции поддерживаются следующие настройки
     - \b polltime - msec, Период цикла обмена
     - \b updatetime - msec, Период цикла обновления данных в SM
     - \b reconnectPause - msec, Пауза между попытками переподключения к серверу, в случае отсутсвия связи
     - \b writeToAllChannels [0,1] - Включение режима записи по всем каналам. По умолчанию датчики пишутся только в активном канале обмена.

    См. так же help \a uniset2-opcua-exchange -h

    \section sec_OPCUAExchange_Sensors_Conf Конфигурирование переменных участвующих в обмене с OPC UA
    Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
    Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
     - \b --xxx-filter-field  - задаёт фильтрующее поле для датчиков
     - \b --xxx-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

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
     - \b opcua_mask - "битовая маска"(uint32_t). Позволяет задать маску для значения. Действует как на значения читаемые,
     так и записываемые. При этом разрешается привязывать разные датчики к одной и той же переменной указывая разные маски.
     - \b opcua_type - типа переменной в ПЛК.
     Поддерживаются следующие типы: bool, byte, int16, uint16, int32, uint32, int64, uint64, float, double(as float)
     При этом происходит преобразование значения int32_t к указанному типу (с игнорированием переполнения!).
     Для float,double при преобразовании учитывается поле precision.

     Пример поддерживаемого формата для opcua_nodeid:
     - "AttrName" (aka "s=AttrName")
     - "i=13" (integer)
     - "ns=10;i=1" (integer)
     - "ns=10;s=AttrName2" (string)
     - "g=09087e75-8e5e-499b-954f-f2a9603db28a" (uuid)
     - "ns=1;b=b3BlbjYyNTQxIQ=="  (base64)

     \b "ns" - namespace index

     \section sec_OPCUAExchange_ExchangeMode Управление режимом работы OPCUAExchange
     В OPCUAExchange заложена возможность управлять режимом работы процесса. Поддерживаются
     следующие режимы:
     - \b emNone - нормальная работа (по умолчанию)
     - \b emWriteOnly - "только посылка данных" (работают только write-функции)
     - \b emReadOnly - "только чтение" (работают только read-функции)
     - \b emSkipSaveToSM - "не записывать данные в SM", это особый режим, похожий на \b emWriteOnly,
     но отличие в том, что при этом режиме ведётся полноценый обмен (и read и write),
     только реально данные не записываются в SharedMemory(SM).
     - \b emSkipExchnage - отключить обмен (при этом данные "из SM" обновляются).

     Режимы переключаются при помощи датчика, который можно задать либо аргументом командной строки
     - \b --prefix-exchange-mode-id либо в конф. файле параметром \b exchangeModeID="". Константы определяющие режимы объявлены в MBTCPMaster::ExchangeMode.
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

            static uint8_t firstBit( uint32_t mask );

            // offset = firstBit(mask)
            static uint32_t getBits( uint32_t value, uint32_t mask, uint8_t offset );
            // if mask = 0 return value
            static uint32_t setBits( uint32_t value, uint32_t set, uint32_t mask, uint8_t offset );
            // if mask=0 return set
            static uint32_t forceSetBits( uint32_t value, uint32_t set, uint32_t mask, uint8_t offset );

            using Tick = uint8_t;

            static const size_t numChannels = 2;
            struct ReadGroup
            {
                std::vector<OPCUAClient::ResultVar> results;
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
                int32_t val { 0 };
                Tick tick = { 0 }; // на каждом ли тике работать с этим аттрибутом
                uint32_t mask = { 0 };
                uint8_t offset = { 0 };
                OPCUAClient::VarType vtype = { OPCUAClient::VarType::Int32 };

                // with precision
                float as_float();

                // with precision/noprecision
                int32_t set( float val );

                std::string attrName = {""};
                struct RdValue
                {
                    std::shared_ptr<ReadGroup> gr;
                    size_t grIndex = {0};
                    int32_t get();
                    float getF();
                    bool statusOk();
                    UA_StatusCode status();
                };
                RdValue rval[numChannels];

                struct WrValue
                {
                    std::shared_ptr<WriteGroup> gr;
                    size_t grIndex = {0};
                    bool set( int32_t val );
                    bool setF( float val );
                    bool statusOk();
                    UA_StatusCode status();
                    const UA_WriteValue& ref();
                    static void init( UA_WriteValue* wval, const std::string& nodeId, const std::string& type, int32_t defvalue );
                };
                WrValue wval[numChannels];

                friend std::ostream& operator<<(std::ostream& os, const OPCAttribute& inf );
                friend std::ostream& operator<<(std::ostream& os, const std::shared_ptr<OPCAttribute>& inf );
            };

            /*! Режимы работы процесса обмена */
            enum ExchangeMode
            {
                emNone = 0,         /*!< нормальная работа (по умолчанию) */
                emWriteOnly = 1,    /*!< "только посылка данных" (работают только write-функции) */
                emReadOnly = 2,     /*!< "только чтение" (работают только read-функции) */
                emSkipSaveToSM = 3, /*!< не писать данные в SM (при этом работают и read и write функции) */
                emSkipExchange = 4, /*!< отключить обмен */
                emLastNumber
            };

            typedef std::list<IOBase> ThresholdList;
            // т.к. пороговые датчики не связаны напрямую с обменом, создаём для них отдельный список
            // и отдельно его проверяем потом
            ThresholdList thrlist;

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
            void channelExchange( Tick tick, Channel* ch, bool writeOn );
            void updateFromChannel( Channel* ch );
            void updateToChannel( Channel* ch );
            void updateFromSM();
            void writeToSM();
            bool isUpdateSM( bool wrFunc ) const noexcept;

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
            timeout_t polltime = { 100 };   /*!< периодичность обновления данных, [мсек] */
            timeout_t updatetime = { 100 };   /*!< периодичность обновления данных в SM, [мсек] */

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
                std::atomic_bool status = { false };
                std::string addr;
                std::string user;
                std::string pass;
                std::unordered_map<Tick, std::shared_ptr<ReadGroup>> readValues;
                std::unordered_map<Tick, std::shared_ptr<WriteGroup>> writeValues;
                uniset::ObjectId respond_s = { uniset::DefaultObjectId };
                IOController::IOStateList::iterator respond_it;
            };
            Channel channels[numChannels];
            uniset::Trigger noConnections;
            std::atomic_uint32_t currentChannel = { 0 };

            uniset::timeout_t reconnectPause = { 10000 };
            int filtersize = { 0 };
            float filterT = { 0.0 };

            std::string s_field;
            std::string s_fvalue;
            std::optional<std::regex> s_fvalue_re;

            std::shared_ptr<SMInterface> shm;
            std::string prefix;
            std::string prop_prefix;

            PassiveTimer ptHeartBeat;
            uniset::ObjectId sidHeartBeat;
            int maxHeartBeat = { 10 };
            IOController::IOStateList::iterator itHeartBeat;

            bool force = { false };            /*!< флаг, означающий, что надо сохранять в SM, даже если значение не менялось */
            bool force_out = { false };        /*!< флаг, включающий принудительное чтение/запись датчиков в SM */
            bool writeToAllChannels = { false }; /*!< флаг, включающий запись во все каналы, по умолчанию только в активный */
            timeout_t smReadyTimeout = { 15000 };    /*!< время ожидания готовности SM к работе, мсек */

            std::atomic_bool activated = { false };
            std::atomic_bool cancelled = { false };
            std::atomic_bool readconf_ok = { false };
            int activateTimeout;
            uniset::ObjectId sidTestSMReady = { uniset::DefaultObjectId };
            uniset::ObjectId sidRespond = {uniset::DefaultObjectId };
            IOController::IOStateList::iterator itRespond;

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> opclog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            std::shared_ptr< ThreadCreator<OPCUAExchange> > thrChannel[numChannels];

            uniset::ObjectId sidExchangeMode = { uniset::DefaultObjectId }; /*!< идентификатор для датчика режима работы */
            IOController::IOStateList::iterator itExchangeMode;
            long exchangeMode = { emNone }; /*!< режим работы см. ExchangeMode */

            VMonitor vmon;

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // OPCUAExchange_H_
// -----------------------------------------------------------------------------
