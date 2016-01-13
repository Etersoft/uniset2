// -------------------------------------------------------------------------
#ifndef ModbusTCPSession_H_
#define ModbusTCPSession_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <unordered_map>
#include <ev++.h>
#include "ModbusServerSlot.h"
#include "ModbusServer.h"
#include "PassiveTimer.h"
#include "USocket.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
/*!
 * \brief The ModbusTCPSession class
 * Класс рассчитан на совместную работу с ModbusTCPServer, т.к. построен на основе libev,
 * и главный цикл (default_loop) находиться там.
 *
 * Текущая реализация не доведена до совершенства использования "событий".
 * И рассчитывает, что данные от клиента приходят все сразу, а так как сокеты не блокирующие,
 * то попыток чтения делается несколько с небольшой паузой, что нехорошо, т.к. отнимает время у
 * других "клиентов", ведь сервер по сути однопоточный (!)
 * Альтернативной реализацией могло быть быть.. чтение по событиям и складывание в отдельную очередь,
 * а обработку делать по мере достаточного накопления данных во входной очереди, но это требует асинхронный
 * парсинг данных протокола modbus (т.е. мы анализируем очередной байт и решаем сколько нам нужно ещё
 * "подождать" данных.. чтобы пойти на следующий шаг), это в результате будет слишком сложная реализация.
 * В конце-концов пока нет рассчёта на >1000 подключений (хотя libev позволяет держать >10k).
 */
class ModbusTCPSession:
	public ModbusServerSlot,
	public ModbusServer
{
	public:

		ModbusTCPSession( int sock, const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, timeout_t timeout );
		virtual ~ModbusTCPSession();

		void cleanInputStream();

		virtual void cleanupChannel() override;
		virtual void terminate() override;

		typedef sigc::slot<void, ModbusTCPSession*> FinalSlot;

		void connectFinalSession( FinalSlot sl );

		inline std::string getClientAddress()
		{
			return caddr;
		}

		void setSessionTimeout( double t );

		// запуск обработки входящих запросов
		void run();

		virtual bool isAcive() override;

	protected:

		virtual ModbusRTU::mbErrCode realReceive( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, timeout_t msecTimeout ) override;

		void callback( ev::io& watcher, int revents );
		void onTimeout( ev::timer& watcher, int revents );
		virtual void readEvent( ev::io& watcher );
		virtual void writeEvent( ev::io& watcher );
		virtual void final();

		virtual size_t getNextData( unsigned char* buf, int len ) override;
		virtual void setChannelTimeout( timeout_t msec );
		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) override;
		virtual ModbusRTU::mbErrCode tcp_processing(ModbusTCP::MBAPHeader& mhead );
		virtual ModbusRTU::mbErrCode pre_send_request( ModbusRTU::ModbusMessage& request ) override;
		virtual ModbusRTU::mbErrCode post_send_request( ModbusRTU::ModbusMessage& request ) override;

		virtual ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query,
				ModbusRTU::ReadCoilRetMessage& reply );

		virtual ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query,
				ModbusRTU::ReadInputStatusRetMessage& reply );

		virtual ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query,
				ModbusRTU::ReadOutputRetMessage& reply );

		virtual ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query,
				ModbusRTU::ReadInputRetMessage& reply );

		virtual ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
				ModbusRTU::ForceSingleCoilRetMessage& reply );

		virtual ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
				ModbusRTU::WriteSingleOutputRetMessage& reply );

		virtual ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
				ModbusRTU::ForceCoilsRetMessage& reply );

		virtual ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
				ModbusRTU::WriteOutputRetMessage& reply );

		virtual ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query,
				ModbusRTU::DiagnosticRetMessage& reply );

		virtual ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query,
											   ModbusRTU::MEIMessageRetRDI& reply );

		virtual ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query,
				ModbusRTU::JournalCommandRetMessage& reply );

		virtual ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query,
				ModbusRTU::SetDateTimeRetMessage& reply );

		virtual ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query,
				ModbusRTU::RemoteServiceRetMessage& reply );

		virtual ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query,
				ModbusRTU::FileTransferRetMessage& reply );

	private:
		std::queue<unsigned char> qrecv;
		ModbusTCP::MBAPHeader curQueryHeader;
		std::unordered_set<ModbusRTU::ModbusAddr> vaddr;
		PassiveTimer ptTimeout;
		timeout_t timeout = { 0 };
		ModbusRTU::ModbusMessage buf;

		ev::io  io;
		std::shared_ptr<USocket> sock;
		std::queue<UTCPCore::Buffer*> qsend;
		ev::timer ioTimeout;
		double sessTimeout = { 10.0 };

		bool ignoreAddr = { false };
		std::string peername = { "" };

		std::string caddr = { "" };

		FinalSlot slFin;

		std::atomic_bool cancelled = { false };
};
// -------------------------------------------------------------------------
#endif // ModbusTCPSession_H_
// -------------------------------------------------------------------------
