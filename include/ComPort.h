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
#ifndef COMPORT_H_
#define COMPORT_H_
// --------------------------------------------------------------------------
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string>
#include "PassiveTimer.h" // for use timeout_t
// --------------------------------------------------------------------------
namespace uniset
{

	class ComPort
	{
		public:
			enum Speed
			{
				ComSpeed0 = B0,
				ComSpeed50 = B50,
				ComSpeed75 = B75,
				ComSpeed110 = B110,
				ComSpeed134 = B134,
				ComSpeed150 = B150,
				ComSpeed200 = B200,
				ComSpeed300 = B300,
				ComSpeed600 = B600,
				ComSpeed1200 = B1200,
				ComSpeed1800 = B1800,
				ComSpeed2400 = B2400,
				ComSpeed4800 = B4800,
				ComSpeed9600 = B9600,
				ComSpeed19200 = B19200,
				ComSpeed38400 = B38400,
				ComSpeed57600 = B57600,
				ComSpeed115200 = B115200,
				ComSpeed230400 = B230400,
				ComSpeed460800 = B460800,
				ComSpeed500000 = B500000,
				ComSpeed576000 = B576000,
				ComSpeed921600 = B921600,
				ComSpeed1000000 = B1000000,
				ComSpeed1152000 = B1152000,
				ComSpeed1500000 = B1500000,
				ComSpeed2000000 = B2000000,
				ComSpeed2500000 = B2500000,
				ComSpeed3000000 = B3000000,
				ComSpeed3500000 = B3500000,
				ComSpeed4000000 = B4000000
			};
			enum Parity
			{
				Odd,
				Even,
				Space,
				Mark,
				NoParity
			};
			enum CharacterSize
			{
				CSize5 = CS5,
				CSize6 = CS6,
				CSize7 = CS7,
				CSize8 = CS8
			};
			enum StopBits
			{
				OneBit = 1,
				OneAndHalfBits = 2,
				TwoBits = 3
			};

			ComPort( const std::string& comDevice, bool nocreate = false );
			virtual ~ComPort();

			inline std::string getDevice()
			{
				return dev;
			}

			void setSpeed( Speed s );
			void setSpeed( const std::string& speed );
			Speed getSpeed() const;

			static Speed getSpeed( const std::string& s );
			static std::string getSpeed( Speed s );

			void setParity(Parity);
			void setCharacterSize(CharacterSize);
			void setStopBits(StopBits sBit);

			virtual void setTimeout( timeout_t msec );
			timeout_t getTimeout() const;

			void setWaiting(bool waiting);

			virtual unsigned char receiveByte();
			virtual void sendByte(unsigned char x);

			virtual size_t receiveBlock( unsigned char* msg, size_t len );
			virtual ssize_t sendBlock( unsigned char* msg, size_t len );

			void setBlocking(bool blocking);

			virtual void cleanupChannel();
			virtual void reopen();

		protected:
			void openPort();

			static const size_t BufSize = 8192;
			unsigned char buf[BufSize];
			ssize_t curSym = { 0 };
			ssize_t bufLength = { 0 };
			int fd = { -1 };
			timeout_t uTimeout = { 0 };
			bool waiting = { false };
			Speed speed = ComSpeed38400;
			std::string dev = { "" };

			virtual unsigned char m_receiveByte( bool wait );

		private:
			struct termios oldTermios;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif // COMPORT_H_
// --------------------------------------------------------------------------
