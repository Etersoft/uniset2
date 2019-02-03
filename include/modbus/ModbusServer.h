// -------------------------------------------------------------------------
#ifndef ModbusServer_H_
#define ModbusServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <sigc++/sigc++.h>

#include "Debug.h"
#include "Mutex.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
// -------------------------------------------------------------------------
namespace std
{
	template<>
	struct hash<uniset::ModbusRTU::mbErrCode>
	{
		size_t operator()(const uniset::ModbusRTU::mbErrCode& e) const
		{
			return std::hash<size_t>()(e);
		}
	};
}
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	/*!    Modbus server interface */
	class ModbusServer
	{
		public:
			ModbusServer();
			virtual ~ModbusServer();

			void initLog( uniset::Configuration* conf, const std::string& name, const std::string& logfile = "" );
			void setLog( std::shared_ptr<DebugStream> dlog );
			inline std::shared_ptr<DebugStream> log()
			{
				return dlog;
			}

			static std::unordered_set<ModbusRTU::ModbusAddr> addr2vaddr( ModbusRTU::ModbusAddr& mbaddr );

			/*! обработать очередное сообщение
				\param vaddr        - вектор адресов для которых принимать сообщения
				\param msecTimeout  - время ожидания прихода очередного сообщения в мсек.
			    \return Возвращает код ошибки из ModbusRTU::mbErrCode
			*/
			ModbusRTU::mbErrCode receive( const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr, timeout_t msecTimeout );

			// версия с "одним" адресом
			virtual ModbusRTU::mbErrCode receive_one( const ModbusRTU::ModbusAddr addr, timeout_t msec );


			// ---------------------------------------------------------------------------------------
			// сигналы по обработке событий приёма сообщения.
			// ---------------------------------------------------------------------------------------
			// сигнал вызова receive, ДО обработки realReceive()
			// \return ModbusRTU::errNoError, тогда обработка продолжиться.
			typedef sigc::signal<ModbusRTU::mbErrCode, const std::unordered_set<ModbusRTU::ModbusAddr>, timeout_t> PreReceiveSignal;
			PreReceiveSignal signal_pre_receive();

			// сигнал после обработки realReceive()
			typedef sigc::signal<void, ModbusRTU::mbErrCode> PostReceiveSignal;
			PostReceiveSignal signal_post_receive();
			// ---------------------------------------------------------------------------------------

			/*! Проверка входит ли данный адрес в список
			  * \param vaddr - вектор адресов
			  * \param addr - адрес который ищем
			  * \return TRUE - если найден
			  * \warning Если addr=ModbusRTU::BroadcastAddr то всегда возвращается TRUE!
			*/
			static bool checkAddr( const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr, const ModbusRTU::ModbusAddr addr );
			static std::string vaddr2str( const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr );

			/*! Установка паузы после посылки
			    \return старое значение
			*/
			timeout_t setAfterSendPause( timeout_t msec );

			/*! Установка таймаута на формирование ответа
			    \return старое значение
			*/
			timeout_t setReplyTimeout( timeout_t msec );

			/*! установить время ожидания по умолчанию */
			void setRecvTimeout( timeout_t msec );

			/*! установить паузу при ожидании символа */
			void setSleepPause( timeout_t msec );

			void setCRCNoCheckit( bool set );
			bool isCRCNoCheckit() const;

			void setBroadcastMode( bool set );
			bool getBroadcastMode() const;

			void setCleanBeforeSend( bool set );
			bool getCleanBeforeSend() const;

			/*! Вспомогательная функция реализующая обработку запроса на установку времени.
			    Основана на использовании gettimeofday и settimeofday.
			*/
			static ModbusRTU::mbErrCode replySetDateTime( ModbusRTU::SetDateTimeMessage& query,
					ModbusRTU::SetDateTimeRetMessage& reply,
					std::shared_ptr<DebugStream> dlog = nullptr );


			/*! Вспомогательная функция реализующая обработку передачи файла
			    \param fname - запрашиваемый файл.
			    \param query - запрос
			    \param reply - ответ
			*/
			static ModbusRTU::mbErrCode replyFileTransfer( const std::string& fname,
					ModbusRTU::FileTransferMessage& query,
					ModbusRTU::FileTransferRetMessage& reply,
					std::shared_ptr<DebugStream> dlog = nullptr );

			virtual void cleanupChannel() {}
			virtual void terminate() {}

			virtual bool isActive() const = 0;

			// ------------ Статистика ---------------
			typedef std::unordered_map<ModbusRTU::mbErrCode, size_t> ExchangeErrorMap;

			ExchangeErrorMap getErrorMap();
			size_t getErrCount( ModbusRTU::mbErrCode e ) const;
			size_t resetErrCount( ModbusRTU::mbErrCode e, size_t set = 0 );

			size_t getAskCount() const noexcept;
			void resetAskCounter();

		protected:

			virtual void iowait( timeout_t usec );

			/*! реализация получения очередного сообщения */
			virtual ModbusRTU::mbErrCode realReceive( const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr, timeout_t msecTimeout ) = 0;

			/*! Обработка запроса на чтение данных (0x01).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query,
					ModbusRTU::ReadCoilRetMessage& reply ) = 0;
			/*! Обработка запроса на чтение данных (0x02).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query,
					ModbusRTU::ReadInputStatusRetMessage& reply ) = 0;

			/*! Обработка запроса на чтение данных (0x03).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query,
					ModbusRTU::ReadOutputRetMessage& reply ) = 0;

			/*! Обработка запроса на чтение данных (0x04).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query,
					ModbusRTU::ReadInputRetMessage& reply ) = 0;

			/*! Обработка запроса на запись данных (0x05).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
					ModbusRTU::ForceSingleCoilRetMessage& reply ) = 0;


			/*! Обработка запроса на запись данных (0x06).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
					ModbusRTU::WriteSingleOutputRetMessage& reply ) = 0;

			/*! Обработка запроса на запись данных (0x0F).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
					ModbusRTU::ForceCoilsRetMessage& reply ) = 0;

			/*! Обработка запроса на запись данных (0x10).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
					ModbusRTU::WriteOutputRetMessage& reply ) = 0;


			/*! Обработка запроса на запись данных (0x08).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query,
					ModbusRTU::DiagnosticRetMessage& reply ) = 0;

			/*! Обработка запроса 43(0x2B).
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query,
												   ModbusRTU::MEIMessageRetRDI& reply ) = 0;


			/*! Обработка запроса по журналу (0x65)
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query,
					ModbusRTU::JournalCommandRetMessage& reply ) = 0;


			/*! Обработка запроса по установке даты и времени (0x50)
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query,
					ModbusRTU::SetDateTimeRetMessage& reply ) = 0;


			/*! Вызов удалённого сервиса (0x53)
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query,
					ModbusRTU::RemoteServiceRetMessage& reply ) = 0;


			/*! Передача файла (0x66)
			    \param query - запрос
			    \param reply - ответ. Заполняется в обработчике.
			    \return Результат обработки
			*/
			virtual ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query,
					ModbusRTU::FileTransferRetMessage& reply ) = 0;

			/*! get next data block from channel or recv buffer
			    \param begin - get from position
			    \param buf  - buffer for data
			    \param len     - size of buf
			    \return real data lenght ( must be <= len )
			*/
			virtual size_t getNextData( unsigned char* buf, int len ) = 0;

			virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) = 0;

			/*! set timeout for receive data */
			virtual void setChannelTimeout( timeout_t msec ) = 0;

			/*! послать сообщение(ответ) в канал */
			virtual ModbusRTU::mbErrCode send( ModbusRTU::ModbusMessage& buf );

			// Если заголовок не должен использоваться оставляйте request.header.len = 0
			virtual ModbusRTU::mbErrCode make_adu_header( ModbusRTU::ModbusMessage& request )
			{
				return ModbusRTU::erNoError;
			}
			virtual ModbusRTU::mbErrCode post_send_request( ModbusRTU::ModbusMessage& request )
			{
				return ModbusRTU::erNoError;
			}

			// default processing
			virtual ModbusRTU::mbErrCode processing( ModbusRTU::ModbusMessage& buf );

			/*! принять сообщение из канала */
			ModbusRTU::mbErrCode recv(const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr, ModbusRTU::ModbusMessage& buf, timeout_t timeout );
			ModbusRTU::mbErrCode recv_pdu( ModbusRTU::ModbusMessage& rbuf, timeout_t timeout );

			std::timed_mutex recvMutex;
			timeout_t recvTimeOut_ms = { 50 };        /*!< таймаут на приём */
			timeout_t replyTimeout_ms = { 2000 };    /*!< таймаут на формирование ответа */
			timeout_t aftersend_msec = { 0 };        /*!< пауза после посылки ответа */
			timeout_t sleepPause_msec = { 10 };     /*!< пауза между попытками чтения символа из канала */
			bool onBroadcast = { false };        /*!< включен режим работы с broadcst-сообщениями */
			bool crcNoCheckit = { false };
			bool cleanBeforeSend = { false };

			void printProcessingTime();
			PassiveTimer tmProcessing;

			std::shared_ptr<DebugStream> dlog;

			// статистика сервера
			size_t askCount = { 0 };
			ExchangeErrorMap errmap;     /*!< статистика ошибок обмена */

			PreReceiveSignal m_pre_signal;
			PostReceiveSignal m_post_signal;

		private:

	};
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusServer_H_
// -------------------------------------------------------------------------
