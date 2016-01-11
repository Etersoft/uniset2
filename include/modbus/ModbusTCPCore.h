// -------------------------------------------------------------------------
#ifndef ModbusTCPCore_H_
#define ModbusTCPCore_H_
// -------------------------------------------------------------------------
#include <queue>
#include <cc++/socket.h>
#include "ModbusRTUErrors.h"
#include "UTCPStream.h"
// -------------------------------------------------------------------------
/*!    ModbusTCP core functions */
namespace ModbusTCPCore
{
	// t - msec (сколько ждать)

	size_t readNextData( UTCPStream* tcp, std::queue<unsigned char>& qrecv, size_t max = 100, timeout_t t = 10 );
	size_t getNextData( UTCPStream* tcp, std::queue<unsigned char>& qrecv, unsigned char* buf, size_t len, timeout_t t = 10 );
	ModbusRTU::mbErrCode sendData(UTCPStream* tcp, unsigned char* buf, size_t len, timeout_t t = 10 );

	// работа напрямую с сокетом
	size_t readDataFD(int fd, std::queue<unsigned char>& qrecv, size_t max = 100, size_t attempts = 1 );
	size_t getDataFD( int fd, std::queue<unsigned char>& qrecv, unsigned char* buf, size_t len, size_t attempts = 1 );
	ModbusRTU::mbErrCode sendDataFD( int fd, unsigned char* buf, size_t len );
}
// -------------------------------------------------------------------------
#endif // ModbusTCPCore_H_
// -------------------------------------------------------------------------
