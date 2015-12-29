/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#ifndef UTCPStream_H_
#define UTCPStream_H_
// -------------------------------------------------------------------------
#include <string>
#include <cc++/socket.h>
// -------------------------------------------------------------------------
/*! Специальная "обёртка" над ost::TCPStream, устанавливающая ещё и параметры KeepAlive,
 * для открытого сокета.
 * \note Правда это linux-only
*/
class UTCPStream:
	public ost::TCPStream
{
	public:

		UTCPStream();
		virtual ~UTCPStream();

		void create( const std::string& hname, int port, bool throwflag = false, timeout_t timer = 0 );

		// set keepalive params
		// return true if OK
		bool setKeepAliveParams( timeout_t timeout_sec = 5, int conn_keepcnt = 1, int keepintvl = 2 );

		bool isSetLinger();
		void forceDisconnect(); // disconnect() без ожидания (с отключением SO_LINGER)

		/*!
		 * Enable/disable delaying packets (Nagle algorithm)
		 *
		 * @return true on success.
		 * @param enable disable Nagle algorithm when set to true.
		 */
		bool setNoDelay(bool enable);

		// --------------------------------------------------------------------
		// Пришлось вынести эти функции read/write[Data] в public
		// т.к. они сразу "посылают" данные в канал, в отличие от operator<<
		// который у TCPStream (или std::iostream?) буферизует их и из-за этого
		// не позволяет работать с отправкой коротких сообщений
		// --------------------------------------------------------------------
		ssize_t writeData( const void* buf, size_t len, timeout_t t=0 );
		ssize_t readData( void * buf,size_t len,char separator=0,timeout_t t=0 );

		int getSocket();

	protected:

	private:

};
// -------------------------------------------------------------------------
#endif // UTCPStream_H_
// -------------------------------------------------------------------------
