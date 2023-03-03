// -------------------------------------------------------------------------
#ifndef MBSlave_H_
#define MBSlave_H_
// -------------------------------------------------------------------------
#include <map>
#include <unordered_set>
#include <string>
#include <random>
#include "modbus/ModbusRTUSlaveSlot.h"

// -------------------------------------------------------------------------
/*! Ничего не делающая реализация MBSlave для тестирования */
class MBSlave
{
    public:
        MBSlave( const std::unordered_set<uniset::ModbusRTU::ModbusAddr>& vaddr, const std::string& dev, const std::string& speed, bool use485 = false );
        ~MBSlave();

        inline void setVerbose( bool state )
        {
            verbose = state;
        }

        inline void setReply( long val )
        {
            replyVal = val;
        }
        inline void setReply2( long val )
        {
            replyVal2 = val;
        }
        inline void setReply3( long val )
        {
            replyVal3 = val;
        }

        void setRandomReply( long min, long max );

        void execute();    /*!< основной цикл работы */

        void setLog( std::shared_ptr<DebugStream> dlog );

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

        uniset::ModbusRTU::mbErrCode diagnostics( uniset::ModbusRTU::DiagnosticMessage& query,
                uniset::ModbusRTU::DiagnosticRetMessage& reply );

        uniset::ModbusRTU::mbErrCode read4314( uniset::ModbusRTU::MEIMessageRDI& query,
                                               uniset::ModbusRTU::MEIMessageRetRDI& reply );

        /*! интерфейс ModbusRTUSlave для обмена по RS */
        uniset::ModbusRTUSlaveSlot* rscomm;
        std::unordered_set<uniset::ModbusRTU::ModbusAddr> vaddr;  /*!< адреса на которые отвечаем */

        bool verbose;
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
        long replyVal;
        long replyVal2;
        long replyVal3;
    private:

};
// -------------------------------------------------------------------------
#endif // MBSlave_H_
// -------------------------------------------------------------------------
