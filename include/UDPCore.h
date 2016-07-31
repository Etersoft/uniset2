// -------------------------------------------------------------------------
#ifndef UDPReceiveU_H_
#define UDPReceiveU_H_
// -------------------------------------------------------------------------
#include <cc++/socket.h>
// -------------------------------------------------------------------------
// различные классы-обёртки, чтобы достучаться до "сырого сокета" и других функций
// необходимых при использовании с libev..
// -------------------------------------------------------------------------
class UDPSocketU:
	public ost::UDPSocket
{
	public:

		UDPSocketU( const ost::IPV4Address& bind, ost::tpport_t port):
			ost::UDPSocket(bind, port)
		{}

		virtual ~UDPSocketU() {}

		inline SOCKET getSocket() const
		{
			return ost::UDPSocket::so;
		}
};
// -------------------------------------------------------------------------
class UDPReceiveU:
	public ost::UDPReceive
{
	public:

		UDPReceiveU( const ost::IPV4Address& bind, ost::tpport_t port):
			ost::UDPReceive(bind, port)
		{}

		virtual ~UDPReceiveU() {}

		inline SOCKET getSocket()
		{
			return ost::UDPReceive::so;
		}
		inline void setCompletion( bool set )
		{
			ost::UDPReceive::setCompletion(set);
		}
};
// -------------------------------------------------------------------------
class UDPDuplexU:
	public ost::UDPDuplex
{
	public:

		UDPDuplexU(const ost::IPV4Address& bind, ost::tpport_t port):
			ost::UDPDuplex(bind, port)
		{}

		virtual ~UDPDuplexU() {}

		SOCKET getReceiveSocket()
		{
			return ost::UDPReceive::so;
		}
		void setReceiveCompletion( bool set )
		{
			ost::UDPReceive::setCompletion(set);
		}
};
// -------------------------------------------------------------------------
#endif // UDPReceiveU_H_
// -------------------------------------------------------------------------
