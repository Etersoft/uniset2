// -------------------------------------------------------------------------
#ifndef ModbusRTUSlave_H_
#define ModbusRTUSlave_H_
// -------------------------------------------------------------------------
#include <string>
#include <unordered_set>
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
		ModbusRTUSlave( const std::string& dev, bool use485 = false, bool tr_ctl = false  );
		ModbusRTUSlave( ComPort* com );
		virtual ~ModbusRTUSlave();

		void setSpeed( ComPort::Speed s );
		void setSpeed( const std::string& s );
		ComPort::Speed getSpeed();

		virtual ModbusRTU::mbErrCode receive( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, timeout_t msecTimeout ) override;

		virtual void cleanupChannel() override
		{
			if(port) port->cleanupChannel();
		}

		virtual void terminate() override;
		virtual bool isAcive() override;

	protected:

		// realisation (see ModbusServer.h)
		virtual size_t getNextData( unsigned char* buf, int len ) override;
		virtual void setChannelTimeout( timeout_t msec ) override;
		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) override;

		std::string dev;    /*!< устройство */
		ComPort* port;        /*!< устройство для работы с COM-портом */
		bool myport;

	private:

};
// -------------------------------------------------------------------------
#endif // ModbusRTUSlave_H_
// -------------------------------------------------------------------------
