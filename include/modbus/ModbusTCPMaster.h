/*! $Id: ModbusTCPMaster.h,v 1.1 2008/11/22 23:22:24 vpashka Exp $ */
// -------------------------------------------------------------------------
#ifndef ModbusTCPMaster_H_
#define ModbusTCPMaster_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <cc++/socket.h>
#include "ModbusTypes.h"
#include "ModbusClient.h"
// -------------------------------------------------------------------------
/*!	Modbus TCP master interface 
*/
class ModbusTCPMaster:
	public ModbusClient
{
	public:

		ModbusTCPMaster();
		virtual ~ModbusTCPMaster();

		void connect( ost::InetAddress addr, int port );
		void disconnect();
		bool isConnection();

	protected:

		virtual int getNextData( unsigned char* buf, int len );
		virtual void setChannelTimeout( int msec );
		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len );
		virtual ModbusRTU::mbErrCode query( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& msg, 
											ModbusRTU::ModbusMessage& reply, int timeout );

	private:
		ost::TCPStream* tcp;
		static int nTransaction;
		std::queue<unsigned char> qrecv;
};
// -------------------------------------------------------------------------
#endif // ModbusTCPMaster_H_
// -------------------------------------------------------------------------
