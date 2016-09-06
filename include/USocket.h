// -------------------------------------------------------------------------
#ifndef USocket_H_
#define USocket_H_
// -------------------------------------------------------------------------
#include <Poco/Net/Socket.h>
#include "PassiveTimer.h" // fot timeout_t
// -------------------------------------------------------------------------
class USocket:
	public Poco::Net::Socket
{
	public:

		// dup and accept...raw socket
		USocket( int sock );
		virtual ~USocket();

		// set keepalive params
		// return true if OK
		bool setKeepAliveParams( timeout_t timeout_sec = 5, int conn_keepcnt = 1, int keepintvl = 2 );

		int getSocket();

	protected:
		void init( bool throwflag = false );

	private:

};
// -------------------------------------------------------------------------
#endif // USocket_H_
// -------------------------------------------------------------------------
