// -------------------------------------------------------------------------
#ifndef UTCPCore_H_
#define UTCPCore_H_
// -------------------------------------------------------------------------
#include <cc++/thread.h> // ..for timeout_t
#include <string>
// -------------------------------------------------------------------------
namespace UTCPCore
{
	bool setKeepAliveParams( int sock, timeout_t timeout_sec = 5, int conn_keepcnt = 1, int keepintvl = 2 );

	// -------------------------------------------
	// author: https://gist.github.com/koblas/3364414
	// ----------------------
	// for use with ev::io..
	// Buffer class - allow for output buffering such that it can be written out into async pieces
	struct Buffer
	{
		unsigned char* data;
		ssize_t len;
		ssize_t pos;

		Buffer( const unsigned char* bytes, ssize_t nbytes )
		{
			pos = 0;
			len = nbytes;

			if( len <= 0 ) // ??!!
				return;

			data = new unsigned char[nbytes];
			memcpy(data, bytes, nbytes);
		}

		Buffer( const std::string& s )
		{
			pos = 0;
			len = s.length();

			if( len <= 0 ) // ??!!
				return;

			data = new unsigned char[len];
			memcpy(data, s.data(), len);
		}

		virtual ~Buffer()
		{
			delete [] data;
		}

		unsigned char* dpos()
		{
			return data + pos;
		}

		ssize_t nbytes()
		{
			return len - pos;
		}
	};
}
// -------------------------------------------------------------------------
#endif // UTCPCore_H_
// -------------------------------------------------------------------------
