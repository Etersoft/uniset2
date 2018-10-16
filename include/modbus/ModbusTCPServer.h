// -------------------------------------------------------------------------
#ifndef ModbusTCPServer_H_
#define ModbusTCPServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
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
namespace uniset
{
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
			ModbusTCPServer( const std::string& addr, int port = 502 );
			virtual ~ModbusTCPServer();

			/*! Запуск сервера. Функция не возвращет управление.
			 * Но может быть прервана вызовом terminate()
			 * \return FALSE - если не удалось запустить
			 */
			bool run( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr );

			/*! Асинхронный запуск сервера (создаётся отдельный поток)
			 * \return TRUE - если поток успешно удалось запустить
			 */
			bool async_run( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr );

			/*! остановить поток выполнения (см. run или async_run) */
			virtual void terminate() override;

			virtual bool isActive() const override;

			void setMaxSessions( size_t num );
			size_t getMaxSessions() const noexcept;

			/*! установить timeout для поддержания соединения с "клиентом" (Default: 10 сек) */
			void setSessionTimeout( timeout_t msec );
			timeout_t getSessionTimeout() const noexcept;

			/*! текущее количество подключений */
			size_t getCountSessions() const noexcept;

			void setIgnoreAddrMode( bool st );
			bool getIgnoreAddrMode() const noexcept;

			// Сбор статистики по соединениям...
			struct SessionInfo
			{
				SessionInfo( const std::string& a, size_t ask ): iaddr(a), askCount(ask) {}

				std::string iaddr;
				size_t askCount;
			};

			typedef std::list<SessionInfo> Sessions;

			void getSessions( Sessions& lst );

			std::string getInetAddress() const noexcept;
			int getInetPort() const noexcept;

			// статистика
			size_t getConnectionCount() const noexcept;

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
			timeout_t getTimer() const noexcept;

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

			int port = { 0 };
			std::string iaddr;
			std::string myname;
			std::queue<unsigned char> qrecv;
			ModbusRTU::MBAPHeader curQueryHeader;

			std::mutex sMutex;
			typedef std::list<std::shared_ptr<ModbusTCPSession>> SessionList;
			SessionList slist;

			bool ignoreAddr = { false };

			size_t maxSessions = { 100 };
			size_t sessCount = { 0 };

			// Статистика
			size_t connCount = { 0 }; // количество обработанных соединений

			timeout_t sessTimeout = { 10000 }; // msec

			ev::io io;
			ev::timer ioTimer;
			std::shared_ptr<UTCPSocket> sock;

			const std::unordered_set<ModbusRTU::ModbusAddr>* vmbaddr = { nullptr };
			TimerSignal m_timer_signal;

			timeout_t tmTime_msec = { UniSetTimer::WaitUpTime }; // время по умолчанию для таймера (TimerSignal)
			double tmTime = { 0.0 };

			PassiveTimer ptWait;

		private:
			// транслирование сигналов от Sessions..
			void postReceiveEvent( ModbusRTU::mbErrCode res );
			ModbusRTU::mbErrCode preReceiveEvent( const std::unordered_set<ModbusRTU::ModbusAddr> vaddr, timeout_t tout );
	};
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusTCPServer_H_
// -------------------------------------------------------------------------
