// -------------------------------------------------------------------------
#ifndef MBSlave_H_
#define MBSlave_H_
// -------------------------------------------------------------------------
#include <map>
#include <string>
#include "modbus/ModbusRTUSlaveSlot.h"

// -------------------------------------------------------------------------
/*! Ничего не делающая реализация MBSlave для тестирования */
class MBSlave
{
    public:
        MBSlave( ModbusRTU::ModbusAddr addr, const std::string& dev, const std::string& speed, bool use485=false );
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

        void execute();    /*!< основной цикл работы */


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

        ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query, 
                                                        ModbusRTU::DiagnosticRetMessage& reply );
        
        ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query, 
                                                        ModbusRTU::MEIMessageRetRDI& reply );

        /*! интерфейс ModbusRTUSlave для обмена по RS */
        ModbusRTUSlaveSlot* rscomm;
        ModbusRTU::ModbusAddr addr;            /*!< адрес данного узла */

        bool verbose;
#if 0        
        typedef std::map<ModbusRTU::mbErrCode,unsigned int> ExchangeErrorMap;
        ExchangeErrorMap errmap;     /*!< статистика обмена */
        ModbusRTU::mbErrCode prev;


        // можно было бы сделать unsigned, но аналоговые датчики у нас имеют 
        // тип long. А это число передаётся в графику в виде аналогового датчика
        long askCount;    /*!< количество принятых запросов */


        typedef std::map<int,std::string> FileList;
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
