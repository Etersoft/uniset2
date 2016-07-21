// -------------------------------------------------------------------------
#ifndef ModbusTCPServer_H_
#define ModbusTCPServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <cc++/socket.h>
#include <ev++.h>
#include <sigc++/sigc++.h>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
#include "ModbusServer.h"
#include "ModbusTCPSession.h"
#include "UTCPSocket.h"
#include "EventLoopServer.h"
// -------------------------------------------------------------------------
/*! ModbusTCPServer
 * Реализация сервера на основе libev. Подерживается "много" соединений (постоянных).
 * Хоть класс и наследуется от ModbusServer на самом деле он не реализует его функции,
 * каждое соединение обслуживается классом ModbusTCPSession.
 * Но собственно реализаия функций одна на всех, это следует учитывать при реализации обработчиков,
 * т.к.из многих "соединений" будут вызываться одни и теже обработатчики.
*/
class ModbusTCPServer:
	public EventLoopServer,
	public ModbusServer
{
	public:
		ModbusTCPServer( ost::InetAddress& ia, int port = 502 );
		virtual ~ModbusTCPServer();

		/*! Запуск сервера
		 * \param thread - создавать ли отдельный поток
		 */
		void run( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, bool thread = false );

		virtual bool isActive() override;

		void setMaxSessions( size_t num );
		inline size_t getMaxSessions()
		{
			return maxSessions;
		}

		/*! установить timeout для поддержания соединения с "клиентом" (Default: 10 сек) */
		void setSessionTimeout( timeout_t msec );
		inline timeout_t getSessionTimeout()
		{
			return sessTimeout;
		}

		/*! текущее количество подключений */
		size_t getCountSessions();

		inline void setIgnoreAddrMode( bool st )
		{
			ignoreAddr = st;
		}
		inline bool getIgnoreAddrMode()
		{
			return ignoreAddr;
		}

		virtual void terminate();

		// Сбор статистики по соединениям...
		struct SessionInfo
		{
			SessionInfo( const std::string& a, unsigned int ask ): iaddr(a), askCount(ask) {}

			std::string iaddr;
			unsigned int askCount;
		};

		typedef std::list<SessionInfo> Sessions;

		void getSessions( Sessions& lst );

		inline ost::InetAddress getInetAddress()
		{
			return iaddr;
		}
		inline ost::tpport_t getInetPort()
		{
			return port;
		}

		// -------------------------------------------------
		// Таймер.
		// Т.к. eventLoop() "бесконечный", то сделана возможность
		// подключиться к сигналу "таймера", например для обновления статистики по сессиям
		// \warning Следует иметь ввиду, что обработчик сигнала, должен быть максимально коротким
		// т.к. на это время останавливается работа основного потока (eventLoop)
		// -------------------------------------------------
		typedef sigc::signal<void> TimerSignal;
		TimerSignal signal_timer();

		void setTimer( timeout_t msec );
		inline timeout_t getTimer()
		{
			return tmTime;
		}

	protected:

		// ожидание (при этом время отдаётся eventloop-у)
		virtual void iowait( timeout_t msec ) override;

		// функция receive пока не поддерживается...
		virtual ModbusRTU::mbErrCode realReceive( const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr, timeout_t msecTimeout ) override;

		virtual void evprepare() override;
		virtual void evfinish() override;

		virtual void ioAccept(ev::io& watcher, int revents);
		void onTimer( ev::timer& t, int revents );

		void sessionFinished( const ModbusTCPSession* s );

		virtual size_t getNextData( unsigned char* buf, int len ) override
		{
			return 0;
		}

		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) override
		{
			return  ModbusRTU::erHardwareError;
		}

		/*! set timeout for receive data */
		virtual void setChannelTimeout( timeout_t msec ) override {};

		ost::tpport_t port = { 0 };
		ost::InetAddress iaddr;
		std::string myname;
		std::queue<unsigned char> qrecv;
		ModbusRTU::ADUHeader curQueryHeader;

		std::mutex sMutex;
		typedef std::list<std::shared_ptr<ModbusTCPSession>> SessionList;
		SessionList slist;

		bool ignoreAddr = { false };

		size_t maxSessions = { 100 };
		size_t sessCount = { 0 };

		timeout_t sessTimeout = { 10000 }; // msec

		ev::io io;
		ev::timer ioTimer;
		std::shared_ptr<UTCPSocket> sock;

		const std::unordered_set<ModbusRTU::ModbusAddr>* vmbaddr = { nullptr };
		TimerSignal m_timer_signal;

		timeout_t tmTime_msec = { TIMEOUT_INF }; // время по умолчанию для таймера (TimerSignal)
		double tmTime = { 0.0 };

		PassiveTimer ptWait;

	private:
		// транслирование сигналов от Sessions..
		void postReceiveEvent( ModbusRTU::mbErrCode res );
		ModbusRTU::mbErrCode preReceiveEvent( const std::unordered_set<ModbusRTU::ModbusAddr> vaddr, timeout_t tout );
};
// -------------------------------------------------------------------------
#endif // ModbusTCPServer_H_
// -------------------------------------------------------------------------
