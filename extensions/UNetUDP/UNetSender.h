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
#ifndef UNetSender_H_
#define UNetSender_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <vector>
#include <limits>
#include <unordered_map>
#include "UniSetObject.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "UDPPacket.h"
#include "UNetTransport.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*
     *    Распределение датчиков по пакетам
     *    =========================================================================
     *    Все пересылаемые данные разбиваются на группы по частоте посылки("sendfactor").
     *    Частота посылки кратна sendpause, задаётся для каждого датчика, при помощи свойства prefix_sendfactor.
     *    Внутри каждой группы пакеты набираются по мере "заполнения".
     *
     *    Добавление датчика в пакет и создание нового пакета при переполнении происходит в функции initItem().
     *    Причем так как дискретные и аналоговые датчики обрабатываются отдельно (но пересылаются в одном пакете),
     *    то датчики, которые первые переполнятся приводят к тому, что создаётся новый пакет и они добавляются в него,
     *    в свою очередь остальные продолжают "добивать" предыдущий пакет.
     *    В initItem() каждому UItem в dlist кроме pack_ind присваивается еще и номер пакета pack_num, который гарантировано соответствует
     *    существующему пакету, поэтому в дальнейшем при использовании pack_num в качестве ключа в mypacks мы не проверяем пакет на существование.
     *
     * Создание соединения
     * ======================================
     * Попытка создать соединение производиться сразу в конструкторе, если это не получается,
     * то в потоке "посылки", с заданным периодом (checkConnectionTime) идёт попытка создать соединение..
     * и так бесконечно, пока не получиться. Это важно для систем, где в момент загрузки программы
     * (в момент создания объекта UNetSender) ещё может быть не поднята сеть или какой-то сбой с сетью и требуется
     * ожидание (без вылета программы) пока "внешняя система мониторинга" не поднимет сеть).
     * Если такая логика не требуется, то можно задать в конструкторе флаг nocheckconnection=true,
     * тогда при создании объекта UNetSender, в конструкторе будет
     * выкинуто исключение при неудачной попытке создания соединения.
     * \warning setCheckConnectionPause(msec) должно быть кратно sendpause!
     */
    class UNetSender final
    {
        public:
            UNetSender( std::unique_ptr<UNetSendTransport>&& transport, const std::shared_ptr<SMInterface>& smi
                        , bool nocheckConnection = false
                        , const std::string& s_field = ""
                        , const std::string& s_fvalue = ""
                        , const std::string& prop_prefix = "unet"
                        , const std::string& prefix = "unet"
                        , size_t maxDCount = UniSetUDP::MaxDCount
                        , size_t maxACount = UniSetUDP::MaxACount );

            ~UNetSender();

            typedef size_t sendfactor_t;

            static const long not_specified_value = { std::numeric_limits<long>::max() };

            enum class Mode : int
            {
                mEnabled = 0, /*!< обычный режим */
                mDisabled = 1 /*!< посылка отключена */
            };

            struct UItem
            {
                UItem():
                    iotype(UniversalIO::UnknownIOType),
                    id(uniset::DefaultObjectId),
                    pack_num(0),
                    pack_ind(0),
                    pack_sendfactor(0) {}

                UniversalIO::IOType iotype;
                uniset::ObjectId id;
                IOController::IOStateList::iterator ioit;
                size_t pack_num;
                size_t pack_ind;
                sendfactor_t pack_sendfactor = { 0 };
                long undefined_value = { not_specified_value };
                friend std::ostream& operator<<( std::ostream& os, UItem& p );
            };

            typedef std::unordered_map<uniset::ObjectId, UItem> UItemMap;

            size_t getDataPackCount() const noexcept;

            void start();
            void stop();

            void send() noexcept;

            struct PackMessage
            {
                PackMessage( uniset::UniSetUDP::UDPMessage&& m ) noexcept: msg(std::move(m)) {}
                PackMessage( const uniset::UniSetUDP::UDPMessage& m ) = delete;

                PackMessage() noexcept {}

                uniset::UniSetUDP::UDPMessage msg;
                uniset::uniset_rwmutex mut;
            };

            void real_send( PackMessage& mypack ) noexcept;

            /*! (принудительно) обновить все данные (из SM) */
            void updateFromSM();

            /*! Обновить значение по ID датчика */
            void updateSensor( uniset::ObjectId id, long value );

            /*! Обновить значение по итератору */
            void updateItem( const UItem& it, long value );

            inline void setSendPause( int msec ) noexcept
            {
                sendpause = msec;
            }
            inline void setPackSendPause( int msec ) noexcept
            {
                packsendpause = msec;
            }
            inline void setPackSendPauseFactor( int factor ) noexcept
            {
                packsendpauseFactor = factor;
            }

            void setModeID( uniset::ObjectId id ) noexcept;

            void setCheckConnectionPause( int msec ) noexcept;

            /*! заказать датчики */
            void askSensors( UniversalIO::UIOCommand cmd );

            /*! инициализация  итераторов */
            void initIterators() noexcept;

            inline std::shared_ptr<DebugStream> getLog() noexcept
            {
                return unetlog;
            }

            std::string getShortInfo() const noexcept;

            inline size_t getADataSize() const noexcept
            {
                return maxAData;
            }
            inline size_t getDDataSize() const noexcept
            {
                return maxDData;
            }

        protected:

            std::string s_field = { "" };
            std::string s_fvalue = { "" };
            std::string prop_prefix = { "" };

            const std::shared_ptr<SMInterface> shm;
            std::shared_ptr<DebugStream> unetlog;

            bool initItem( UniXML::iterator& it );
            bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );

            void readConfiguration();

            bool createConnection( bool throwEx );

        private:
            UNetSender();

            std::unique_ptr<UNetSendTransport> transport;

            std::string myname = { "" };
            timeout_t sendpause = { 150 };
            timeout_t packsendpause = { 5 };
            int packsendpauseFactor = { 1 };
            timeout_t writeTimeout = { 1000 }; // msec
            std::atomic_bool activated = { false };
            PassiveTimer ptCheckConnection;

            // режим работы
            uniset::ObjectId sidMode = { uniset::DefaultObjectId };
            IOController::IOStateList::iterator itMode;
            Mode mode = { Mode::mEnabled };

            typedef std::unordered_map<sendfactor_t, std::vector<PackMessage>> Packs;

            // mypacks заполняется в начале и дальше с ним происходит только чтение
            // (меняются данные внутри пакетов, но не меняется само количество пакетов)
            // поэтому mutex-ом его не защищаем
            Packs mypacks;
            std::unordered_map<sendfactor_t, size_t> packs_anum;
            std::unordered_map<sendfactor_t, size_t> packs_dnum;
            UItemMap items;
            size_t packetnum = { 1 }; /*!< номер очередного посылаемого пакета */

            size_t maxAData = { UniSetUDP::MaxACount };
            size_t maxDData = { UniSetUDP::MaxDCount };

            std::unique_ptr< ThreadCreator<UNetSender> > s_thr;    // send thread

            size_t ncycle = { 0 }; /*!< номер цикла посылки */

    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
namespace std
{
    std::string to_string( const uniset::UNetSender::Mode& p );
}
// -----------------------------------------------------------------------------
#endif // UNetSender_H_
// -----------------------------------------------------------------------------
