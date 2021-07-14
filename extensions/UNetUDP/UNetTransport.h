/*
 * Copyright (c) 2021 Pavel Vainerman.
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
#ifndef UNetTransport_H_
#define UNetTransport_H_
// -------------------------------------------------------------------------
#include <string>
#include "PassiveTimer.h" // for typedef timeout_t
// -------------------------------------------------------------------------
namespace uniset
{
    // Интерфейс для получения данных по сети
    class UNetReceiveTransport
    {
        public:

            virtual ~UNetReceiveTransport() {}

            virtual bool isConnected() const noexcept = 0;
            virtual std::string toString() const noexcept = 0;
            virtual std::string ID() const noexcept = 0;

            virtual bool createConnection( bool throwEx, timeout_t recvTimeout, bool noblock ) = 0;
            virtual int getSocket() const = 0;

            virtual bool isReadyForReceive(timeout_t tout) noexcept = 0;
            virtual ssize_t receive( void* r_buf, size_t sz ) = 0;
            virtual void disconnect() = 0;
            virtual int available() = 0;
    };

    // Интерфейс для посылки данных в сеть
    class UNetSendTransport
    {
        public:

            virtual ~UNetSendTransport() {}

            virtual bool isConnected() const = 0;
            virtual std::string toString() const = 0;

            virtual bool createConnection( bool throwEx, timeout_t sendTimeout ) = 0;
            virtual int getSocket() const = 0;

            // write
            virtual bool isReadyForSend( timeout_t tout ) = 0;
            virtual ssize_t send( const void* r_buf, size_t sz ) = 0;
    };
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UNetTransport_H_
// -------------------------------------------------------------------------
