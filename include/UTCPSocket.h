// -------------------------------------------------------------------------
#ifndef UTCPSocket_H_
#define UTCPSocket_H_
// -------------------------------------------------------------------------
#include <string>
#include <Poco/Net/RawSocket.h>
#include "PassiveTimer.h" // for timeout_t
// -------------------------------------------------------------------------
class UTCPSocket:
	public Poco::Net::RawSocket
{
	public:

		// dup and accept...raw socket
		UTCPSocket( int sock );

		UTCPSocket(const std::string& host, int port);

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
