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
    \page page_OPCUAGate (OPCUAGate) Коннектор к uniset поддерживающий протокол OPC-UA

      - \ref sec_OPCUAGate_Comm
      - \ref sec_OPCUAGate_Conf

    \section sec_OPCUAGate_Comm Общее описание OPCUAGate


    \section sec_OPCUAGate_Conf Настройка OPCUAGate
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
            uniset::timeout_t updateLoopPause_msec = { 100 };
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _OPCUAGate_H_
// -----------------------------------------------------------------------------
