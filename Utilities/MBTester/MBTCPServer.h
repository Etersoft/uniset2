#ifndef MBTCPServer_H_
#define MBTCPServer_H_
// -------------------------------------------------------------------------
//#include <map>
#include <string>
#include "modbus/ModbusTCPServerSlot.h"

// -------------------------------------------------------------------------
/*! Ничего не делающая реализация MBTCPServer для тестирования */
class MBTCPServer
{
	public:
		MBTCPServer( ModbusRTU::ModbusAddr myaddr, const std::string& inetaddr, int port=502, bool verbose=false );
		~MBTCPServer();

		inline void setVerbose( bool state )
		{
			verbose = state;
		}
		
		inline void setReply( long val )
		{
			replyVal = val;
		}

		inline void setIgnoreAddrMode( bool state )
		{
			if( sslot )
				sslot->setIgnoreAddrMode(state);
		}

		void execute();	/*!< основной цикл работы */
		void setLog( DebugStream& dlog );

	protected:
		// действия при завершении работы
		void sigterm( int signo );

		/*! обработка 0x01 */
		ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query, 
													ModbusRTU::ReadCoilRetMessage& reply );
		/*! обработка 0x02 */		
		ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query, 
													ModbusRTU::ReadInputStatusRetMessage& reply );

		/*! обработка 0x03 */
		ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query, 
													ModbusRTU::ReadOutputRetMessage& reply );

		/*! обработка 0x04 */
		ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query, 
													ModbusRTU::ReadInputRetMessage& reply );

		/*! обработка 0x05 */
		ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query, 
														ModbusRTU::ForceSingleCoilRetMessage& reply );

		/*! обработка 0x0F */
		ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
													ModbusRTU::ForceCoilsRetMessage& reply );


		/*! обработка 0x10 */
		ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query, 
														ModbusRTU::WriteOutputRetMessage& reply );

		/*! обработка 0x06 */
		ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query, 
														ModbusRTU::WriteSingleOutputRetMessage& reply );


		ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query, 
											ModbusRTU::DiagnosticRetMessage& reply );
		
		ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query, 
														ModbusRTU::MEIMessageRetRDI& reply );

		/*! обработка запросов на чтение ошибок */
		ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query, 
															ModbusRTU::JournalCommandRetMessage& reply );

		/*! обработка запроса на установку времени */
		ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query, 
															ModbusRTU::SetDateTimeRetMessage& reply );

		/*! обработка запроса удалённого сервиса */
		ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query, 
															ModbusRTU::RemoteServiceRetMessage& reply );

		ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query, 
															ModbusRTU::FileTransferRetMessage& reply );


		/*! интерфейс ModbusSlave для обмена по RS */
		ModbusTCPServerSlot* sslot;
		ModbusRTU::ModbusAddr addr;			/*!< адрес данного узла */

		bool verbose;
		long replyVal;
#if 0		
		typedef std::map<ModbusRTU::mbErrCode,unsigned int> ExchangeErrorMap;
		ExchangeErrorMap errmap; 	/*!< статистика обмена */
		ModbusRTU::mbErrCode prev;


		// можно было бы сделать unsigned, но аналоговые датчики у нас имеют 
		// тип long. А это число передаётся в графику в виде аналогового датчика
		long askCount;	/*!< количество принятых запросов */


		typedef std::map<int,std::string> FileList;
		FileList flist;
#endif 
		
	private:
		
};
// -------------------------------------------------------------------------
#endif // MBTCPServer_H_
// -------------------------------------------------------------------------
