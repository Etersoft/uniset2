#include "UTCPCore.h"
// glibc..
#include <netinet/tcp.h>
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    bool UTCPCore::setKeepAliveParams( int fd, timeout_t timeout_sec, int keepcnt, int keepintvl ) noexcept
    {
        int enable = 1;
        bool ok = true;

        if( setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&enable, sizeof(enable)) == -1 )
            ok = false;

        if( setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*) &keepcnt, sizeof(keepcnt)) == -1 )
            ok = false;

        if( setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*) &keepintvl, sizeof (keepintvl)) == -1 )
            ok = false;

        if( setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*) &timeout_sec, sizeof (timeout_sec)) == -1 )
            ok = false;

        return ok;
    }
    // -------------------------------------------------------------------------
    UTCPCore::Buffer::Buffer( const unsigned char* bytes, size_t nbytes )
    {
        pos = 0;
        len = nbytes;

        if( len == 0 ) // ??!!
            return;

        data = new unsigned char[nbytes];
        std::memcpy(data, bytes, nbytes);
    }
    // -------------------------------------------------------------------------
    UTCPCore::Buffer::Buffer( const string& s )
    {
        pos = 0;
        len = s.length();

        if( len == 0 ) // ??!!
            return;

        data = new unsigned char[len];
        std::memcpy(data, s.data(), len);
    }
    // -------------------------------------------------------------------------
    void UTCPCore::Buffer::reset( const std::string& s )
    {
        pos = 0;
        if( s.length() == 0 ) // ??!!
        {
            len = 0;
            return;
        }

        if( s.length() > len )
        {
            delete [] data;
            data = new unsigned char[s.length()];
        }

        len = s.length();
        std::memcpy(data, s.data(), s.length());
    }
    // -------------------------------------------------------------------------
    void UTCPCore::Buffer::reset( const unsigned char* bytes, size_t nbytes )
    {
        pos = 0;
        if( nbytes == 0 ) // ??!!
        {
            len = 0;
            return;
        }

        if( nbytes > len )
        {
            delete [] data;
            data = new unsigned char[nbytes];
        }
        len = nbytes;
        std::memcpy(data, bytes, nbytes);
    }
    // -------------------------------------------------------------------------
    UTCPCore::Buffer::~Buffer()
    {
        delete [] data;
    }
    // -------------------------------------------------------------------------
    unsigned char* UTCPCore::Buffer::dpos() const noexcept
    {
        return data + pos;
    }
    // -------------------------------------------------------------------------
    size_t UTCPCore::Buffer::nbytes() const noexcept
    {
        return len - pos;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
