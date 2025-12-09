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
#ifndef SharedMemory_H_
#define SharedMemory_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include <string>
#include <memory>
#include <deque>
#include <time.h>
#include "IONotifyController.h"
#include "Mutex.h"
#include "PassiveTimer.h"
#include "WDTInterface.h"
#include "LogServer.h"
#include "DebugStream.h"
#include "LogAgregator.h"
#include "VMonitor.h"
#include "IOConfig_XML.h"
#include "USingleProcess.h"
// -----------------------------------------------------------------------------
#ifndef vmonit
#define vmonit( var ) vmon.add( #var, var )
#endif
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*! Реализация SharedMemory */
    class SharedMemory:
        private USingleProcess,
        public IONotifyController
    {
        public:

            // конструктор с конфигурированием через xml
            SharedMemory( ObjectId id,
                          xmlNode* cnode,
                          const std::shared_ptr<IOConfig_XML>& ioconf );

            virtual ~SharedMemory();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<SharedMemory> init_smemory( int argc, const char* const* argv );

            /*! глобальная функция для вывода help-а */
            static void help_print( int argc, const char* const* argv );

            // функция определяет "готовность" SM к работе.
            // должна использоваться другими процессами, для того,
            // чтобы понять, когда можно получать от SM данные.
            virtual CORBA::Boolean exist() override;

            virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

            void addReadItem( IOConfig_XML::ReaderSlot sl );

            // ------------  HISTORY  --------------------
            typedef std::deque<long> HBuffer;

            struct HistoryItem
            {
                explicit HistoryItem( size_t bufsize = 0 ): id(uniset::DefaultObjectId), buf(bufsize) {}
                HistoryItem( const uniset::ObjectId _id, const size_t bufsize, const long val ): id(_id), buf(bufsize, val) {}

                inline void init( size_t size, long val )
                {
                    if( size > 0 )
                        buf.assign(size, val);
                }

                uniset::ObjectId id;
                HBuffer buf;

                IOStateList::iterator ioit;

                void add( long val, size_t size )
                {
                    // т.к. буфер у нас уже заданного размера
                    // то просто удаляем очередную точку в начале
                    // и добавляем в конце
                    buf.pop_front();
                    buf.push_back(val);
                }
            };

            typedef std::list<HistoryItem> HistoryList;

            struct HistoryInfo
            {
                HistoryInfo()
                {
                    ::clock_gettime(CLOCK_REALTIME, &fuse_tm);
                }

                long id = { 0 };                // ID
                HistoryList hlst;               // history list
                size_t size = { 0 };
                std::string filter = { "" }; // filter field
                uniset::ObjectId fuse_id = { uniset::DefaultObjectId };  // fuse sensor
                bool fuse_invert = { false };
                bool fuse_use_val = { false };
                long fuse_val = { 0 };
                timespec fuse_tm = { 0, 0 }; // timestamp
            };

            friend std::ostream& operator<<( std::ostream& os, const HistoryInfo& h );

            typedef std::list<HistoryInfo> History;

            // т.к. могут быть одинаковые "детонаторы" для разных "историй" то,
            // вводим не просто map, а "map списка историй".
            // точнее итераторов-историй.
            typedef std::list<History::iterator> HistoryItList;
            typedef std::unordered_map<uniset::ObjectId, HistoryItList> HistoryFuseMap;

            typedef sigc::signal<void, const HistoryInfo&> HistorySlot;
            HistorySlot signal_history(); /*!< сигнал о срабатывании условий "сброса" дампа истории */

            inline int getHistoryStep() const
            {
                return histSaveTime;    /*!< период между точками "дампа", мсек */
            }

            // -------------------------------------------------------------------------------

            inline std::shared_ptr<LogAgregator> logAgregator()
            {
                return loga;
            }
            inline std::shared_ptr<DebugStream> log()
            {
                return smlog;
            }

        protected:
            typedef std::list<IOConfig_XML::ReaderSlot> ReadSlotList;
            ReadSlotList lstRSlot;

            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            virtual void timerInfo( const uniset::TimerMessage* tm ) override;
            virtual std::string getTimerName(int id) const override;

            void sendEvent( uniset::SystemMessage& sm );
            void initFromReserv();
            bool initFromSM( uniset::ObjectId sm_id, uniset::ObjectId sm_node );
            void reloadConfig();

            virtual bool activateObject() override;
            virtual bool deactivateObject() override;
            bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );

            void buildEventList( xmlNode* cnode );
            void readEventList( const std::string& oname );

            std::mutex mutexStart;

            class HeartBeatInfo
            {
                public:
                    HeartBeatInfo():
                        a_sid(uniset::DefaultObjectId),
                        d_sid(uniset::DefaultObjectId),
                        reboot_msec(UniSetTimer::WaitUpTime),
                        timer_running(false),
                        ptReboot(UniSetTimer::WaitUpTime)
                    {}

                    uniset::ObjectId a_sid; // аналоговый счётчик
                    uniset::ObjectId d_sid; // дискретный датчик состояния процесса
                    IOStateList::iterator a_it;
                    IOStateList::iterator d_it;

                    timeout_t reboot_msec; /*!< Время в течение которого процесс обязан подтвердить своё существование,
                                иначе будет произведена перезагрузка контроллера по WDT (в случае если он включён).
                                Если данный параметр не указывать, то "не живость" процесса просто игнорируется
                                (т.е. только сброс датчика heartbeat (d_sid) в ноль). */

                    bool timer_running;
                    PassiveTimer ptReboot;
            };

            enum Timers
            {
                tmHeartBeatCheck,
                tmEvent,
                tmHistory,
                tmPulsar,
                tmLastOfTimerID
            };

            int heartbeatCheckTime;
            std::string heartbeat_node;
            int histSaveTime;

            void checkHeartBeat();

            typedef std::list<HeartBeatInfo> HeartBeatList;
            HeartBeatList hblist; // список датчиков "сердцебиения"
            std::shared_ptr<WDTInterface> wdt;
            std::atomic_bool activated = { false };
            std::atomic_bool workready = { false };
            std::atomic_bool cancelled = { false };

            typedef std::list<uniset::ObjectId> EventList;
            EventList elst;
            std::string e_filter;
            int evntPause;
            int activateTimeout;

            virtual void logging( uniset::SensorMessage& sm ) override;

            bool dblogging = { false };

            //! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет
            //! использовать userdata[2]. (0,1 - использует IONotifyController)
            // оптимизация с использованием userdata (IOController::USensorInfo::userdata) нужна
            // чтобы не использовать поиск в HistoryFuseMap (см. checkFuse)
            // т.к. 0,1 - использует IONotifyController (см. IONotifyController::UserDataID)
            // то используем не занятый "2" - в качестве элемента userdata
            static const size_t udataHistory = 2;

            History hist;
            HistoryFuseMap histmap;  /*!< map для оптимизации поиска */

            virtual void checkFuse( std::shared_ptr<IOController::USensorInfo>& usi, IOController* );
            virtual void saveToHistory();

            void buildHistoryList( xmlNode* cnode );
            void checkHistoryFilter( UniXML::iterator& it );

            IOStateList::iterator itPulsar;
            uniset::ObjectId sidPulsar;
            int msecPulsar;

            xmlNode* confnode;

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> smlog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            VMonitor vmon;

#ifndef DISABLE_REST_API
            Poco::JSON::Object::Ptr buildLogServerInfo();
            virtual Poco::JSON::Object::Ptr httpRequest( const UHttp::HttpRequestContext& ctx ) override;
            virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override;
            virtual Poco::JSON::Object::Ptr httpGetMyInfo( Poco::JSON::Object::Ptr root ) override;
#endif

        private:
            HistorySlot m_historySignal;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // SharedMemory_H_
// -----------------------------------------------------------------------------
