// -------------------------------------------------------------------------
#ifndef UTCPCore_H_
#define UTCPCore_H_
// -------------------------------------------------------------------------
#include <string>
#include <cstring> // for std::memcpy
#include "PassiveTimer.h" // ..for timeout_t
// -------------------------------------------------------------------------
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
		Buffer( const unsigned char* bytes, ssize_t nbytes );
		Buffer( const std::string& s );
		virtual ~Buffer();

		unsigned char* dpos() noexcept;

		ssize_t nbytes() noexcept;

		unsigned char* data = { 0 };
		ssize_t len;
		ssize_t pos;
	};
}
// -------------------------------------------------------------------------
#endif // UTCPCore_H_
// -------------------------------------------------------------------------
