#ifndef ModbusTCPMaster_H_
#define ModbusTCPMaster_H_
// -------------------------------------------------------------------------
#include <memory>
#include <string>
#include <queue>
#include <cc++/socket.h>
#include "ModbusTypes.h"
#include "ModbusClient.h"
#include "UTCPStream.h"
// -------------------------------------------------------------------------
/*!    Modbus TCP master interface */
class ModbusTCPMaster:
	public ModbusClient
{
	public:

		ModbusTCPMaster();
		virtual ~ModbusTCPMaster();

		void connect( const std::string& addr, int port );
		void connect( ost::InetAddress addr, int port );
		void disconnect();
		void forceDisconnect();
		bool isConnection() const;

		static bool checkConnection( const std::string& ip, int port, int timeout_msec = 100 );

		inline void setForceDisconnect( bool s )
		{
			force_disconnect = s;
		}

		void reconnect();
		void cleanInputStream();

		virtual void cleanupChannel() override
		{
			cleanInputStream();
		}

		inline std::string getAddress() const
		{
			return iaddr;
		}
		inline int getPort() const
		{
			return port;
		}

	protected:

		virtual size_t getNextData(unsigned char* buf, size_t len ) override;
		virtual void setChannelTimeout( timeout_t msec ) override;
		virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, size_t len ) override;
		virtual ModbusRTU::mbErrCode query( ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusMessage& msg,
											ModbusRTU::ModbusMessage& reply, timeout_t timeout ) override;

	private:
		//ost::TCPStream* tcp;
		std::shared_ptr<UTCPStream> tcp;
		ModbusRTU::ModbusData nTransaction;
		std::queue<unsigned char> qrecv;
		PassiveTimer ptTimeout;
		std::string iaddr = { "" };
		int port = { 0 };
		bool force_disconnect = { false };
		int keepAliveTimeout = { 1000 };
};
// -------------------------------------------------------------------------
#endif // ModbusTCPMaster_H_
// -------------------------------------------------------------------------
