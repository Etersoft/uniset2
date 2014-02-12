#ifndef ModbusTCPMaster_H_
#define ModbusTCPMaster_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <cc++/socket.h>
#include "ModbusTypes.h"
#include "ModbusClient.h"
// -------------------------------------------------------------------------
/*!	Modbus TCP master interface */
class ModbusTCPMaster:
	public ModbusClient
{
	public:

		ModbusTCPMaster();
		virtual ~ModbusTCPMaster();

		void connect( const std::string addr, int port );
		void connect( ost::InetAddress addr, int port );
		void disconnect();
		bool isConnection();

		static bool checkConnection( const std::string& ip, int port, int timeout_msec=100 );

		inline void setForceDisconnect( bool s )
		{
			force_disconnect = s;
		}
		
		void reconnect();
		void cleanInputStream();
		
		virtual void cleanupChannel(){ cleanInputStream(); }

	protected:
		
		virtual int getNextData( unsigned char* buf, int len );
		virtual void setChannelTimeout( timeout_t msec );
		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len );
		virtual ModbusRTU::mbErrCode query( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& msg, 
											ModbusRTU::ModbusMessage& reply, timeout_t timeout );

	private:
		ost::TCPStream* tcp;
		ModbusRTU::ModbusData nTransaction;
		std::queue<unsigned char> qrecv;
		PassiveTimer ptTimeout;
		std::string iaddr;
		std::string addr;
		bool force_disconnect;
};
// -------------------------------------------------------------------------
#endif // ModbusTCPMaster_H_
// -------------------------------------------------------------------------
