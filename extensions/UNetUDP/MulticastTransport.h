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
#include "UniXML.h"
// -------------------------------------------------------------------------
namespace uniset
{
    class MulticastReceiveTransport:
        public UNetReceiveTransport
    {
        public:

            static std::unique_ptr<MulticastReceiveTransport> createFromXml( UniXML::iterator root, UniXML::iterator it, int numChan);
            static xmlNode* getReceiveListNode( UniXML::iterator root );

            MulticastReceiveTransport( const std::string& bind, int port, const std::vector<Poco::Net::IPAddress>& joinGroups, const std::string& iface = "" );
            virtual ~MulticastReceiveTransport();

            virtual bool isConnected() const noexcept override;
            virtual std::string toString() const noexcept override;
            virtual std::string ID() const noexcept override;

            virtual bool createConnection(bool throwEx, timeout_t readTimeout, bool noblock) override;
            virtual void disconnect() override;
            virtual int getSocket() const override;
            std::vector<Poco::Net::IPAddress> getGroups();
            void setLoopBack( bool state );

            bool isReadyForReceive( timeout_t tout ) noexcept override;
            virtual ssize_t receive(void* r_buf, size_t sz) override;
            virtual int available() override;
            std::string iface() const;

        protected:
            std::unique_ptr <MulticastSocketU> udp;
            const std::string host;
            const int port;
            const std::vector<Poco::Net::IPAddress> groups;
            const std::string ifaceaddr;
    };

    class MulticastSendTransport:
        public UNetSendTransport
    {
        public:

            static std::unique_ptr<MulticastSendTransport> createFromXml( UniXML::iterator root, UniXML::iterator it, int numChan );

            MulticastSendTransport(const std::string& sockHost, int sockPort, const std::string& groupHost, int groupPort, int ttl = 1 );
            virtual ~MulticastSendTransport();

            virtual bool isConnected() const noexcept override;
            virtual std::string toString() const noexcept override;

            virtual bool createConnection(bool throwEx, timeout_t sendTimeout) override;
            virtual int getSocket() const override;
            Poco::Net::SocketAddress getGroupAddress();

            // write
            virtual bool isReadyForSend(timeout_t tout) noexcept override;
            virtual ssize_t send(const void* buf, size_t sz) override;

            void setTimeToLive( int ttl );
            void setLoopBack( bool state );

        protected:
            std::unique_ptr <MulticastSocketU> udp;
            const Poco::Net::SocketAddress sockAddr;
            const Poco::Net::SocketAddress toAddr;
            int ttl; // ttl for packets
    };

} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // MulticastTransport_H_
// -------------------------------------------------------------------------
