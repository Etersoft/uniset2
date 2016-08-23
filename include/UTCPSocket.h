// -------------------------------------------------------------------------
#ifndef UTCPSocket_H_
#define UTCPSocket_H_
// -------------------------------------------------------------------------
#include <string>
#include <Poco/Net/ServerSocket.h>
#include "PassiveTimer.h" // for timeout_t
// -------------------------------------------------------------------------
class UTCPSocket:
	public Poco::Net::ServerSocket
{
	public:

		UTCPSocket();

		// dup and accept...raw socket
		UTCPSocket( int sock );

		UTCPSocket( const std::string& host, int port );

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

	protected:
		void init( bool throwflag = false );

	private:

};
// -------------------------------------------------------------------------
#endif // UTCPSocket_H_
// -------------------------------------------------------------------------
