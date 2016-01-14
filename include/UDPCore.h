// -------------------------------------------------------------------------
#ifndef UDPReceiveU_H_
#define UDPReceiveU_H_
// -------------------------------------------------------------------------
#include <cc++/socket.h>
// -------------------------------------------------------------------------
// обёртка над ost::UDPReceive, чтобы достучаться до "сырого сокета"
// для дальнейшего использования с libev..
class UDPReceiveU:
	public ost::UDPReceive
{
	public:

		SOCKET getSocket(){ return ost::UDPReceive::so; }
};
// -------------------------------------------------------------------------
// обёртка над ost::UDPReceive, чтобы достучаться до "сырого сокета"
// для дальнейшего использования с libev..
class UDPDuplexU:
	public ost::UDPDuplex
{
	public:

		UDPDuplexU(const ost::IPV4Address &bind, ost::tpport_t port):
			ost::UDPDuplex(bind,port)
		{}

		SOCKET getReceiveSocket(){ return ost::UDPReceive::so; }
		void setReceiveCompletion( bool set ){ ost::UDPReceive::setCompletion(set); }
};
// -------------------------------------------------------------------------
#endif // UDPReceiveU_H_
// -------------------------------------------------------------------------
