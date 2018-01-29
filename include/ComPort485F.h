#ifndef DISABLE_COMPORT_485F
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
// --------------------------------------------------------------------------
#ifndef COMPORT_485F_H_
#define COMPORT_485F_H_
// --------------------------------------------------------------------------
#include <queue>
#include "ComPort.h"
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
namespace uniset
{
	/*!
	    Класс для обмена через 485 интерфейс СПЕЦИАЛЬНО
	    для контроллеров фирмы Fastwel.
	    Управляет приёмо/передатчиком. Удаляет "эхо"
	    посылок переданных в канал.

	    kernel 2.6.12:
	        module 8250_pnp
	        gpio_num=5 dev: /dev/ttyS2
	        gpio_num=6 dev: /dev/ttyS3
	*/
	class ComPort485F:
		public ComPort
	{
		public:

			ComPort485F( const std::string& comDevice, char gpio_num, bool tmit_ctrl = false );

			virtual void sendByte( unsigned char x ) override;
			virtual void setTimeout( timeout_t timeout ) override;
			virtual ssize_t sendBlock( unsigned char* msg, size_t len ) override;

			virtual void cleanupChannel() override;
			virtual void reopen() override;

		protected:

			virtual unsigned char m_receiveByte( bool wait ) override;
			void save2queue( unsigned char* msg, size_t len, size_t bnum );
			bool remove_echo( unsigned char tb[], ssize_t len );
			void m_read( timeout_t tmsec );

			/*! просто временный буфер для считывания данных */
			unsigned char tbuf[ComPort::BufSize];

			std::queue<unsigned char> wq; /*!< хранилище байтов записанных в канал */
			std::queue<unsigned char> rq; /*!< очередь для чтения */

			char gpio_num;
			bool tmit_ctrl_on;
			PassiveTimer ptRecv;
			timeout_t tout_msec = { 2000 };
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif // COMPORT_485F_H_
// --------------------------------------------------------------------------
#endif // #ifndef DISABLE_COMPORT_485F
