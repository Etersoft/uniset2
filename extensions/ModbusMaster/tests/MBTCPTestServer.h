#ifndef MBTCPTestServer_H_
#define MBTCPTestServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <atomic>
#include <ostream>
#include <unordered_set>
#include "modbus/ModbusTCPServerSlot.h"
#include "VTypes.h"
// -------------------------------------------------------------------------
/*! Реализация MBTCPTestServer для тестирования */
class MBTCPTestServer
{
    public:
        MBTCPTestServer( const std::unordered_set<uniset::ModbusRTU::ModbusAddr>& vaddr, const std::string& inetaddr, int port = 502, bool verbose = false );
        ~MBTCPTestServer();

        inline void setVerbose( bool state )
        {
            verbose = state;
        }

        inline void setReply( uint32_t val )
        {
            replyVal = val;
        }

        void execute();    /*!< основной цикл работы */
        void setLog( std::shared_ptr<DebugStream> dlog );

        inline bool isRunning()
        {
            return ( sslot && sslot->isActive() );
        }

        inline void disableExchange( bool set = true )
        {
            disabled = set;
        }

        inline bool getForceSingleCoilCmd()
        {
            return forceSingleCoilCmd;
        }
        inline int16_t getLastWriteRegister( uint16_t reg )
        {
            return lastWriteRegister[reg].value;
        }
        inline float getLastWriteRegisterF2( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                   };
            uniset::VTypes::F2 f2(data, uniset::VTypes::F2::wsize());
            return (float)f2;
        }
        inline uint16_t getLastWriteRegisterByte( uint16_t reg )
        {
            uniset::VTypes::Byte b((uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value);
            return (uint16_t)b;
        }
        inline float getLastWriteRegisterF2r( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                   };
            uniset::VTypes::F2r f2r(data, uniset::VTypes::F2r::wsize());
            return (float)f2r;
        }
        inline double getLastWriteRegisterF4( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 2].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 3].value
                                                   };
            uniset::VTypes::F4 f4(data, uniset::VTypes::F4::wsize());
            return (double)f4;
        }
        inline int32_t getLastWriteRegisterI2( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                   };
            uniset::VTypes::I2 i2(data, uniset::VTypes::I2::wsize());
            return (int32_t)i2;
        }
        inline int32_t getLastWriteRegisterI2r( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                   };
            uniset::VTypes::I2r i2r(data, uniset::VTypes::I2r::wsize());
            return (int32_t)i2r;
        }
        inline uint32_t getLastWriteRegisterU2( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                   };
            uniset::VTypes::U2 u2(data, uniset::VTypes::U2::wsize());
            return (uint32_t)u2;
        }
        inline uint32_t getLastWriteRegisterU2r( uint16_t reg )
        {
            uniset::ModbusRTU::ModbusData data[] = {(uniset::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                    (uniset::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                   };
            uniset::VTypes::U2r u2r(data, uniset::VTypes::U2r::wsize());
            return (uint32_t)u2r;
        }
        inline long getWriteRegisterCount( uint16_t reg )
        {
            return lastWriteRegister[reg].count;
        }
        inline uniset::ModbusRTU::ForceCoilsMessage getLastForceCoilsQ()
        {
            return lastForceCoilsQ;
        }
        inline uniset::ModbusRTU::WriteOutputMessage getLastWriteOutput()
        {
            return lastWriteOutputQ;
        }

        friend std::ostream& operator<<(std::ostream& os, const MBTCPTestServer* m );

        inline float getF2TestValue()
        {
            return f2_test_value;
        }

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

        bool verbose;
        uint32_t replyVal;
        bool forceSingleCoilCmd;
        struct regData
        {
            int16_t value = {0};
            long count = {0};
            void setValue(int16_t val)
            {
                value = val;
                count++;
            }
        };
        std::unordered_map<int16_t, regData> lastWriteRegister;
        uniset::ModbusRTU::ForceCoilsMessage lastForceCoilsQ;
        uniset::ModbusRTU::WriteOutputMessage lastWriteOutputQ;
        float f2_test_value = {0.0};

#if 0
        typedef std::map<uniset::ModbusRTU::mbErrCode, unsigned int> ExchangeErrorMap;
        ExchangeErrorMap errmap;     /*!< статистика обмена */
        uniset::ModbusRTU::mbErrCode prev;


        // можно было бы сделать unsigned, но аналоговые датчики у нас имеют
        // тип long. А это число передаётся в графику в виде аналогового датчика
        long askCount;    /*!< количество принятых запросов */


        typedef std::map<int, std::string> FileList;
        FileList flist;
#endif

    private:
        bool disabled;
        std::string myname;
};
// -------------------------------------------------------------------------
#endif // MBTCPTestServer_H_
// -------------------------------------------------------------------------
