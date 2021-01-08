// -------------------------------------------------------------------------
#ifndef UTCPSocket_H_
#define UTCPSocket_H_
// -------------------------------------------------------------------------
#include <string>
#include <Poco/Net/ServerSocket.h>
#include "PassiveTimer.h" // for timeout_t
// -------------------------------------------------------------------------
namespace uniset
{

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

            int getSocket() const noexcept;

        protected:
            void init();

        private:

    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UTCPSocket_H_
// -------------------------------------------------------------------------
