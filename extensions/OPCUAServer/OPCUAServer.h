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
#include <unordered_map>
#include "open62541pp/open62541pp.h"
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "Extensions.h"
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
     <OPCUAServer name="OPCUAServer" port="4840"
         appName="uniset2 OPC UA Server"
         maxSubscriptions="10"
         maxSessions="10"
         updatePause="200"
    />
    \endcode
    К основным параметрам относятся
     - \b updatePause - мсек, период с которым обновляются значения датчиков
     - \b port - port на котором слушает OPC UA сервер

    Остальные параметры определяют детали функционирования OPC UA сервера
     https://www.open62541.org/doc/master/server.html#limits

    Часть параметров берётся из настроек текущего локального узла в секции <nodes>
    \code
     <LocalNode name="localhost"/>
     ..
     <nodes>
        <item id="3000" ip="127.0.0.1" name="node2" textname="Локальный узел" opcua_ip="0.0.0.0" .../>
        ...
     </nodes>
     \endcode
    В частности параметр \b opcua_ip - позволяет задать адрес на котором будет слушать OPC UA сервер.
    <br>По умолчанию адрес берётся из параметра \b ip.
    <br>В качестве \a browseName берётся \b name в качестве \a description - \b textname

    См. так же help \a uniset2-opcua-server -h

    \section sec_OPCUAServer_Sensors_Conf Конфигурирование списка доступных переменных (variables) для OPC UA
    Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
    Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
     - \b --xxx-filter-field  - задаёт фильтрующее поле для датчиков
     - \b --xxx-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

    Пример конфигурационных параметров для запуска с параметрами
    `--opcua-s-filter-field opcua --opcua-s-filter-value 1`
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
     - \b opcua_name - Имя OPC UA переменной, если не задано, то используется name
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

     По умолчанию все датчики доступны только на чтение.

    \section sec_OPCUAServer_Folders Структура OPC UA узла
    При запуске в дереве объектов создаётся специальный корневой объект "uniset", внутри которого регистрируются
    остальные доступные для работы объекты. В частности каждый uniset-узел регистрирует свои датчики в разделе
    \b "uniset/имя_узла/io/xxx". На рисунке представлен пример иерархии объектов

     \image html uniset-opcua-folders.png
    */
    // -----------------------------------------------------------------------------
    /*! Реализация OPCUAServer */
    class OPCUAServer:
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
            static const opcua::Type DefaultVariableType = { opcua::Type::Int32 };

            static uint8_t firstBit( DefaultValueUType mask );
            // offset = firstBit(mask)
            static DefaultValueUType getBits( DefaultValueUType value, DefaultValueUType mask, uint8_t offset );
            // if mask = 0 return value
            static DefaultValueUType setBits( DefaultValueUType value, DefaultValueUType set, DefaultValueUType mask, uint8_t offset );
            // if mask=0 return set
            static DefaultValueUType forceSetBits( DefaultValueUType value, DefaultValueUType set, DefaultValueUType mask, uint8_t offset );

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
                opcua::Type vtype = { DefaultVariableType };
                uint8_t precision = { 0 }; // only for float
            };

            std::unordered_map<ObjectId, IOVariable> variables;
            size_t writeCount = { 0 };

            struct IONode
            {
                opcua::Node<opcua::Server> node;
                IONode( const opcua::Node<opcua::Server>& n ):  node(n) {};
            };

        private:
            std::unique_ptr<opcua::Server> opcServer = { nullptr };
            std::unique_ptr<IONode> ioNode = { nullptr };
            std::string prefix;
            std::string s_field;
            std::string s_fvalue;
            uniset::timeout_t updateTime_msec = { 100 };
            std::atomic_bool firstUpdate = false;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _OPCUAServer_H_
// -----------------------------------------------------------------------------
