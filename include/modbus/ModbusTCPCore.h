// -------------------------------------------------------------------------
#ifndef ModbusTCPCore_H_
#define ModbusTCPCore_H_
// -------------------------------------------------------------------------
#include <queue>
#include "ModbusRTUErrors.h"
#include "UTCPStream.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	/*!    ModbusTCP core functions */
	namespace ModbusTCPCore
	{
		// Если соединение закрыто (другой стороной), функции выкидывают исключение uniset::CommFailed

		// t - msec (сколько ждать)
		size_t readNextData(UTCPStream* tcp, std::queue<unsigned char>& qrecv, size_t max = 100);
		size_t getNextData( UTCPStream* tcp, std::queue<unsigned char>& qrecv, unsigned char* buf, size_t len );
		ModbusRTU::mbErrCode sendData(UTCPStream* tcp, unsigned char* buf, size_t len );

		// работа напрямую с сокетом
		size_t readDataFD(int fd, std::queue<unsigned char>& qrecv, size_t max = 100, size_t attempts = 1 );
		size_t getDataFD( int fd, std::queue<unsigned char>& qrecv, unsigned char* buf, size_t len, size_t attempts = 1 );
		ModbusRTU::mbErrCode sendDataFD( int fd, unsigned char* buf, size_t len );
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusTCPCore_H_
// -------------------------------------------------------------------------
