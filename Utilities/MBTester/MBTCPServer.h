#ifndef MBTCPServer_H_
#define MBTCPServer_H_
// -------------------------------------------------------------------------
#include <unordered_set>
#include <string>
#include <random>
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

        void setRandomReply( long min, long max );

        void setFreezeReply( const std::unordered_map<uint16_t, uint16_t>& );

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
        uniset::ModbusRTU::mbErrCode readCoilStatus( const uniset::ModbusRTU::ReadCoilMessage& query,
                uniset::ModbusRTU::ReadCoilRetMessage& reply );
        /*! обработка 0x02 */
        uniset::ModbusRTU::mbErrCode readInputStatus( const uniset::ModbusRTU::ReadInputStatusMessage& query,
                uniset::ModbusRTU::ReadInputStatusRetMessage& reply );

        /*! обработка 0x03 */
        uniset::ModbusRTU::mbErrCode readOutputRegisters( const uniset::ModbusRTU::ReadOutputMessage& query,
                uniset::ModbusRTU::ReadOutputRetMessage& reply );

        /*! обработка 0x04 */
        uniset::ModbusRTU::mbErrCode readInputRegisters( const uniset::ModbusRTU::ReadInputMessage& query,
                uniset::ModbusRTU::ReadInputRetMessage& reply );

        /*! обработка 0x05 */
        uniset::ModbusRTU::mbErrCode forceSingleCoil( const uniset::ModbusRTU::ForceSingleCoilMessage& query,
                uniset::ModbusRTU::ForceSingleCoilRetMessage& reply );

        /*! обработка 0x0F */
        uniset::ModbusRTU::mbErrCode forceMultipleCoils( const uniset::ModbusRTU::ForceCoilsMessage& query,
                uniset::ModbusRTU::ForceCoilsRetMessage& reply );


        /*! обработка 0x10 */
        uniset::ModbusRTU::mbErrCode writeOutputRegisters( const uniset::ModbusRTU::WriteOutputMessage& query,
                uniset::ModbusRTU::WriteOutputRetMessage& reply );

        /*! обработка 0x06 */
        uniset::ModbusRTU::mbErrCode writeOutputSingleRegister( const uniset::ModbusRTU::WriteSingleOutputMessage& query,
                uniset::ModbusRTU::WriteSingleOutputRetMessage& reply );


        uniset::ModbusRTU::mbErrCode diagnostics( const uniset::ModbusRTU::DiagnosticMessage& query,
                uniset::ModbusRTU::DiagnosticRetMessage& reply );

        uniset::ModbusRTU::mbErrCode read4314( const uniset::ModbusRTU::MEIMessageRDI& query,
                                               uniset::ModbusRTU::MEIMessageRetRDI& reply );

        /*! обработка запросов на чтение ошибок */
        uniset::ModbusRTU::mbErrCode journalCommand( const uniset::ModbusRTU::JournalCommandMessage& query,
                uniset::ModbusRTU::JournalCommandRetMessage& reply );

        /*! обработка запроса на установку времени */
        uniset::ModbusRTU::mbErrCode setDateTime( const uniset::ModbusRTU::SetDateTimeMessage& query,
                uniset::ModbusRTU::SetDateTimeRetMessage& reply );

        /*! обработка запроса удалённого сервиса */
        uniset::ModbusRTU::mbErrCode remoteService( const uniset::ModbusRTU::RemoteServiceMessage& query,
                uniset::ModbusRTU::RemoteServiceRetMessage& reply );

        uniset::ModbusRTU::mbErrCode fileTransfer( const uniset::ModbusRTU::FileTransferMessage& query,
                uniset::ModbusRTU::FileTransferRetMessage& reply );


        /*! интерфейс ModbusSlave для обмена по RS */
        uniset::ModbusTCPServerSlot* sslot;
        std::unordered_set<uniset::ModbusRTU::ModbusAddr> vaddr; /*!< адреса данного узла */

        bool verbose = { false };
        long replyVal = { -1 };

        std::unordered_map<uint16_t, uint16_t> reglist = {};

        std::random_device rnd;
        std::unique_ptr<std::mt19937> gen = { nullptr };
        std::unique_ptr<std::uniform_int_distribution<>> rndgen = { nullptr };
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
