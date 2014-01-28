// -------------------------------------------------------------------------
#ifndef ModbusRTUSlave_H_
#define ModbusRTUSlave_H_
// -------------------------------------------------------------------------
#include <string>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ComPort.h"
#include "ModbusTypes.h"
#include "ModbusServer.h"
// -------------------------------------------------------------------------
/*!    Modbus RTU slave mode  
    Класс не самостоятельный и содержит "чисто" виртуальные функции
    для реализации ответов на запросы.

    \todo Разобратся с тем как отвечать на неправильные запросы! 
        Формат ответных сообщений!!! Коды ошибок!!!
    \todo Доработать terminate, чтобы можно было прервать ожидание
*/
class ModbusRTUSlave:
    public ModbusServer
{
    public:
        ModbusRTUSlave( const std::string& dev, bool use485=false, bool tr_ctl=false  );
        ModbusRTUSlave( ComPort* com );
        virtual ~ModbusRTUSlave();
        
        void setSpeed( ComPort::Speed s );
        void setSpeed( const std::string& s );
        ComPort::Speed getSpeed();

        virtual ModbusRTU::mbErrCode receive( ModbusRTU::ModbusAddr addr, timeout_t msecTimeout );

        virtual void cleanupChannel(){ if(port) port->cleanupChannel(); }

        virtual void terminate();
        
    protected:

        // realisation (see ModbusServer.h)
        virtual int getNextData( unsigned char* buf, int len );
        virtual void setChannelTimeout( timeout_t msec );
        virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len );

        std::string dev;    /*!< устройство */
        ComPort* port;        /*!< устройство для работы с COM-портом */
        bool myport;

    private:

};
// -------------------------------------------------------------------------
#endif // ModbusRTUSlave_H_
// -------------------------------------------------------------------------
