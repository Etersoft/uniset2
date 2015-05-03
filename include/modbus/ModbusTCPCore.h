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
	int readNextData( ost::TCPStream* tcp, std::queue<unsigned char>& qrecv, int max = 100 );
	int getNextData( ost::TCPStream* tcp, std::queue<unsigned char>& qrecv, unsigned char* buf, int len );
	ModbusRTU::mbErrCode sendData( ost::TCPStream* tcp, unsigned char* buf, int len );
}
// -------------------------------------------------------------------------
#endif // ModbusTCPCore_H_
// -------------------------------------------------------------------------
