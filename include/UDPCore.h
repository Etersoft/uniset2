// -------------------------------------------------------------------------
#ifndef UDPReceiveU_H_
#define UDPReceiveU_H_
// -------------------------------------------------------------------------
#include <Poco/Net/DatagramSocket.h>
// -------------------------------------------------------------------------
// различные классы-обёртки, чтобы достучаться до "сырого сокета" и других функций
// необходимых при использовании с libev..
// -------------------------------------------------------------------------
class UDPSocketU:
	public Poco::Net::DatagramSocket
{
	public:

		UDPSocketU( const std::string& bind, int port ):
			Poco::Net::DatagramSocket(Poco::Net::SocketAddress(bind, port),true)
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

		UDPReceiveU( const std::string& bind, int port):
			Poco::Net::DatagramSocket(Poco::Net::SocketAddress(bind, port),true)
		{}

		virtual ~UDPReceiveU() {}

		inline int getSocket()
		{
			return Poco::Net::DatagramSocket::sockfd();
		}
		inline void setCompletion( bool set )
		{
			Poco::Net::DatagramSocket::setBlocking(set);
		}
};
// -------------------------------------------------------------------------
class UDPDuplexU:
	public Poco::Net::DatagramSocket
{
	public:

		UDPDuplexU(const std::string& bind, int port):
			Poco::Net::DatagramSocket(Poco::Net::SocketAddress(bind, port),true)
		{}

		virtual ~UDPDuplexU() {}

		int getReceiveSocket()
		{
			return Poco::Net::DatagramSocket::sockfd();;
		}
		void setReceiveCompletion( bool set )
		{
			Poco::Net::DatagramSocket::setBlocking(set);
		}
};
// -------------------------------------------------------------------------
#endif // UDPReceiveU_H_
// -------------------------------------------------------------------------
