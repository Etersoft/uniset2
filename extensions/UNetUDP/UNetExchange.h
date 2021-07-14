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
    \page pageUNetExchangeUDP Сетевой обмен на основе UDP (UNetUDP)

    - \ref pgUNetUDP_Common
    - \ref pgUNetUDP_Conf
    - \ref pgUNetUDP_ConfMulticast
    - \ref pgUNetUDP_Reserv
    - \ref pgUNetUDP_SendFactor
    - \ref pgUNetUDP_Stat

    \section pgUNetUDP_Common Общее описание
    Обмен построен  на основе протокола UDP.
    Основная идея заключается в том, что каждый узел на порту равном своему ID
    посылает в сеть UDP-пакеты содержащие данные считанные из локальной SM. Формат данных - это набор
    пар [id,value]. Другие узлы принимают их. Помимо этого данный процесс запускает
    "получателей" по одному на каждый (другой) узел и ловит пакеты от них, сохраняя данные в SM.
    При этом "получатели" работают на одном потоке с использованием libev (см. UNetReceiver)
    или каждый на своём потоке. Это определяется параметром \b unet_update_strategy.

    В текущей версии поддерживается два протокола для обмена broadcast udp и multicast. Какой использовать протокол
    определяется в настроечной секции параметром \b unet_transport="broadcast" или \b unet_transport="multicast".
    По умолчанию "broadcast".
    \code
            <UNetExchange name=".."  unet_transport="broadcast" />
    \endcode
    В зависимости от заданного протокола, будут использованы те или иные настройки.

    \par
    При своём старте процесс считывает из секции \<nodes> список узлов которые необходимо "слушать",
    а также параметры своего узла. Открывает по потоку приёма на каждый узел и поток
    передачи данных от своего узла. Помимо этого такие же потоки для резервных каналов, если они включены
    (см. \ref pgUNetUDP_Reserv ).

    \section pgUNetUDP_Conf Пример конфигурирования (UDP)
    По умолчанию при считывании используется \b unet_broadcast_ip (указанный в секции \<nodes>)
    и \b id узла - в качестве порта.
    Но можно переопределять эти параметры, при помощи указания \b unet_port и/или \b unet_broadcast_ip,
    для конкретного узла (\<item>).
    \code
    <nodes port="2809" unet_broadcast_ip="192.168.56.255">
      <item ip="127.0.0.1" name="LocalhostNode" textname="Локальный узел" unet_ignore="1" unet_port="3000" unet_broadcast_ip="192.168.57.255">
        <iocards>
          ...
        </iocards>
      </item>
      <item ip="192.168.56.10" name="Node1" textname="Node1" unet_port="3001" unet_update_strategy="evloop"/>
      <item ip="192.168.56.11" name="Node2" textname="Node2" unet_port="3002"/>
    </nodes>
    \endcode

        Буфер для приёма сообщений можно настроить параметром \b recvBufferSize="1000" в конфигурационной секции
        или аргументом командной строки \b --prefix-recv-buffer-size sz

    Максимальное число сообщений вычитываемых из ести за один раз настраивается параметром
    \b recvMaxAtTime="5" или \b --prefix-recv-max-at-time num

    \note Имеется возможность задавать отдельную настроечную секцию для "списка узлов" при помощи параметра
     --prefix-nodes-confnode name. По умолчанию настройка ведётся по секции <nodes>

    Чтобы отключить запуск "sender", можно указать \b nosender="1" в \b <item> конкретного узла
    или непосредственно в настройках \b <UNetExchange  nosender="1"...>


    \section pgUNetUDP_ConfMulticast Пример конфигурирования (Multicast)
    По умолчанию при считывании используется \b unet_multicast_ip и \b id узла - в качестве порта.
    Но можно переопределиять эти параметры, при помощи указания \b unet_multicast_port и/или \b unet_multicast_ip,
    у конкретного узла (\<item>).
    При этом в параметре unet_multicast_ip должен быть задан адрес multicast-групы на которую будет
    подписываться каждый receiver и в которую будет писать соответствующий sender.
    По умолчанию для подключения к группе используется интерфейс ANY, но параметром
    unet_multicast_iface="192.168.1.1" можно задать интерфейс через который ожидаются multicast-пакеты.
    Поддерживается текстовое задание интерфейса в виде unet_multicast_iface="eth0"

    Для указания ip для sender используется параметр unet_multicast_sender_ip="..", если он не задан,
    будет использован unet_multicast_iface="..".

    Для посылающего процесса можно определить параметр \b unet_multicast_ttl задающий время жизни multicast пакетов.
    По умолчанию ttl=1. А так же определить ip для сокета параметром \b unet_multicast_iface.
    Можно задавать текстовое название интерфейса unet_multicast_iface="eth0", при этом
    в качестве ip будет взят \b первый ip-адрес из привязанных к указанному интерфейсу.

    В данной реализации поддерживается работа в два канала. Соответствующие настройки для второго канала имеют индекс "2":
    \a unet_multicast_ip2, \a unet_multicast_port2, \a unet_multicast_iface2

    Чтобы отключить запуск "sender", можно указать \b nosender="1" в \b <item> конкретного узла
    или непосредственно в настройках \b <UNetExchange  nosender="1"...>

    В корневой секции \b <nodes..>  можно задавать значения по умолчанию используемые для всех улов
    \b  unet_multicast_ip, \b unet_multicast_iface, \b unet_multicast_sender_ip.

    \code
    <nodes port="2809" unet_broadcast_ip="192.168.56.255" unet_multicast_ip="224.0.0.1 unet_multicast_iface="net1">
      <item ip="127.0.0.1" name="LocalhostNode" textname="Локальный узел" unet_ignore="1">
        <iocards>
          ...
        </iocards>
      </item>
      <item id="3001" ip="192.168.56.10" name="Node1" textname="Node1" unet_update_strategy="evloop"
            unet_multicast_ip="224.0.0.1"
            unet_multicast_iface="192.168.1.1"
            unet_multicast_port2="3031"
            unet_multicast_ip2="225.0.0.1"
            unet_multicast_iface2="192.168.2.1">
        ...
      </item>
      <item id="3002" ip="192.168.56.11" name="Node2" textname="Node2">
            unet_multicast_ip="224.0.0.2"
            unet_multicast_iface="192.168.1.2"
            unet_multicast_port2="3032"
            unet_multicast_ip2="225.0.0.2"
            unet_multicast_iface2="eth0">
        ...
      </item>
    </nodes>
    \endcode

    \section pgUNetUDP_Reserv Настройка резервного канала связи
    В текущей реализации поддерживается возможность обмена по двум подсетям (Ethernet-каналам).
    Она основана на том, что, для каждого узла помимо основного "читателя",
    создаётся дополнительный "читатель" слушающий другой ip-адрес и порт.
    А так же, для локального узла создаётся дополнительный "писатель",
    который посылает данные в (указанную) вторую подсеть. Для того, чтобы задействовать
    второй канал, достаточно объявить в настройках переменные
    \b unet_broadcast_ip2. А также в случае необходимости для конкретного узла
    можно указать \b unet_broadcast_ip2 и \b unet_port2.
    Или в случае multicast \b unet_multicast_ip2 и \b unet_multicast_port2.

    Переключение между "каналами" происходит по следующей логике:

    При старте включается только первый канал. Второй канал работает в режиме "пассивного" чтения.
    Т.е. все пакеты принимаются, но данные в SharedMemory не сохраняются.
    Если во время работы пропадает связь по первому каналу, идёт переключение на второй канал.
    Первый канал переводиться в "пассивный" режим, а второй канал, переводится в "нормальный"(активный)
    режим. Далее работа ведётся по второму каналу, независимо от того, что связь на первом
    канале может восстановиться. Это сделано для защиты от постоянных перескакиваний
    с канала на канал. Работа на втором канале будет вестись, пока не пропадёт связь
    на нём. Тогда будет попытка переключиться обратно на первый канал и так "по кругу".

    В свою очередь "писатели"(если они не отключены) всегда посылают данные в оба канала.

    \section pgUNetUDP_SendFactor Регулирование частоты посылки
    Данный механизм, позволяет регулировать частоту посылки данных
    для каждого датчика. Суть механизма заключается в том, что для каждого датчика можно задать свойство
    - \b prefix_sendfactor="N" Где N>1 - задаёт "делитель" относительно \b sendpause определяющий с какой частотой
    информация о данном датчике будет посылаться. Например N=2 - каждый второй цикл, N=3 - каждый третий и т.п.
    При загрузке все датчики (относящиеся к данному процессу) разбиваются на группы пакетов согласно своей частоте посылки.
    При этом внутри одной группы датчики разбиваются по пакетам согласно заданному максимальному размеру пакета
    (см. конструктор класса UNetSender()).

    \section pgUNetUDP_PackSendPause Пауза между посылкой пакетов
    Параметр \b --prefix-packsendpause или \b packsendpause в настаройках позволяет задать паузу (в миллисекундах)
    между посылками пакетов. Если итоговых пакетов с данными больше чем 1.
    При этом параметр \b --prefix-packsendpause-factor или \b packsendpauseFactor позволяет указать,
    что необходимо делать паузы не между каждым пакетом, а через каждый N пакет.
    По умолчанию \b packsendpause=5 миллисекунд.

     \section pgUNetUDP_Stat Статистика работы канала
     Для возможности мониторинга работы имеются счётчики, которые можно привязать к датчикам,
     задав их для соответствующего узла в секции '<nodes>' конфигурационного файла.

     - unet_lostpackets_id=""  - общее количество потерянных пакетов с данным узлом (суммарно по обоим каналам)
     - unet_lostpackets1_id=""  - количество потерянных пакетов с данным узлом по первому каналу
     - unet_lostpackets2_id=""  - количество потерянных пакетов с данным узлом по второму каналу
     - unet_respond_id=""  - наличие связи хотя бы по одному каналу
     - unet_respond1_id  - наличие связи по первому каналу
     - unet_respond2_id  - наличие связи по второму каналу
     - unet_numchannel_id=""   - номер текущего активного канала
     - unet_channelswitchcount_id="" - количество переключений с канала на канал
    */
    // -----------------------------------------------------------------------------
    /*! Реализация обмена по протоколу UNet */
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
