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
#ifndef _OPCUAGate_H_
#define _OPCUAGate_H_
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
    \page page_OPCUAGate Поддержка работы с OPC-UA

      - \ref sec_OPCUAGate_Comm
      - \ref sec_OPCUAGate_Conf
      - \ref sec_OPCUAGate_Sesnros_Conf

    \section sec_OPCUAGate_Comm Общее описание OPCUAGate
    Данная реализация построена на использовании проекта https://github.com/open62541/open62541
    и обёртки для с++ https://github.com/open62541pp/open62541pp

    При старте запускается OPC UA сервер, реализующий возможность получать данные по протоколу OPC UA.
    В отдельном потоке происходит обновление данных из SharedMemory (SM). По умолчанию все датчики доступны
    для чтения, но при конфигурировании процесса можно указать датчики которые будут доступны на запись
    через функции OPC UA. Процесс может работать напрямую с SM через указатель или как отдельно запускаемый
    процесс работающий с SM через uniset RPC (remote call procedure).

    \section sec_OPCUAGate_Conf Настройка OPCUAGate
    При старте, в конфигурационном файле ищется секция с названием объекта,
    в которой указываются настроечные параметры по умолчанию.
    Пример:
    \code
     <OPCUAGate name="OPCUAGate" port="4840"
         appName="uniset2 OPC UA gate"
         shutdownDelay="5000"
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

    См. так же help \a uniset2-opcuagate -h

    \section sec_OPCUAGate_Sesnros_Conf Конфигурирование списка доступных переменных (variables) для OPC UA
    Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
    Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
     - \b --xxx-s-filter-field  - задаёт фильтрующее поле для датчиков
     - \b --xxx-s-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

    Пример конфигурационных параметров для запуска с параметрами
    `--opcua-s-filter-field opcua --opcua-s-filter-value 1`
    \code
    <sensors name="Sensors">
     ...
     <item name="MySensor_S" textname="my sesnsor" iotype="DI" opc_ua="1" opcua_rwmode="w"/>
     <item name="OPCUA_Command_C" textname="opc ua command" iotype="DI" opc_ua="1" opcua_rwmode="w"/>
     ...
    </sensors>
    \endcode
     - \b opcua_rwmode - Режим доступа к датчику.
       - \b w - датчик будет доступен на запись через OPC UA.
       - \b none - не предоставлять доступ к датчику (не будет виден через OPC UA)

     По умолчанию все датчики доступны только на чтение.
    */
    // -----------------------------------------------------------------------------
    /*! Реализация OPCUAGate */
    class OPCUAGate:
        public UObject_SK
    {
        public:
            OPCUAGate(uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID,
                      const std::shared_ptr<SharedMemory>& ic = nullptr,
                      const std::string& prefix = "opcua");

            virtual ~OPCUAGate();

            virtual CORBA::Boolean exist() override;

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<OPCUAGate> init_opcuagate(int argc, const char* const* argv,
                    uniset::ObjectId shmID,
                    const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "opcua");

            /*! глобальная функция для вывода help-а */
            static void help_print();

            inline std::shared_ptr<LogAgregator> getLogAggregator()
            {
                return loga;
            }

            inline std::shared_ptr<DebugStream> log()
            {
                return mylog;
            }

        protected:
            OPCUAGate();

            virtual void step() override;
            virtual void sysCommand(const uniset::SystemMessage* sm) override;
            virtual bool deactivateObject() override;
            virtual void askSensors(UniversalIO::UIOCommand cmd) override;
            virtual void sensorInfo(const uniset::SensorMessage* sm) override;
            void serverLoopTerminate();
            void serverLoop();
            void updateLoop();
            void update();

            bool initVariable(UniXML::iterator& it);
            bool readItem(const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec);
            void readConfiguration();

            std::shared_ptr<SMInterface> shm;
            std::unique_ptr<ThreadCreator<OPCUAGate>> serverThread;
            std::unique_ptr<ThreadCreator<OPCUAGate>> updateThread;

            struct IOVariable
            {
                opcua::VariableNode node;
                IOController::IOStateList::iterator it;
                UniversalIO::IOType stype = {UniversalIO::AI};
                IOVariable(const opcua::VariableNode& n) : node(n) {};
            };

            std::unordered_map<ObjectId, IOVariable> variables;

            struct IONode
            {
                opcua::ObjectNode node;
                IONode( const opcua::ObjectNode& n ):  node(n) {};
            };

        private:
            std::unique_ptr<opcua::Server> opcServer = {nullptr };
            std::unique_ptr<IONode> ioNode = {nullptr};
            std::string prefix;
            std::string s_field;
            std::string s_fvalue;
            uniset::timeout_t updatePause_msec = { 200 };
            std::atomic_bool firstUpdate = false;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _OPCUAGate_H_
// -----------------------------------------------------------------------------
