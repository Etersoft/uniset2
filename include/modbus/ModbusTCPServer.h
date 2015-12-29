// -------------------------------------------------------------------------
#ifndef ModbusTCPServer_H_
#define ModbusTCPServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <cc++/socket.h>
#include <ev++.h>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
#include "ModbusServer.h"
#include "ModbusTCPSession.h"
#include "UTCPStream.h"
// -------------------------------------------------------------------------
/*! ModbusTCP server */
class ModbusTCPServer:
	public ModbusServer
{
	public:
		ModbusTCPServer( ost::InetAddress& ia, int port = 502 );
		virtual ~ModbusTCPServer();

		/*! Однопоточная обработка (каждый запрос последовательно), с разрывом соединения в конце */
		virtual ModbusRTU::mbErrCode receive( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, timeout_t msecTimeout ) override;

		/*! Поддержка большого количества соединений (постоянных) */
		virtual void mainLoop( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr );

		void setMaxSessions( unsigned int num );
		inline unsigned int getMaxSessions()
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
		unsigned getCountSessions();

		inline void setIgnoreAddrMode( bool st )
		{
			ignoreAddr = st;
		}
		inline bool getIgnoreAddrMode()
		{
			return ignoreAddr;
		}

		void cleanInputStream();
		virtual void cleanupChannel() override
		{
			cleanInputStream();
		}

		virtual void terminate() override;

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

	protected:

		virtual void ioAccept(ev::io &watcher, int revents);
		virtual ModbusRTU::mbErrCode pre_send_request( ModbusRTU::ModbusMessage& request ) override;
		virtual ModbusRTU::mbErrCode post_send_request( ModbusRTU::ModbusMessage& request ) override;

		// realisation (see ModbusServer.h)
		virtual size_t getNextData( unsigned char* buf, int len ) override;
		virtual void setChannelTimeout( timeout_t msec ) override;
		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) override;

		virtual ModbusRTU::mbErrCode tcp_processing( UTCPStream& tcp, ModbusTCP::MBAPHeader& mhead );
		void sessionFinished( ModbusTCPSession* s );

		ost::tpport_t port = { 0 };
		ost::InetAddress iaddr;
		std::queue<unsigned char> qrecv;
		ModbusTCP::MBAPHeader curQueryHeader;

		typedef std::list<ModbusTCPSession*> SessionList;
		UniSetTypes::uniset_mutex sMutex;
		SessionList slist;

		bool ignoreAddr = { false };

		unsigned int maxSessions = { 5 };
		unsigned int sessCount = { 0 };

		timeout_t sessTimeout = { 10000 }; // msec

		ev::default_loop* evloop = { 0 };
		ev::io io;
		int sock = { -1 };

		const std::unordered_set<ModbusRTU::ModbusAddr>* vmbaddr;

	private:

		std::atomic_bool cancelled;
};
// -------------------------------------------------------------------------
#endif // ModbusTCPServer_H_
// -------------------------------------------------------------------------
