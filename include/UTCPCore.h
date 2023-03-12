// -------------------------------------------------------------------------
#ifndef UTCPCore_H_
#define UTCPCore_H_
// -------------------------------------------------------------------------
#include <string>
#include <cstring> // for std::memcpy
#include "PassiveTimer.h" // ..for timeout_t
// -------------------------------------------------------------------------
namespace uniset
{

    namespace UTCPCore
    {
        bool setKeepAliveParams( int sock, timeout_t timeout_sec = 5, int conn_keepcnt = 1, int keepintvl = 2 ) noexcept;

        // -------------------------------------------
        // author: https://gist.github.com/koblas/3364414
        // ----------------------
        // for use with ev::io..
        // Buffer class - allow for output buffering such that it can be written out into async pieces
        struct Buffer
        {
            Buffer(){}
            Buffer( const unsigned char* bytes, size_t nbytes );
            Buffer( const std::string& s );
            virtual ~Buffer();

            void reset( const std::string& s );
            void reset( const unsigned char* bytes, size_t nbytes );

            unsigned char* dpos() const noexcept;

            size_t nbytes() const noexcept;

            unsigned char* data = { nullptr };
            size_t len = { 0 };
            size_t pos = { 0 };
        };
    }
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UTCPCore_H_
// -------------------------------------------------------------------------
