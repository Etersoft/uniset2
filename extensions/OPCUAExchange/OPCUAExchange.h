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

            static const size_t channels = 2;

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

                OPCAttribute() {}

                uniset::uniset_rwmutex vmut;
                long val { 0 };
                Tick tick = { 0 }; // на каждом ли тике работать с этим аттрибутом
                OPCUAClient::Variable32 request[channels];

                friend std::ostream& operator<<(std::ostream& os, const OPCAttribute& inf );
                friend std::ostream& operator<<(std::ostream& os, const std::shared_ptr<OPCAttribute>& inf );
            };

        protected:

            enum Timers
            {
                tmUpdates
            };

            void channel1Exchange();
            void channel2Exchange();
            bool prepare();
            void channelExchange( Tick tick, size_t chan );
            void updateFromChannel( size_t chan );
            void updateToChannel( size_t chan );
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
            void buildRequests();
            bool tryConnect(size_t chan);
            void initOutputs();

            xmlNode* confnode = { 0 }; /*!< xml-узел в настроечном файле */

            timeout_t polltime = { 150 };   /*!< периодичность обновления данных, [мсек] */
            timeout_t updatetime = { 150 };   /*!< периодичность обновления данных в SM, [мсек] */

            typedef std::vector< std::shared_ptr<OPCAttribute> > IOList;
            IOList iolist;    /*!< список входов/выходов */
            int maxItem = { 0 };

            std::string addr[channels];
            std::string user[channels];
            std::string pass[channels];
            std::shared_ptr<OPCUAClient> client[channels];
            struct ClientInfo
            {
                uniset::Trigger trStatus;
                uniset::PassiveTimer ptTimeout;
                std::atomic_bool status = { true };
            };
            ClientInfo clientInfo[channels];
            uniset::Trigger noConnections;
            std::atomic_uint32_t currentChannel = { 0 };
            typedef std::vector<OPCUAClient::Variable32*> Request;

            uniset::timeout_t reconnectPause = { 10000 };
            // <priority, requests>
            std::unordered_map<Tick, Request> writeRequests[channels];
            std::unordered_map<Tick, Request> readRequests[channels];

            int filtersize = { 0 };
            float filterT = { 0.0 };

            std::string s_field;
            std::string s_fvalue;

            std::shared_ptr<SMInterface> shm;
            uniset::ObjectId myid = { uniset::DefaultObjectId };
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
            bool readconf_ok = { false };
            int activateTimeout;
            uniset::ObjectId sidTestSMReady = { uniset::DefaultObjectId };

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> opclog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            std::shared_ptr< ThreadCreator<OPCUAExchange> > thrChannel[channels];

            VMonitor vmon;

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // OPCUAExchange_H_
// -----------------------------------------------------------------------------
