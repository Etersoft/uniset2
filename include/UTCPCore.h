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
			Buffer( const unsigned char* bytes, size_t nbytes );
			Buffer( const std::string& s );
			virtual ~Buffer();

			unsigned char* dpos() noexcept;

			size_t nbytes() noexcept;

			unsigned char* data = { 0 };
			size_t len;
			size_t pos;
		};
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UTCPCore_H_
// -------------------------------------------------------------------------
