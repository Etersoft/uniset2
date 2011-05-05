// -------------------------------------------------------------------------
#ifndef ModbusClient_H_
#define ModbusClient_H_
// -------------------------------------------------------------------------
#include <string>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
// -------------------------------------------------------------------------
/*!	Modbus client (master) interface
*/
class ModbusClient
{
	public:

		ModbusClient();
		virtual ~ModbusClient();

		// ------------- Modbus-функции ----------------------------------------
		/*! Чтение группы регистров (0x01) 
			\param addr - адрес slave-узла
			\param start - начальный регистр с которого читать
			\param count - сколько регистров читать 
		*/
		ModbusRTU::ReadCoilRetMessage read01( ModbusRTU::ModbusAddr addr,
												ModbusRTU::ModbusData start, ModbusRTU::ModbusData count )
													throw(ModbusRTU::mbException);

		/*! Чтение группы регистров (0x02) 
			\param addr - адрес slave-узла
			\param start - начальный регистр с которого читать
			\param count - сколько регистров читать 
		*/
		ModbusRTU::ReadInputStatusRetMessage read02( ModbusRTU::ModbusAddr addr,
												ModbusRTU::ModbusData start, ModbusRTU::ModbusData count )
													throw(ModbusRTU::mbException);


		/*! Чтение группы регистров (0x03) 
			\param addr - адрес slave-узла
			\param start - начальный регистр с которого читать
			\param count - сколько регистров читать 
		*/
		ModbusRTU::ReadOutputRetMessage read03( ModbusRTU::ModbusAddr addr,
												ModbusRTU::ModbusData start, ModbusRTU::ModbusData count )
													throw(ModbusRTU::mbException);

		/*! Чтение группы регистров (0x04) 
			\param addr - адрес slave-узла
			\param start - начальный регистр с которого читать
			\param count - сколько регистров читать 
		*/
		ModbusRTU::ReadInputRetMessage read04( ModbusRTU::ModbusAddr addr,
												ModbusRTU::ModbusData start, ModbusRTU::ModbusData count )
													throw(ModbusRTU::mbException);

		/*! 0x05 
			\param addr - адрес slave-узла
			\param reg - записываемый регистр
			\param cmd - команда ON | OFF
		*/
		ModbusRTU::ForceSingleCoilRetMessage write05( ModbusRTU::ModbusAddr addr,
															ModbusRTU::ModbusData reg, bool cmd )
																throw(ModbusRTU::mbException);

		/*! Запись одного регистра (0x06) 
			\param addr - адрес slave-узла
			\param reg - записываемый регистр
			\param data	- данные
		*/
		ModbusRTU::WriteSingleOutputRetMessage write06( ModbusRTU::ModbusAddr addr,
															ModbusRTU::ModbusData reg, ModbusRTU::ModbusData data )
																throw(ModbusRTU::mbException);

		/*! Запись группы выходов (0x0F) */
		ModbusRTU::ForceCoilsRetMessage write0F( ModbusRTU::ForceCoilsMessage& msg )
														throw(ModbusRTU::mbException);

		/*! Запись группы регистров (0x10) */
		ModbusRTU::WriteOutputRetMessage write10( ModbusRTU::WriteOutputMessage& msg )
														throw(ModbusRTU::mbException);

		/*! Диагностика (0x08) */
		ModbusRTU::DiagnosticRetMessage diag08( ModbusRTU::ModbusAddr addr,
												ModbusRTU::DiagnosticsSubFunction subfunc,
												ModbusRTU::ModbusData dat=0 )
														throw(ModbusRTU::mbException);

		/*! Modbus Encapsulated Interface 43(0x2B) 
			Read Device Identification 14(0x0E)
		*/
		ModbusRTU::MEIMessageRetRDI read4314( ModbusRTU::ModbusAddr addr,
												ModbusRTU::ModbusByte devID,
												ModbusRTU::ModbusByte objID )
														throw(ModbusRTU::mbException);

		/*! Установить системное время (0x50)
			hour	- часы [0..23]
			min		- минуты [0..59]
			sec		- секунды [0..59]
			day		- день [1..31]
			mon		- месяц [1..12]
			year	- год [0..99]
			century - столетие [19-20]
		*/
		ModbusRTU::SetDateTimeRetMessage setDateTime( ModbusRTU::ModbusAddr addr, 
							ModbusRTU::ModbusByte hour, ModbusRTU::ModbusByte min, ModbusRTU::ModbusByte sec,
							ModbusRTU::ModbusByte day, ModbusRTU::ModbusByte mon, ModbusRTU::ModbusByte year,
							ModbusRTU::ModbusByte century )
								throw(ModbusRTU::mbException);


		/*! Загрузить файл (0x66) 
			\param idFile - идентификатор файла
			\param numpack - номер очередного запрашиваемого пакета
			\param save2filename - имя файла, под которым будет сохранён полученный файл
			\param part_timeout_msec - таймаут на получение очередной части файла.
		*/
		ModbusRTU::FileTransferRetMessage partOfFileTransfer( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusData idFile, 
																ModbusRTU::ModbusData numpack, timeout_t part_timeout_msec=2000 )
																	throw(ModbusRTU::mbException);

		/*! Загрузить файл
			\param idFile - идентификатор файла
			\param save2filename - имя файла, под которым будет сохранён полученный файл
			\param part_timeout_msec - таймаут на получение очередной части файла.
		*/
		void fileTransfer( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusData idFile, 
							const char* save2filename, timeout_t part_timeout_msec=2000 )
														throw(ModbusRTU::mbException);

		// ---------------------------------------------------------------------
		/*! установить время ожидания по умолчанию */
		void setTimeout( timeout_t msec );
		
		/*! Установка паузы после посылки запроса
			\return старое значение
		*/
		int setAfterSendPause( timeout_t msec );

		/*! установить паузу при ожидании символа */
		inline void setSleepPause( timeout_t usec ){ sleepPause_usec = usec; }

		void initLog( UniSetTypes::Configuration* conf, const std::string name, const std::string logfile="" );
		void setLog( DebugStream& dlog );


		inline void setCRCNoCheckit( bool set ){ crcNoCheckit = set; }
		inline bool isCRCNoCheckit(){ return crcNoCheckit; }

		virtual void cleanupChannel(){}

	protected:

		/*! get next data block from channel ot recv buffer 
			\param begin - get from position
			\param buf  - buffer for data
			\param len 	- size of buf
			\return real data lenght ( must be <= len ) 
		*/
		virtual int getNextData( unsigned char* buf, int len )=0;

		/*! set timeout for send/receive data */
		virtual void setChannelTimeout( timeout_t msec )=0;

		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len )=0;

		/*! функция запрос-ответ */
		virtual ModbusRTU::mbErrCode query( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& msg, 
											ModbusRTU::ModbusMessage& reply, timeout_t timeout )=0;

		// -------------------------------------
		/*! посылка запроса */
		virtual ModbusRTU::mbErrCode send( ModbusRTU::ModbusMessage& msg );

		/*! обработка ответа */
		virtual ModbusRTU::mbErrCode recv( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusByte qfunc, 
									ModbusRTU::ModbusMessage& rbuf, timeout_t timeout );

		virtual ModbusRTU::mbErrCode recv_pdu( ModbusRTU::ModbusByte qfunc, 
									ModbusRTU::ModbusMessage& rbuf, timeout_t timeout );



		ModbusRTU::ModbusMessage reply;	/*!< буфер для приёма сообщений */
		ModbusRTU::ModbusMessage qbuf; 	/*!< буфер для посылки сообщений */

		timeout_t replyTimeOut_ms;	/*!< таймаут на ожидание ответа */
		timeout_t aftersend_msec;	/*!< пауза после посылки запроса */
		timeout_t sleepPause_usec; 	/*!< пауза между попытками чтения символа из канала */
		
		bool crcNoCheckit;

		UniSetTypes::uniset_mutex sendMutex;
		DebugStream dlog;

		void printProcessingTime();
		PassiveTimer tmProcessing;
		
		



	private:
};
// -------------------------------------------------------------------------
#endif // ModbusClient_H_
// -------------------------------------------------------------------------
