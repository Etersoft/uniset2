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
#ifndef UNetExchange_H_
#define UNetExchange_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <queue>
#include <deque>
#include "UniSetObject.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "UNetReceiver.h"
#include "UNetSender.h"
#include "LogServer.h"
#include "DebugStream.h"
#include "UNetLogSugar.h"
#include "LogAgregator.h"
#include "VMonitor.h"
// -----------------------------------------------------------------------------
#ifndef vmonit
#define vmonit( var ) vmon.add( #var, var )
#endif
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
        \ingroup UNetUDP
        Реализация обмена по протоколу UNet.
        \sa См. README.md для полной документации
    */
    class UNetExchange:
        public UniSetObject
    {
        public:
            UNetExchange( uniset::ObjectId objId, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "unet" );
            virtual ~UNetExchange();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<UNetExchange> init_unetexchange( int argc, const char* const argv[],
                    uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = 0, const std::string& prefix = "unet" );

            /*! глобальная функция для вывода help-а */
            static void help_print( int argc, const char* argv[] ) noexcept;

            bool checkExistTransport( const std::string& transportID ) noexcept;

            inline std::shared_ptr<LogAgregator> getLogAggregator() noexcept
            {
                return loga;
            }
            inline std::shared_ptr<DebugStream> log() noexcept
            {
                return unetlog;
            }

            virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

#ifndef DISABLE_REST_API
            // HTTP API
            virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override;
            virtual Poco::JSON::Object::Ptr httpRequest( const UHttp::HttpRequestContext& ctx ) override;
            virtual Poco::JSON::Object::Ptr httpGetMyInfo( Poco::JSON::Object::Ptr root ) override;

            Poco::JSON::Object::Ptr httpStatus();
            Poco::JSON::Object::Ptr httpReceivers( const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpSenders( const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpGetParam( const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpSetParam( const Poco::URI::QueryParameters& p );

            bool httpEnabledSetParams = { false };
#endif

        protected:

            xmlNode* cnode;
            std::string s_field;
            std::string s_fvalue;

            std::shared_ptr<SMInterface> shm;
            void step() noexcept;

            void sysCommand( const uniset::SystemMessage* msg ) override;
            void sensorInfo( const uniset::SensorMessage* sm ) override;
            void timerInfo( const uniset::TimerMessage* tm ) override;
            void askSensors( UniversalIO::UIOCommand cmd );
            bool waitSMReady();
            void receiverEvent( const std::shared_ptr<UNetReceiver>& r, UNetReceiver::Event ev ) noexcept;

            virtual bool activateObject() override;
            virtual bool deactivateObject() override;

            // действия при завершении работы
            void termSenders();
            void termReceivers();

            void initMulticastTransport( UniXML::iterator nodes, const std::string& n_field, const std::string& n_fvalue, const std::string& prefix );
            void initMulticastReceiverForNode( UniXML::iterator root, UniXML::iterator n_it, const std::string& prefix );

            void initUDPTransport(UniXML::iterator nodes, const std::string& n_field, const std::string& n_fvalue, const std::string& prefix);
            void initIterators() noexcept;
            void startReceivers();

            enum Timer
            {
                tmStep
            };

        private:
            UNetExchange();
            timeout_t initPause = { 0 };
            uniset::uniset_rwmutex mutex_start;

            PassiveTimer ptHeartBeat;
            uniset::ObjectId sidHeartBeat = { uniset::DefaultObjectId };
            timeout_t maxHeartBeat = { 10 };
            IOController::IOStateList::iterator itHeartBeat;
            uniset::ObjectId test_id = { uniset::DefaultObjectId };

            timeout_t steptime = { 1000 };    /*!< периодичность вызова step, [мсек] */

            std::atomic_bool activated = { false };
            std::atomic_bool cancelled = { false };
            timeout_t activateTimeout = { 20000 }; // msec

            struct ReceiverInfo
            {
                ReceiverInfo() noexcept: r1(nullptr), r2(nullptr),
                    sidRespond(uniset::DefaultObjectId),
                    respondInvert(false),
                    sidLostPackets(uniset::DefaultObjectId),
                    sidChannelNum(uniset::DefaultObjectId)
                {}

                ReceiverInfo( const std::shared_ptr<UNetReceiver>& _r1, const std::shared_ptr<UNetReceiver>& _r2 ) noexcept:
                    r1(_r1), r2(_r2),
                    sidRespond(uniset::DefaultObjectId),
                    respondInvert(false),
                    sidLostPackets(uniset::DefaultObjectId),
                    sidChannelNum(uniset::DefaultObjectId)
                {}

                std::shared_ptr<UNetReceiver> r1;    /*!< приём по первому каналу */
                std::shared_ptr<UNetReceiver> r2;    /*!< приём по второму каналу */

                void step(const std::shared_ptr<SMInterface>& shm, const std::string& myname, std::shared_ptr<DebugStream>& log ) noexcept;

                inline void setRespondID( uniset::ObjectId id, bool invert = false ) noexcept
                {
                    sidRespond = id;
                    respondInvert = invert;
                }
                inline void setLostPacketsID( uniset::ObjectId id ) noexcept
                {
                    sidLostPackets = id;
                }
                inline void setChannelNumID( uniset::ObjectId id ) noexcept
                {
                    sidChannelNum = id;
                }

                inline void setChannelSwitchCountID( uniset::ObjectId id ) noexcept
                {
                    sidChannelSwitchCount = id;
                }

                inline void initIterators( const std::shared_ptr<SMInterface>& shm ) noexcept
                {
                    shm->initIterator(itLostPackets);
                    shm->initIterator(itRespond);
                    shm->initIterator(itChannelNum);
                    shm->initIterator(itChannelSwitchCount);
                }

                // Сводная информация по двум каналам
                // сумма потерянных пакетов и наличие связи
                // хотя бы по одному каналу, номер рабочего канала
                // количество переключений с канала на канал
                // ( реализацию см. ReceiverInfo::step() )
                uniset::ObjectId sidRespond;
                IOController::IOStateList::iterator itRespond;
                bool respondInvert = { false };
                uniset::ObjectId sidLostPackets;
                IOController::IOStateList::iterator itLostPackets;
                uniset::ObjectId sidChannelNum;
                IOController::IOStateList::iterator itChannelNum;

                long channelSwitchCount = { 0 }; /*!< счётчик переключений с канала на канал */
                uniset::ObjectId sidChannelSwitchCount = { uniset::DefaultObjectId };
                IOController::IOStateList::iterator itChannelSwitchCount;
            };

            typedef std::deque<ReceiverInfo> ReceiverList;
            ReceiverList recvlist;

            bool no_sender = { false };  /*!< флаг отключения посылки сообщений (создания потока для посылки)*/
            std::shared_ptr<UNetSender> sender;
            std::shared_ptr<UNetSender> sender2;

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> unetlog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            VMonitor vmon;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UNetExchange_H_
// -----------------------------------------------------------------------------
