// -------------------------------------------------------------------------
#ifndef UDPReceiveU_H_
#define UDPReceiveU_H_
// -------------------------------------------------------------------------
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/MulticastSocket.h>
// -------------------------------------------------------------------------
// Классы-обёртки, чтобы достучаться до "сырого сокета" и других функций
// необходимых при использовании с libev..
// -------------------------------------------------------------------------
namespace uniset
{

    class UDPSocketU:
        public Poco::Net::DatagramSocket
    {
        public:

            UDPSocketU():
                Poco::Net::DatagramSocket(Poco::Net::IPAddress::IPv4)
            {}

            UDPSocketU( const std::string& bind, int port ):
                Poco::Net::DatagramSocket(Poco::Net::SocketAddress(bind, port), true)
            {}

            virtual ~UDPSocketU() {}

            inline int getSocket() const
            {
                return Poco::Net::DatagramSocket::sockfd();
            }
    };
    // -------------------------------------------------------------------------
    class UDPReceiveU:
        public Poco::Net::DatagramSocket
    {
        public:

            UDPReceiveU():
                Poco::Net::DatagramSocket(Poco::Net::IPAddress::IPv4)
            {}

            UDPReceiveU( const std::string& bind, int port):
                Poco::Net::DatagramSocket(Poco::Net::SocketAddress(bind, port), true)
            {}

            virtual ~UDPReceiveU() {}

            inline int getSocket()
            {
                return Poco::Net::DatagramSocket::sockfd();
            }
    };
    // -------------------------------------------------------------------------
    class MulticastSocketU:
            public Poco::Net::MulticastSocket
    {
    public:

        MulticastSocketU():
                Poco::Net::MulticastSocket(Poco::Net::IPAddress::IPv4)
        {}

        MulticastSocketU( int port ):
               Poco::Net::MulticastSocket(Poco::Net::SocketAddress(Poco::Net::IPAddress(), port), true)
        {}

        MulticastSocketU( const std::string& bind, int port ):
                Poco::Net::MulticastSocket(Poco::Net::SocketAddress(bind, port), true)
        {}

        virtual ~MulticastSocketU() {}

        inline int getSocket() const
        {
            return Poco::Net::MulticastSocket::sockfd();
        }
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UDPReceiveU_H_
// -------------------------------------------------------------------------
