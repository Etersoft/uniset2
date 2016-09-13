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
// -----------------------------------------------------------------------------
/*!
    \page pageUNetExchangeUDP Сетевой обмен на основе UDP (UNetUDP)

        - \ref pgUNetUDP_Common
        - \ref pgUNetUDP_Conf
        - \ref pgUNetUDP_Reserv
        - \ref pgUNetUDP_SendFactor

    \section pgUNetUDP_Common Общее описание
        Обмен построен  на основе протокола UDP.
        Основная идея заключается в том, что каждый узел на порту равном своему ID
	посылает в сеть UDP-пакеты содержащие данные считанные из локальной SM. Формат данных - это набор
    пар [id,value]. Другие узлы принимают их. Помимо этого данный процесс запускает
	"получателей" по одному на каждый (другой) узел и ловит пакеты от них, сохраняя данные в SM.
	При этом "получатели" работают на одном(!) потоке с использованием событий libev (см. UNetReceiver).

    \par
        При своём старте процесс считывает из секции \<nodes> список узлов которые необходимо "слушать",
    а также параметры своего узла. Открывает по потоку приёма на каждый узел и поток
    передачи для своих данных. Помимо этого такие же потоки для резервных каналов, если они включены
    (см. \ref pgUNetUDP_Reserv ).

    \section pgUNetUDP_Conf Пример конфигурирования
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

	* \b unet_update_strategy - задаёт стратегию обновления данных в SM.
	   Поддерживается два варианта:
	- 'thread' - отдельный поток обновления
	- 'evloop' - использование общего с приёмом event loop (libev)

	\note Имеется возможность задавать отдельную настроечную секцию для "списка узлов" при помощи параметра
	 --prefix-nodes-confnode name. По умолчанию настройка ведётся по секции <nodes>

    \section pgUNetUDP_Reserv Настройка резервного канала связи
        В текущей реализации поддерживается возможность обмена по двум подсетям (эзернет-каналам).
    Она основана на том, что, для каждого узла помимо основного "читателя",
    создаётся дополнительный "читатель"(поток) слушающий другой ip-адрес и порт.
    А так же, для локального узла создаётся дополнительный "писатель"(поток),
    который посылает данные в (указанную) вторую подсеть. Для того, чтобы задействовать
    второй канал, достаточно объявить в настройках переменные
    \b unet_broadcast_ip2. А также в случае необходимости для конкретного узла
    можно указать \b unet_broadcast_ip2 и \b unet_port2.

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
	В текущей реализации поддерживается механизм, позволяющий регулировать частоту посылки данных
	для каждого датчика. Суть механизма заключается в том, что для каждого датчика можно задать свойство
	- \b prefix_sendfactor="N" Где N>1 - задаёт "делитель" относительно \b sendpause определяющий с какой частотой
	информация о данном датчике будет посылаться. Например N=2 - каждый второй цикл, N=3 - каждый третий и т.п.
	При загрузке все датчики (относщиеся к данному процессу) разбиваются на группы пакетов согласно своей частоте посылки.
	При этом внутри одной группы датчики разбиваются по пакетам согласно заданному максимальному размеру пакета
	(см. конструктор класса UNetSender()).
*/
// -----------------------------------------------------------------------------
class UNetExchange:
	public UniSetObject
{
	public:
		UNetExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "unet" );
		virtual ~UNetExchange();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<UNetExchange> init_unetexchange( int argc, const char* const argv[],
				UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = 0, const std::string& prefix = "unet" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* argv[] ) noexcept;

		bool checkExistUNetHost( const std::string& host, int port ) noexcept;

		inline std::shared_ptr<LogAgregator> getLogAggregator() noexcept
		{
			return loga;
		}
		inline std::shared_ptr<DebugStream> log() noexcept
		{
			return unetlog;
		}

		virtual UniSetTypes::SimpleInfo* getInfo( CORBA::Long userparam = 0 ) override;

	protected:

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		std::shared_ptr<SMInterface> shm;
		void step() noexcept;

		void sysCommand( const UniSetTypes::SystemMessage* msg ) override;
		void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		void askSensors( UniversalIO::UIOCommand cmd );
		void waitSMReady();
		void receiverEvent( const std::shared_ptr<UNetReceiver>& r, UNetReceiver::Event ev ) noexcept;

		virtual bool activateObject() override;
		virtual bool deactivateObject() override;

		// действия при завершении работы
		virtual void sigterm( int signo );
		void termSenders();
		void termReceivers();

		void initIterators() noexcept;
		void startReceivers();

		enum Timer
		{
			tmStep
		};

	private:
		UNetExchange();
		timeout_t initPause = { 0 };
		UniSetTypes::uniset_rwmutex mutex_start;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat = { UniSetTypes::DefaultObjectId };
		timeout_t maxHeartBeat = { 10 };
		IOController::IOStateList::iterator itHeartBeat;
		UniSetTypes::ObjectId test_id = { UniSetTypes::DefaultObjectId };

		timeout_t steptime = { 1000 };    /*!< периодичность вызова step, [мсек] */

		std::atomic_bool activated = { false };
		timeout_t activateTimeout = { 20000 }; // msec

		struct ReceiverInfo
		{
			ReceiverInfo() noexcept: r1(nullptr), r2(nullptr),
				sidRespond(UniSetTypes::DefaultObjectId),
				respondInvert(false),
				sidLostPackets(UniSetTypes::DefaultObjectId),
				sidChannelNum(UniSetTypes::DefaultObjectId)
			{}

			ReceiverInfo( const std::shared_ptr<UNetReceiver>& _r1, const std::shared_ptr<UNetReceiver>& _r2 ) noexcept:
				r1(_r1), r2(_r2),
				sidRespond(UniSetTypes::DefaultObjectId),
				respondInvert(false),
				sidLostPackets(UniSetTypes::DefaultObjectId),
				sidChannelNum(UniSetTypes::DefaultObjectId)
			{}

			std::shared_ptr<UNetReceiver> r1;    /*!< приём по первому каналу */
			std::shared_ptr<UNetReceiver> r2;    /*!< приём по второму каналу */

			void step(const std::shared_ptr<SMInterface>& shm, const std::string& myname, std::shared_ptr<DebugStream>& log ) noexcept;

			inline void setRespondID( UniSetTypes::ObjectId id, bool invert = false ) noexcept
			{
				sidRespond = id;
				respondInvert = invert;
			}
			inline void setLostPacketsID( UniSetTypes::ObjectId id ) noexcept
			{
				sidLostPackets = id;
			}
			inline void setChannelNumID( UniSetTypes::ObjectId id ) noexcept
			{
				sidChannelNum = id;
			}

			inline void initIterators( const std::shared_ptr<SMInterface>& shm ) noexcept
			{
				shm->initIterator(itLostPackets);
				shm->initIterator(itRespond);
				shm->initIterator(itChannelNum);
			}

			// Сводная информация по двум каналам
			// сумма потерянных пакетов и наличие связи
			// хотя бы по одному каналу, номер рабочего канала
			// ( реализацию см. ReceiverInfo::step() )
			UniSetTypes::ObjectId sidRespond;
			IOController::IOStateList::iterator itRespond;
			bool respondInvert = { false };
			UniSetTypes::ObjectId sidLostPackets;
			IOController::IOStateList::iterator itLostPackets;
			UniSetTypes::ObjectId sidChannelNum;
			IOController::IOStateList::iterator itChannelNum;
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
// -----------------------------------------------------------------------------
#endif // UNetExchange_H_
// -----------------------------------------------------------------------------
