// -------------------------------------------------------------------------
#ifndef ModbusClient_H_
#define ModbusClient_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    /*!    Modbus client (master) interface
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

                throw ModbusRTU::mbException
            */
            ModbusRTU::ReadCoilRetMessage read01( ModbusRTU::ModbusAddr addr,
                                                  ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );

            /*! Чтение группы регистров (0x02)
                \param addr - адрес slave-узла
                \param start - начальный регистр с которого читать
                \param count - сколько регистров читать

                throw ModbusRTU::mbException
            */
            ModbusRTU::ReadInputStatusRetMessage read02( ModbusRTU::ModbusAddr addr,
                    ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );


            /*! Чтение группы регистров (0x03)
                \param addr - адрес slave-узла
                \param start - начальный регистр с которого читать
                \param count - сколько регистров читать

                throw ModbusRTU::mbException
            */
            ModbusRTU::ReadOutputRetMessage read03( ModbusRTU::ModbusAddr addr,
                                                    ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );

            /*! Чтение группы регистров (0x04)
                \param addr - адрес slave-узла
                \param start - начальный регистр с которого читать
                \param count - сколько регистров читать

                throw ModbusRTU::mbException
            */
            ModbusRTU::ReadInputRetMessage read04( ModbusRTU::ModbusAddr addr,
                                                   ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );

            /*! 0x05
                \param addr - адрес slave-узла
                \param reg - записываемый регистр
                \param cmd - команда ON | OFF

                throw ModbusRTU::mbException
            */
            ModbusRTU::ForceSingleCoilRetMessage write05( ModbusRTU::ModbusAddr addr,
                    ModbusRTU::ModbusData reg, bool cmd );

            /*! Запись одного регистра (0x06)
                \param addr - адрес slave-узла
                \param reg - записываемый регистр
                \param data    - данные

                throw ModbusRTU::mbException
            */
            ModbusRTU::WriteSingleOutputRetMessage write06( ModbusRTU::ModbusAddr addr,
                    ModbusRTU::ModbusData reg, ModbusRTU::ModbusData data );

            /*! Запись группы выходов (0x0F) throw ModbusRTU::mbException*/
            ModbusRTU::ForceCoilsRetMessage write0F( ModbusRTU::ForceCoilsMessage& msg );

            /*! Запись группы регистров (0x10) throw ModbusRTU::mbException*/
            ModbusRTU::WriteOutputRetMessage write10( ModbusRTU::WriteOutputMessage& msg );

            /*! Диагностика (0x08) throw ModbusRTU::mbException*/
            ModbusRTU::DiagnosticRetMessage diag08( ModbusRTU::ModbusAddr addr,
                                                    ModbusRTU::DiagnosticsSubFunction subfunc,
                                                    ModbusRTU::ModbusData dat = 0 );

            /*! Modbus Encapsulated Interface 43(0x2B)
                Read Device Identification 14(0x0E)
                throw ModbusRTU::mbException
            */
            ModbusRTU::MEIMessageRetRDI read4314( ModbusRTU::ModbusAddr addr,
                                                  ModbusRTU::ModbusByte devID,
                                                  ModbusRTU::ModbusByte objID );

            /*! Установить системное время (0x50)
                hour    - часы [0..23]
                min        - минуты [0..59]
                sec        - секунды [0..59]
                day        - день [1..31]
                mon        - месяц [1..12]
                year    - год [0..99]
                century - столетие [19-20]

                throw ModbusRTU::mbException
            */
            ModbusRTU::SetDateTimeRetMessage setDateTime( ModbusRTU::ModbusAddr addr,
                    ModbusRTU::ModbusByte hour, ModbusRTU::ModbusByte min, ModbusRTU::ModbusByte sec,
                    ModbusRTU::ModbusByte day, ModbusRTU::ModbusByte mon, ModbusRTU::ModbusByte year,
                    ModbusRTU::ModbusByte century );


            /*! Загрузить файл (0x66)
                \param idFile - идентификатор файла
                \param numpack - номер очередного запрашиваемого пакета
                \param save2filename - имя файла, под которым будет сохранён полученный файл
                \param part_timeout_msec - таймаут на получение очередной части файла.

                throw ModbusRTU::mbException
            */
            ModbusRTU::FileTransferRetMessage partOfFileTransfer( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusData idFile,
                    ModbusRTU::ModbusData numpack, timeout_t part_timeout_msec = 2000 );

            /*! Загрузить файл
                \param idFile - идентификатор файла
                \param save2filename - имя файла, под которым будет сохранён полученный файл
                \param part_timeout_msec - таймаут на получение очередной части файла.

                throw ModbusRTU::mbException
            */
            void fileTransfer( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusData idFile,
                               const std::string& save2filename, timeout_t part_timeout_msec = 2000 );

            // ---------------------------------------------------------------------
            /*! установить время ожидания по умолчанию */
            void setTimeout( timeout_t msec );

            /*! Установка паузы после посылки запроса
                \return старое значение
            */
            int setAfterSendPause( timeout_t msec );

            /*! установить паузу при ожидании символа */
            inline void setSleepPause( timeout_t usec )
            {
                sleepPause_usec = usec;
            }

            void initLog( std::shared_ptr<uniset::Configuration> conf, const std::string& name, const std::string& logfile = "" );
            void setLog( std::shared_ptr<DebugStream> dlog );

            inline void setCRCNoCheckit( bool set )
            {
                crcNoCheckit = set;
            }
            inline bool isCRCNoCheckit() const
            {
                return crcNoCheckit;
            }

            virtual void cleanupChannel() {}

        protected:

            /*! get next data block from channel ot recv buffer
                \param begin - get from position
                \param buf  - buffer for data
                \param len     - size of buf
                \return real data length ( must be <= len )
            */
            virtual size_t getNextData( unsigned char* buf, size_t len ) = 0;

            /*! set timeout for send/receive data */
            virtual void setChannelTimeout( timeout_t msec ) = 0;

            virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, size_t len ) = 0;

            /*! функция запрос-ответ */
            virtual ModbusRTU::mbErrCode query( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& msg,
                                                ModbusRTU::ModbusMessage& qreply, timeout_t timeout ) = 0;

            // -------------------------------------
            /*! посылка запроса */
            virtual ModbusRTU::mbErrCode send( ModbusRTU::ModbusMessage& msg );

            /*! обработка ответа */
            virtual ModbusRTU::mbErrCode recv( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusByte qfunc,
                                               ModbusRTU::ModbusMessage& rbuf, timeout_t timeout );

            virtual ModbusRTU::mbErrCode recv_pdu( ModbusRTU::ModbusByte qfunc,
                                                   ModbusRTU::ModbusMessage& rbuf, timeout_t timeout );

            timeout_t replyTimeOut_ms;    /*!< таймаут на ожидание ответа */
            timeout_t aftersend_msec;    /*!< пауза после посылки запроса */
            timeout_t sleepPause_usec;     /*!< пауза между попытками чтения символа из канала */

            bool crcNoCheckit;

            uniset::uniset_rwmutex sendMutex;
            std::shared_ptr<DebugStream> dlog;

            void printProcessingTime();
            PassiveTimer tmProcessing;

        private:

            ModbusRTU::ModbusMessage qreply;   /*!< буфер для приёма сообщений */
            ModbusRTU::ModbusMessage qbuf;     /*!< буфер для посылки сообщений */
    };
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusClient_H_
// -------------------------------------------------------------------------
