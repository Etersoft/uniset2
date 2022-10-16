// -------------------------------------------------------------------------
#ifndef ModbusRTUMaster_H_
#define ModbusRTUMaster_H_
// -------------------------------------------------------------------------
#include <string>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ComPort.h"
#include "ModbusTypes.h"
#include "ModbusClient.h"
//-----------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    /*!    Modbus RTU master mode
        \todo Добавить ведение статистики по ошибкам
    */
    class ModbusRTUMaster:
        public ModbusClient
    {
        public:

            ModbusRTUMaster( ComPort* com );
            ModbusRTUMaster( const std::string& dev, bool use485 = false, bool tr_ctl = false );
            virtual ~ModbusRTUMaster();

            virtual void cleanupChannel() override;

            void setSpeed( ComPort::Speed s );
            void setSpeed( const std::string& s );
            ComPort::Speed getSpeed();

            void setParity( ComPort::Parity parity );
            ComPort::Parity getParity();
            void setCharacterSize( ComPort::CharacterSize csize );
            ComPort::CharacterSize getCharacterSize();
            void setStopBits( ComPort::StopBits sBit );
            ComPort::StopBits getStopBits();

            timeout_t getTimeout() const;

        protected:

            /*! get next data block from channel ot recv buffer
                \param begin - get from position
                \param buf  - buffer for data
                \param len     - size of buf
                \return real data lenght ( must be <= len )
            */
            virtual size_t getNextData( unsigned char* buf, size_t len ) override;

            /*! set timeout for send/receive data */
            virtual void setChannelTimeout( timeout_t msec ) override;

            virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, size_t len ) override;

            /*! функция запрос-ответ */
            virtual ModbusRTU::mbErrCode query( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& msg,
                                                ModbusRTU::ModbusMessage& reply, timeout_t timeout ) override;


            std::string dev;         /*!< устройство */
            ComPort* port;            /*!< устройство для работы с COM-портом */
            bool myport;

        private:
    };
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusRTUMaster_H_
// -------------------------------------------------------------------------
