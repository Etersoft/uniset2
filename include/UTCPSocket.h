// -------------------------------------------------------------------------
#ifndef UTCPSocket_H_
#define UTCPSocket_H_
// -------------------------------------------------------------------------
#include <string>
#include <cc++/socket.h>
// -------------------------------------------------------------------------
class UTCPSocket:
	public ost::TCPSocket
{
	public:

		// dup and accept...raw socket
		UTCPSocket( int sock );

		// hname = "host:port"
		UTCPSocket(const std::string& hname, unsigned backlog = 5, unsigned mss = 536 );
		UTCPSocket(const ost::IPV4Address& bind, ost::tpport_t port, unsigned backlog = 5, unsigned mss = 536 );

		virtual ~UTCPSocket();

		// set keepalive params
		// return true if OK
		bool setKeepAliveParams( timeout_t timeout_sec = 5, int conn_keepcnt = 1, int keepintvl = 2 );

		/*!
		 * Enable/disable delaying packets (Nagle algorithm)
		 *
		 * @return true on success.
		 * @param enable disable Nagle algorithm when set to true.
		 */
		bool setNoDelay( bool enable );

		void setCompletion( bool set )
		{
			ost::TCPSocket::setCompletion(set);
		}

		int getSocket();

		// --------------------------------------------------------------------
		// Пришлось вынести эти функции read/write[Data] в public
		// т.к. они сразу "посылают" данные в канал, в отличие от operator<<
		// который у TCPStream (или std::iostream?) буферизует их и из-за этого
		// не позволяет работать с отправкой коротких сообщений
		// --------------------------------------------------------------------
		ssize_t writeData( const void* buf, size_t len, timeout_t t = 0 );
		ssize_t readData( void* buf, size_t len, char separator = 0, timeout_t t = 0 );

	protected:
		void init( bool throwflag = false );

	private:

};
// -------------------------------------------------------------------------
#endif // UTCPSocket_H_
// -------------------------------------------------------------------------
