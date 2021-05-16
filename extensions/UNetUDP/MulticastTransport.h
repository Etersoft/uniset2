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
#ifndef MulticastTransport_H_
#define MulticastTransport_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <vector>
#include "UNetTransport.h"
#include "UDPCore.h"
// -------------------------------------------------------------------------
namespace uniset
{
    class MulticastReceiveTransport :
        public UNetReceiveTransport
    {
        public:

            MulticastReceiveTransport( const std::string& bind, int port, const std::vector<Poco::Net::IPAddress> joinGroups );

            virtual ~MulticastReceiveTransport();

            virtual bool isConnected() const override;

            virtual std::string toString() const override;

            virtual std::string ID() const noexcept override;

            virtual bool createConnection(bool throwEx, timeout_t readTimeout, bool noblock) override;

            virtual void disconnect() override;

            virtual int getSocket() const override;

            virtual ssize_t receive(void* r_buf, size_t sz) override;

        protected:
            std::unique_ptr <MulticastSocketU> udp;
            const std::string host;
            const int port;
            const std::vector<Poco::Net::IPAddress> groups;
    };

    class MulticastSendTransport :
        public UNetSendTransport
    {
        public:

            MulticastSendTransport(const std::string& host, int port, const std::vector<Poco::Net::IPAddress> sendGroups );

            virtual ~MulticastSendTransport();

            virtual bool isConnected() const override;

            virtual std::string toString() const override;

            virtual bool createConnection(bool throwEx, timeout_t sendTimeout) override;

            virtual int getSocket() const override;

            // write
            virtual bool isReadyForSend(timeout_t tout) override;

            virtual ssize_t send(void* buf, size_t sz) override;

        protected:
            std::unique_ptr <MulticastSocketU> udp;
            const Poco::Net::SocketAddress saddr;
            const std::vector<Poco::Net::IPAddress> groups;
    };

} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // MulticastTransport_H_
// -------------------------------------------------------------------------
