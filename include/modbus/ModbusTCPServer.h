// -------------------------------------------------------------------------
#ifndef ModbusTCPServer_H_
#define ModbusTCPServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <cc++/socket.h>
#include "Mutex.h"
#include "Debug.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "ModbusTypes.h"
#include "ModbusServer.h"

// -------------------------------------------------------------------------
/*!    ModbusTCP server */
class ModbusTCPServer:
    public ModbusServer,
    public ost::TCPSocket
{
    public:
        ModbusTCPServer( ost::InetAddress &ia, int port=502 );
        virtual ~ModbusTCPServer();

        virtual ModbusRTU::mbErrCode receive( ModbusRTU::ModbusAddr addr, timeout_t msecTimeout ) override;

        inline void setIgnoreAddrMode( bool st ){ ignoreAddr = st; }
        inline bool getIgnoreAddrMode(){ return ignoreAddr; }

        void cleanInputStream();
        virtual void cleanupChannel() override { cleanInputStream(); }

        virtual void terminate() override;

    protected:

        virtual ModbusRTU::mbErrCode pre_send_request( ModbusRTU::ModbusMessage& request );
//        virtual ModbusRTU::mbErrCode post_send_request( ModbusRTU::ModbusMessage& request );

        // realisation (see ModbusServer.h)
        virtual int getNextData( unsigned char* buf, int len ) override;
        virtual void setChannelTimeout( timeout_t msec ) override;
        virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) override;

        virtual ModbusRTU::mbErrCode tcp_processing( ost::TCPStream& tcp, ModbusTCP::MBAPHeader& mhead );

        ost::tpport_t port;
        ost::TCPStream tcp;
        ost::InetAddress iaddr;
        std::queue<unsigned char> qrecv;
        ModbusTCP::MBAPHeader curQueryHeader;

        bool ignoreAddr;

    private:

};
// -------------------------------------------------------------------------
#endif // ModbusTCPServer_H_
// -------------------------------------------------------------------------
