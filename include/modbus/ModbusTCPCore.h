// -------------------------------------------------------------------------
#ifndef ModbusTCPCore_H_
#define ModbusTCPCore_H_
// -------------------------------------------------------------------------
#include <queue>
#include <cc++/socket.h>
#include "ModbusRTUErrors.h"
// -------------------------------------------------------------------------
/*!    ModbusTCP core functions */
namespace ModbusTCPCore
{
	size_t readNextData( ost::TCPStream* tcp, std::queue<unsigned char>& qrecv, int max = 100 );
	size_t getNextData( ost::TCPStream* tcp, std::queue<unsigned char>& qrecv, unsigned char* buf, size_t len );
	ModbusRTU::mbErrCode sendData( ost::TCPStream* tcp, unsigned char* buf, size_t len );
}
// -------------------------------------------------------------------------
#endif // ModbusTCPCore_H_
// -------------------------------------------------------------------------
