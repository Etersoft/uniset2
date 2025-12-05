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
#ifndef _OPCUAServer_H_
#define _OPCUAServer_H_
// -----------------------------------------------------------------------------
#include <memory>
#include <atomic>
#include <regex>
#include <optional>
#include <unordered_map>
#include "open62541pp/open62541pp.h"
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "Extensions.h"
#include "USingleProcess.h"
#include "LogServer.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
    \page page_OPCUAServer Поддержка работы с OPC-UA

      - \ref sec_OPCUAServer_Comm
      - \ref sec_OPCUAServer_Conf
      - \ref sec_OPCUAServer_Sensors_Conf
      - \ref sec_OPCUAServer_Folders

    \section sec_OPCUAServer_Comm Общее описание OPCUAServer
    Данная реализация построена на использовании проекта https://github.com/open62541/open62541
    и обёртки для с++ https://github.com/open62541pp/open62541pp

    При старте запускается OPC UA сервер, реализующий возможность получать данные по протоколу OPC UA.
    В отдельном потоке происходит обновление данных из SharedMemory (SM). По умолчанию все датчики доступны
    для чтения, но при конфигурировании процесса можно указать датчики которые будут доступны на запись
    через функции OPC UA. Процесс может работать напрямую с SM через указатель или как отдельно запускаемый
    процесс работающий с SM через uniset RPC (remote call procedure).

    \section sec_OPCUAServer_Conf Настройка OPCUAServer
    При старте, в конфигурационном файле ищется секция с названием объекта,
    в которой указываются настроечные параметры по умолчанию.
    Пример:
    \code
     <OPCUAServer name="OPCUAServer" port="4840" host="0.0.0.0"
         appName="uniset2 OPC UA Server"
         maxSubscriptions="10"
         maxSessions="10"
         updatePause="200"
         namePrefix="uniset."
    />
    \endcode
    К основным параметрам относятся
     - \b updatePause - мсек, период с которым обновляются значения датчиков
     - \b port - port на котором слушает OPC UA сервер
     - \b host - адрес на котором запускается сервер. По умолчанию 0.0.0.0

    К дополнительным:
     - \b browseName - имя видимое в списке доступных. По умолчанию name узла
     - \b description - описание узла. По умолчанию будет взято browseName
     - \b appName - По умолчанию 'Uniset2 OPC UA Server'
     - \b appUri - По умолчанию 'urn:uniset2.server'
     - \b productUri - По умолчанию 'https://github.com/Etersoft/uniset2/'
     - \b namePrefix - Префикс добавляемый к названиям переменных.
     - \b maxSessions - Максимальное количество подключений
     - \b maxSecureChannels - максимальное количество https-подключений
     - \b maxSessionTimeout - мсек, Таймаут для сессий. По умолчанию 5 сек.
     - \b maxSecurityTokenLifetime - мсек, таймаут для токена
     - \b maxSubscriptions

    Остальные параметры определяют детали функционирования OPC UA сервера
     https://www.open62541.org/doc/master/server.html#limits

    См. так же help \a uniset2-opcua-server -h

    \section sec_OPCUAServer_Sensors_Conf Конфигурирование списка доступных переменных (variables) для OPC UA
    Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
    Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
     - \b --xxx-filter-field     - задаёт фильтрующее поле для датчиков
     - \b --xxx-filter-value     - задаёт значение фильтрующего поля. Необязательный параметр.
     - \b --xxx-filter-value-re  - задаёт значение фильтрующего поля, в виде регулярного выражения. Необязательный параметр.

    Пример конфигурационных параметров для запуска с параметрами
    `--opcua-s-filter-field opcua --opcua-s-filter-value 1`

    Пример с регулярным выражением
     `--opcua-s-filter-field opcua --opcua-s-filter-value-re "1|2"`

    \code
    <sensors name="Sensors">
     ...
     <item name="MySensor_S" textname="my sesnsor" iotype="DI" opcua="1" opcua_rwmode="w"/>
     <item name="OPCUA_Command_C" textname="opc ua command" iotype="DI" opc_ua="1" opcua_rwmode="w"/>
     <item name="MySensor2_S" textname="my sesnsor" iotype="DI" opcua="1" opcua_name="Attribute2" opcua_displayname="Attr2"/>
     ...
    </sensors>
    \endcode
     - \b opcua_rwmode - Режим доступа к датчику.
       - \b w - датчик будет доступен на запись через OPC UA.
       - \b none - не предоставлять доступ к датчику (не будет виден через OPC UA)
     - \b opcua_name - Имя OPC UA переменной, если не задано, то используется name (см. так же namePrefix)
     - \b opcua_displayname - Отображаемое имя. По умолчанию берётся name
     - \b opcua_displayname_lang - Язык для отображаемого имени. По умолчанию "en".
     - \b opcua_description - Описание. По умолчанию берётся textname
     - \b opcua_description_lang - Язык описания. По умолчанию "ru"
     - \b opcua_type - Тип переменной. Поддерживаются: bool, int32, float. Если не задан, тип определяется по типу датчика.
        DI,DO - bool, AI,AO - int32
     - \b opcua_mask - "битовая маска"(uint32_t). Позволяет задать маску для значения.
        Действует как на читаемые так и на записываемые переменные.
     - \b precision - точность, для преобразования числа в целое. Используется только для типа float. Задаёт степень 10.
       precision="2" означает 10^2 = 100.  precision="3" означает 10^3 = 1000. При преобразовании AI датчиков используется округление.
     - \b opcua_method - Флаг означающий, что датчик привязан к OPCUA методу. OPCUA метод вызывается клиентом с аргументом, тип которого
        зависит от типа датчика. В данной реализации поддерживается три типа: boolean для DI, INT32 для AI и float для AI с precision.
        После вызова метода клиентом на сервере происходит выставление значения датчика вне цикла обновления SM т.е. принудительно.
        Проверка аргумента перед вызовом метода(см. open62541/server/ua_service_method.c) реализована путем сверки типов аргумента в запросе
        и в вызываемом методе. Если типы не совпадают, то вызов метода отклоняется сервером OPCUA с кодом ошибки UA_STATUSCODE_BADINVALIDARGUMENT
        и обработчик UA_setValueMethod не вызывается. Таким образом корректность типов проверяется на уровне libopen62541 перед вызовом обработчика,
        заданного при конфигурировании MethodNode в uniset2. Также в самом обработчике UA_setValueMethod тип аргумента сверяется с указанными ранее,
        а аргументы другого типа отклоняются с выставлением кода ошибки UA_STATUSCODE_BADINVALIDARGUMENT. Если же будет вызван обработчик
        с неизвестным methodID т.е. не будет совпадать ни с одним из сконфигурированных MethodNode в uniset2, то также обработка запроса отклоняется
        с кодом ошибки UA_STATUSCODE_BADMETHODINVALID.

     По умолчанию все датчики доступны только на чтение.

    \section sec_OPCUAServer_Folders Структура OPC UA узла
    При запуске в дереве объектов создаётся специальный корневой объект "uniset", внутри которого регистрируются
    остальные доступные для работы объекты. В частности каждый uniset-узел регистрирует свои датчики в конкретном
    разделе заданном в теге opcua_folder="folder1.subfolder1", например. Каталоги разделяются точкой и корневой
    считается "имя_узла", от которого начинается путь каталога для каждого датчика.
    \b "uniset/имя_узла/folder1/subfolder1/xxx". Структура каталогов задается в общей секции OPCUA сервера в подсекции
    <folders>, например:

        <OPCUAServer ...>
            <folders>
                <folder name="folder1" description="some sensors">
                    <folder name="subfolder1" description="some sensors"/>
                </folder>
                <folder name="folder2" description="some sensors">
                    <folder name="subfolder2" description="some sensors"/>
                </folder>
            </folders>
        </OPCUAServer>

    \b Если путь каталога в теге opcua_folder будет не соответсвовать каталогу в структуре(или просто ошибка в тексте), то будет
    генерироваться исключение и запуск остановится с ошибкой.

    \b На рисунке представлен пример иерархии объектов

     \image html uniset-opcua-folders.png
    */
    // -----------------------------------------------------------------------------
    /*! Реализация OPCUAServer */
    class OPCUAServer:
        private USingleProcess,
        public UObject_SK
    {
        public:
            OPCUAServer(uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID,
                        const std::shared_ptr<SharedMemory>& ic = nullptr,
                        const std::string& prefix = "opcua");

            virtual ~OPCUAServer();

            virtual CORBA::Boolean exist() override;

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<OPCUAServer> init_opcua_server(int argc, const char* const* argv,
                    uniset::ObjectId shmID,
                    const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "opcua");

            /*! глобальная функция для вывода help-а */
            static void help_print();

            using DefaultValueType = int32_t;
            using DefaultValueUType = uint32_t;
            static const opcua::DataTypeId DefaultVariableType = { opcua::DataTypeId::Int32 };

            static uint8_t firstBit( DefaultValueUType mask );
            // offset = firstBit(mask)
            static DefaultValueUType getBits( DefaultValueUType value, DefaultValueUType mask, uint8_t offset );
            // if mask = 0 return value
            static DefaultValueUType setBits( DefaultValueUType value, DefaultValueUType set, DefaultValueUType mask, uint8_t offset );
            // if mask=0 return set
            static DefaultValueUType forceSetBits( DefaultValueUType value, DefaultValueUType set, DefaultValueUType mask, uint8_t offset );

            static UA_StatusCode UA_setValueMethod(UA_Server* server, const UA_NodeId* sessionId, void* sessionHandle,
                                                   const UA_NodeId* methodId, void* methodContext, const UA_NodeId* objectId,
                                                   void* objectContext, size_t inputSize, const UA_Variant* input, size_t outputSize, UA_Variant* output);

#ifndef DISABLE_REST_API
            // HTTP API
            virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override;
            virtual Poco::JSON::Object::Ptr httpRequest( const std::string& req, const Poco::URI::QueryParameters& p ) override;
            virtual Poco::JSON::Object::Ptr httpGet( const Poco::URI::QueryParameters& p ) override;
#endif

        protected:
            OPCUAServer();

            virtual void callback() noexcept override;
            virtual void sysCommand(const uniset::SystemMessage* sm) override;
            virtual bool deactivateObject() override;
            virtual void askSensors(UniversalIO::UIOCommand cmd) override;
            virtual void sensorInfo(const uniset::SensorMessage* sm) override;
            virtual std::string getMonitInfo() const override;
            void serverLoopTerminate();
            void serverLoop();
            void updateLoop();
            void update();

#ifndef DISABLE_REST_API
            // params
            virtual Poco::JSON::Object::Ptr httpGetParam( const Poco::URI::QueryParameters& p );
            virtual Poco::JSON::Object::Ptr httpSetParam( const Poco::URI::QueryParameters& p );

            // status
            Poco::JSON::Object::Ptr httpStatus();
            Poco::JSON::Object::Ptr buildLogServerInfo();

            // Защитный флаг: разрешить/запретить /setparam (по аналогии с MBExchange/MBSlave)
            bool httpEnabledSetParams { true };
#endif
            bool initVariable(UniXML::iterator& it);
            bool readItem(const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec);
            void readConfiguration();

            std::shared_ptr<SMInterface> shm;
            std::unique_ptr<ThreadCreator<OPCUAServer>> serverThread;
            std::unique_ptr<ThreadCreator<OPCUAServer>> updateThread;

            struct IOVariable
            {
                IOVariable(const opcua::Node<opcua::Server>& n) : node(n) {};
                opcua::Node<opcua::Server> node;
                IOController::IOStateList::iterator it;
                UniversalIO::IOType stype = { UniversalIO::AO };

                uniset::uniset_rwmutex vmut;
                DefaultValueType value = { 0 };
                bool state = { false };
                DefaultValueUType mask = { 0 };
                uint8_t offset = { 0 };
                opcua::DataTypeId vtype = { DefaultVariableType };
                uint8_t precision = { 0 }; // only for float
            };

            std::unordered_map<ObjectId, IOVariable> variables;
            size_t writeCount = { 0 };

            struct IONode
            {
                opcua::Node<opcua::Server> node;
                IONode( const opcua::Node<opcua::Server>& n ):  node(n) {};
            };

            struct IOMethod
            {
                IOMethod( uniset::ObjectId _sid) : sid(_sid) {};

                IOController::IOStateList::iterator it;
                uniset::ObjectId sid = { DefaultObjectId };
                uint8_t precision = { 0 }; // only for float
            };

            std::unordered_map<uint32_t, IOMethod> methods;
            size_t methodCount = { 0 };

        private:
            std::unique_ptr<opcua::Server> opcServer = { nullptr };
            std::unique_ptr<IONode> ioNode = { nullptr };
            std::string prefix;
            std::string propPrefix;
            std::string s_field;
            std::string s_fvalue;
            std::optional<std::regex> s_fvalue_re;
            std::string namePrefix;
            uniset::timeout_t updateTime_msec = { 100 };
            std::atomic_bool firstUpdate = false;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            using folderMap  = std::unordered_map<std::string, std::unique_ptr<IONode>>;
            folderMap foldermap; // список тегов
            void initFolderMap( uniset::UniXML::iterator it, const std::string& parent_name, std::unique_ptr<IONode>& parent );
    };
    // --------------------------------------------------------------------------
    /// Get name of log level.
    constexpr std::string_view getLogLevelName(opcua::LogLevel level)
    {
        switch (level)
        {
            case opcua::LogLevel::Trace:
                return "trace";

            case opcua::LogLevel::Debug:
                return "debug";

            case opcua::LogLevel::Info:
                return "info";

            case opcua::LogLevel::Warning:
                return "warning";

            case opcua::LogLevel::Error:
                return "error";

            case opcua::LogLevel::Fatal:
                return "fatal";

            default:
                return "unknown";
        }
    }

    /// Get name of log category.
    constexpr std::string_view getLogCategoryName(opcua::LogCategory category)
    {
        switch (category)
        {
            case opcua::LogCategory::Network:
                return "network";

            case opcua::LogCategory::SecureChannel:
                return "channel";

            case opcua::LogCategory::Session:
                return "session";

            case opcua::LogCategory::Server:
                return "server";

            case opcua::LogCategory::Client:
                return "client";

            case opcua::LogCategory::Userland:
                return "userland";

            case opcua::LogCategory::SecurityPolicy:
                return "securitypolicy";

            default:
                return "unknown";
        }
    }
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _OPCUAServer_H_
// -----------------------------------------------------------------------------
