#ifndef MBTCPServer_H_
#define MBTCPServer_H_
// -------------------------------------------------------------------------
#include <unordered_set>
#include <string>
#include "modbus/ModbusTCPServerSlot.h"

// -------------------------------------------------------------------------
/*! Ничего не делающая реализация MBTCPServer для тестирования */
class MBTCPServer
{
	public:
		MBTCPServer( const std::unordered_set<uniset::ModbusRTU::ModbusAddr>& myaddr, const std::string& inetaddr, int port = 502, bool verbose = false );
		~MBTCPServer();

		inline void setVerbose( bool state )
		{
			verbose = state;
		}

		inline void setReply( long val )
		{
			replyVal = val;
		}

		inline uniset::timeout_t setAfterSendPause( uniset::timeout_t msec )
		{
			return sslot->setAfterSendPause(msec);
		}

		void execute();    /*!< основной цикл работы */
		void setLog( std::shared_ptr<DebugStream>& dlog );

		void setMaxSessions( size_t max );

	protected:
		// действия при завершении работы
		void sigterm( int signo );

		/*! обработка 0x01 */
		uniset::ModbusRTU::mbErrCode readCoilStatus( uniset::ModbusRTU::ReadCoilMessage& query,
				uniset::ModbusRTU::ReadCoilRetMessage& reply );
		/*! обработка 0x02 */
		uniset::ModbusRTU::mbErrCode readInputStatus( uniset::ModbusRTU::ReadInputStatusMessage& query,
				uniset::ModbusRTU::ReadInputStatusRetMessage& reply );

		/*! обработка 0x03 */
		uniset::ModbusRTU::mbErrCode readOutputRegisters( uniset::ModbusRTU::ReadOutputMessage& query,
				uniset::ModbusRTU::ReadOutputRetMessage& reply );

		/*! обработка 0x04 */
		uniset::ModbusRTU::mbErrCode readInputRegisters( uniset::ModbusRTU::ReadInputMessage& query,
				uniset::ModbusRTU::ReadInputRetMessage& reply );

		/*! обработка 0x05 */
		uniset::ModbusRTU::mbErrCode forceSingleCoil( uniset::ModbusRTU::ForceSingleCoilMessage& query,
				uniset::ModbusRTU::ForceSingleCoilRetMessage& reply );

		/*! обработка 0x0F */
		uniset::ModbusRTU::mbErrCode forceMultipleCoils( uniset::ModbusRTU::ForceCoilsMessage& query,
				uniset::ModbusRTU::ForceCoilsRetMessage& reply );


		/*! обработка 0x10 */
		uniset::ModbusRTU::mbErrCode writeOutputRegisters( uniset::ModbusRTU::WriteOutputMessage& query,
				uniset::ModbusRTU::WriteOutputRetMessage& reply );

		/*! обработка 0x06 */
		uniset::ModbusRTU::mbErrCode writeOutputSingleRegister( uniset::ModbusRTU::WriteSingleOutputMessage& query,
				uniset::ModbusRTU::WriteSingleOutputRetMessage& reply );


		uniset::ModbusRTU::mbErrCode diagnostics( uniset::ModbusRTU::DiagnosticMessage& query,
				uniset::ModbusRTU::DiagnosticRetMessage& reply );

		uniset::ModbusRTU::mbErrCode read4314( uniset::ModbusRTU::MEIMessageRDI& query,
											   uniset::ModbusRTU::MEIMessageRetRDI& reply );

		/*! обработка запросов на чтение ошибок */
		uniset::ModbusRTU::mbErrCode journalCommand( uniset::ModbusRTU::JournalCommandMessage& query,
				uniset::ModbusRTU::JournalCommandRetMessage& reply );

		/*! обработка запроса на установку времени */
		uniset::ModbusRTU::mbErrCode setDateTime( uniset::ModbusRTU::SetDateTimeMessage& query,
				uniset::ModbusRTU::SetDateTimeRetMessage& reply );

		/*! обработка запроса удалённого сервиса */
		uniset::ModbusRTU::mbErrCode remoteService( uniset::ModbusRTU::RemoteServiceMessage& query,
				uniset::ModbusRTU::RemoteServiceRetMessage& reply );

		uniset::ModbusRTU::mbErrCode fileTransfer( uniset::ModbusRTU::FileTransferMessage& query,
				uniset::ModbusRTU::FileTransferRetMessage& reply );


		/*! интерфейс ModbusSlave для обмена по RS */
		uniset::ModbusTCPServerSlot* sslot;
		std::unordered_set<uniset::ModbusRTU::ModbusAddr> vaddr; /*!< адреса данного узла */

		bool verbose = { false };
		long replyVal = { -1 };
#if 0
		typedef std::unordered_map<uniset::ModbusRTU::mbErrCode, unsigned int> ExchangeErrorMap;
		ExchangeErrorMap errmap;     /*!< статистика обмена */
		uniset::ModbusRTU::mbErrCode prev;


		// можно было бы сделать unsigned, но аналоговые датчики у нас имеют
		// тип long. А это число передаётся в графику в виде аналогового датчика
		long askCount;    /*!< количество принятых запросов */


		typedef std::unordered_map<int, std::string> FileList;
		FileList flist;
#endif

	private:

};
// -------------------------------------------------------------------------
#endif // MBTCPServer_H_
// -------------------------------------------------------------------------
