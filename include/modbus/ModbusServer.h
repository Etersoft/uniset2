// -------------------------------------------------------------------------
#ifndef ModbusServer_H_
#define ModbusServer_H_
// -------------------------------------------------------------------------
#include <string>

#include "Debug.h"
#include "Mutex.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
// -------------------------------------------------------------------------
/*!    Modbus server interface */
class ModbusServer
{
    public:
        ModbusServer();
        virtual ~ModbusServer();

        void initLog( UniSetTypes::Configuration* conf, const std::string& name, const std::string& logfile="" );
        void setLog( DebugStream& dlog );


        /*! обработать очередное сообщение 
            \param addr         - адрес для которого принимать сообщения
            \param msecTimeout     - время ожидания прихода очередного сообщения в мсек.
            \return Возвращает код ошибки из ModbusRTU::mbErrCode
        */
        virtual ModbusRTU::mbErrCode receive( ModbusRTU::ModbusAddr addr, timeout_t msecTimeout )=0;


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
        inline void setSleepPause( timeout_t usec ){ sleepPause_usec = usec; }

        inline void setCRCNoCheckit( bool set ){ crcNoCheckit = set; }
        inline bool isCRCNoCheckit(){ return crcNoCheckit; }

        inline void setBroadcastMode( bool set ){ onBroadcast = set; }
        inline bool getBroadcastMode(){ return onBroadcast; }

        inline void setCleanBeforeSend( bool set ){ cleanBeforeSend = set; }
        inline bool getCleanBeforeSend(){ return cleanBeforeSend; }

        /*! Вспомогательная функция реализующая обработку запроса на установку времени.
            Основана на использовании gettimeofday и settimeofday.
        */
        static ModbusRTU::mbErrCode replySetDateTime( ModbusRTU::SetDateTimeMessage& query, 
                                                        ModbusRTU::SetDateTimeRetMessage& reply,
                                                        DebugStream* dlog=0 );


        /*! Вспомогательная функция реализующая обработку передачи файла 
            \param fname - запрашиваемый файл.
            \param query - запрос
            \param reply - ответ
        */
        static ModbusRTU::mbErrCode replyFileTransfer( const std::string& fname,
                                                            ModbusRTU::FileTransferMessage& query, 
                                                            ModbusRTU::FileTransferRetMessage& reply,
                                                            DebugStream* dlog=0 );

        virtual void cleanupChannel(){}
        virtual void terminate(){}

    protected:

        /*! Обработка запроса на чтение данных (0x01).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query, 
                                                            ModbusRTU::ReadCoilRetMessage& reply )=0;
        /*! Обработка запроса на чтение данных (0x02).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query, 
                                                            ModbusRTU::ReadInputStatusRetMessage& reply )=0;
    
        /*! Обработка запроса на чтение данных (0x03).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query, 
                                                            ModbusRTU::ReadOutputRetMessage& reply )=0;

        /*! Обработка запроса на чтение данных (0x04).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query, 
                                                            ModbusRTU::ReadInputRetMessage& reply )=0;

        /*! Обработка запроса на запись данных (0x05).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query, 
                                                        ModbusRTU::ForceSingleCoilRetMessage& reply )=0;


        /*! Обработка запроса на запись данных (0x06).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query, 
                                                        ModbusRTU::WriteSingleOutputRetMessage& reply )=0;

        /*! Обработка запроса на запись данных (0x0F).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
                                                        ModbusRTU::ForceCoilsRetMessage& reply )=0;

        /*! Обработка запроса на запись данных (0x10).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query, 
                                                        ModbusRTU::WriteOutputRetMessage& reply )=0;

        
        /*! Обработка запроса на запись данных (0x08).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query, 
                                                        ModbusRTU::DiagnosticRetMessage& reply )=0;

        /*! Обработка запроса 43(0x2B).
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query, 
                                                        ModbusRTU::MEIMessageRetRDI& reply )=0;


        /*! Обработка запроса по журналу (0x65)
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query, 
                                                            ModbusRTU::JournalCommandRetMessage& reply )=0;


        /*! Обработка запроса по установке даты и времени (0x50)
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query, 
                                                            ModbusRTU::SetDateTimeRetMessage& reply )=0;


        /*! Вызов удалённого сервиса (0x53)
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query, 
                                                            ModbusRTU::RemoteServiceRetMessage& reply )=0;


        /*! Передача файла (0x66)
            \param query - запрос
            \param reply - ответ. Заполняется в обработчике.
            \return Результат обработки
        */
        virtual ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query, 
                                                            ModbusRTU::FileTransferRetMessage& reply )=0;

        /*! get next data block from channel ot recv buffer 
            \param begin - get from position
            \param buf  - buffer for data
            \param len     - size of buf
            \return real data lenght ( must be <= len ) 
        */
        virtual int getNextData( unsigned char* buf, int len )=0;
        
        virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len )=0;
        

        /*! set timeout for receive data */
        virtual void setChannelTimeout( timeout_t msec )=0;

        /*! послать сообщение(ответ) в канал */
        virtual ModbusRTU::mbErrCode send( ModbusRTU::ModbusMessage& buf );

        virtual ModbusRTU::mbErrCode pre_send_request( ModbusRTU::ModbusMessage& request ){ return ModbusRTU::erNoError; }
        virtual ModbusRTU::mbErrCode post_send_request( ModbusRTU::ModbusMessage& request ){ return ModbusRTU::erNoError; }

        // default processing
        virtual ModbusRTU::mbErrCode processing( ModbusRTU::ModbusMessage& buf );

        /*! принять сообщение из канала */
        ModbusRTU::mbErrCode recv( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& buf, timeout_t timeout );
        ModbusRTU::mbErrCode recv_pdu( ModbusRTU::ModbusMessage& rbuf, timeout_t timeout );

        UniSetTypes::uniset_mutex recvMutex;
        timeout_t recvTimeOut_ms;        /*!< таймаут на приём */
        timeout_t replyTimeout_ms;    /*!< таймаут на формирование ответа */
        timeout_t aftersend_msec;        /*!< пауза после посылки ответа */
        timeout_t sleepPause_usec;     /*!< пауза между попытками чтения символа из канала */
        bool onBroadcast;        /*!< включен режим работы с broadcst-сообщениями */
        bool crcNoCheckit;
        bool cleanBeforeSend;

        void printProcessingTime();
        PassiveTimer tmProcessing;

        DebugStream dlog;

    private:

};
// -------------------------------------------------------------------------
#endif // ModbusServer_H_
// -------------------------------------------------------------------------
