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
    int readNextData( ost::TCPStream* tcp, std::queue<unsigned char>& qrecv, int max=100 );
    int getNextData( unsigned char* buf, int len, std::queue<unsigned char>& qrecv, ost::TCPStream* tcp );
    ModbusRTU::mbErrCode sendData( unsigned char* buf, int len, ost::TCPStream* tcp );
}
// -------------------------------------------------------------------------
#endif // ModbusTCPCore_H_
// -------------------------------------------------------------------------
