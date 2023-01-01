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
extern "C" {
#include <open62541/types.h>
}
#include <memory>
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
struct UA_Server;
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
            OPCUAGate( uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                       const std::string& prefix = "opcua" );
            virtual ~OPCUAGate();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<OPCUAGate> init_opcuagate( int argc, const char* const* argv,
                    uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "opcua" );

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
            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            virtual bool deactivateObject() override;

            void serverLoopTerminate();
            void serverLoop();

            std::shared_ptr<SMInterface> shm;
            std::unique_ptr<ThreadCreator<OPCUAGate>> serverThread;

        private:
            UA_Server* uaServer;
            volatile UA_Boolean running;
            std::string prefix;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _OPCUAGate_H_
// -----------------------------------------------------------------------------
