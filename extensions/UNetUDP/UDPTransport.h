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
#ifndef UDPTransport_H_
#define UDPTransport_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include "UNetTransport.h"
#include "UDPCore.h"
#include "UniXML.h"
// -------------------------------------------------------------------------
namespace uniset
{
    class UDPReceiveTransport:
        public UNetReceiveTransport
    {
        public:

            static std::unique_ptr<UDPReceiveTransport> createFromXml( UniXML::iterator it, const std::string& defaultIP, int numChan );

            UDPReceiveTransport( const std::string& bind, int port );
            virtual ~UDPReceiveTransport();

            virtual bool isConnected() const noexcept override;
            virtual std::string toString() const noexcept override;
            virtual std::string ID() const noexcept override;

            virtual bool createConnection( bool throwEx, timeout_t readTimeout, bool noblock ) override;
            virtual void disconnect() override;
            virtual int getSocket() const override;
            virtual ssize_t receive( void* r_buf, size_t sz ) override;
            virtual bool isReadyForReceive(timeout_t tout) noexcept override;
            virtual int available() override;

        protected:
            std::unique_ptr<UDPReceiveU> udp;
            const std::string host;
            const int port;
    };

    class UDPSendTransport:
        public UNetSendTransport
    {
        public:

            static std::unique_ptr<UDPSendTransport> createFromXml( UniXML::iterator it, const std::string& defaultIP, int numChan );

            UDPSendTransport( const std::string& host, int port );
            virtual ~UDPSendTransport();

            virtual bool isConnected() const noexcept override;
            virtual std::string toString() const noexcept override;

            virtual bool createConnection( bool throwEx, timeout_t sendTimeout ) override;
            virtual int getSocket() const override;

            // write
            virtual bool isReadyForSend( timeout_t tout ) noexcept override;
            virtual ssize_t send( const void* buf, size_t sz ) override;

        protected:
            std::unique_ptr<UDPSocketU> udp;
            Poco::Net::SocketAddress saddr;
    };

} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UDPTransport_H_
// -------------------------------------------------------------------------
