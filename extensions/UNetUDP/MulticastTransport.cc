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
#include <sstream>
#include <iomanip>
#include <Poco/Net/NetException.h>
#include "Exceptions.h"
#include "PassiveTimer.h"
#include "unisetstd.h"
#include "MulticastTransport.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
MulticastReceiveTransport::MulticastReceiveTransport( const std::string& _bind, int _port, const std::vector<Poco::Net::IPAddress> _joinGroups ):
    host(_bind),
    port(_port),
    groups(_joinGroups)
{

}
// -------------------------------------------------------------------------
MulticastReceiveTransport::~MulticastReceiveTransport()
{
    if( udp )
    {
        for (const auto& s : groups)
        {
            try
            {
                udp->leaveGroup(s);
            }
            catch (...) {}
        }
    }
}
// -------------------------------------------------------------------------
bool MulticastReceiveTransport::isConnected() const
{
    return udp != nullptr;
}
// -------------------------------------------------------------------------
std::string MulticastReceiveTransport::ID() const noexcept
{
    return toString();
}
// -------------------------------------------------------------------------
std::string MulticastReceiveTransport::toString() const
{
    ostringstream s;
    s << host << ":" << port;
    return s.str();
}
// -------------------------------------------------------------------------
void MulticastReceiveTransport::disconnect()
{
    if (udp)
    {
        for (const auto& s : groups)
        {
            try
            {
                udp->leaveGroup(s);
            }
            catch (...) {}
        }

        udp = nullptr;
    }
}

// -------------------------------------------------------------------------
bool MulticastReceiveTransport::createConnection( bool throwEx, timeout_t readTimeout, bool noblock )
{
    try
    {
        udp = unisetstd::make_unique<MulticastSocketU>(host, port);
        udp->setBlocking(!noblock);

        for( const auto& s : groups )
            udp->joinGroup(s);
    }
    catch( const std::exception& e )
    {
        udp = nullptr;
        ostringstream s;
        s << host << ":" << port << "(createConnection): " << e.what();

        if( throwEx )
            throw uniset::SystemError(s.str());
    }
    catch( ... )
    {
        udp = nullptr;
        ostringstream s;
        s << host << ":" << port << "(createConnection): catch...";

        if( throwEx )
            throw uniset::SystemError(s.str());
    }

    return ( udp != nullptr );
}
// -------------------------------------------------------------------------
int MulticastReceiveTransport::getSocket() const
{
    return udp->getSocket();
}
// -------------------------------------------------------------------------
ssize_t MulticastReceiveTransport::receive( void* r_buf, size_t sz )
{
    return udp->receiveBytes(r_buf, sz);
}
// -------------------------------------------------------------------------
MulticastSendTransport::MulticastSendTransport( const std::string& _host, int _port, const std::vector<Poco::Net::IPAddress> _sendGroups ):
    saddr(_host, _port),
    groups(_sendGroups)
{

}
// -------------------------------------------------------------------------
MulticastSendTransport::~MulticastSendTransport()
{
    if( udp )
    {
        for (const auto& s : groups)
        {
            try
            {
                udp->leaveGroup(s);
            }
            catch (...) {}
        }
    }
}
// -------------------------------------------------------------------------
std::string MulticastSendTransport::toString() const
{
    return saddr.toString();
}
// -------------------------------------------------------------------------
bool MulticastSendTransport::isConnected() const
{
    return udp != nullptr;
}
// -------------------------------------------------------------------------
bool MulticastSendTransport::createConnection( bool throwEx, timeout_t sendTimeout )
{
    try
    {
        udp = unisetstd::make_unique<MulticastSocketU>();

        for( const auto& s : groups )
            udp->joinGroup(s);

        udp->setSendTimeout( UniSetTimer::millisecToPoco(sendTimeout) );
    }
    catch( const std::exception& e )
    {
        udp = nullptr;
        ostringstream s;
        s << saddr.toString() << "(createConnection): " << e.what();

        if( throwEx )
            throw uniset::SystemError(s.str());
    }
    catch( ... )
    {
        udp = nullptr;
        ostringstream s;
        s << saddr.toString() << "(createConnection): catch...";

        if( throwEx )
            throw uniset::SystemError(s.str());
    }

    return (udp != nullptr);
}
// -------------------------------------------------------------------------
int MulticastSendTransport::getSocket() const
{
    return udp->getSocket();
}
// -------------------------------------------------------------------------
bool MulticastSendTransport::isReadyForSend( timeout_t tout )
{
    return udp && udp->poll( UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_WRITE );
}
// -------------------------------------------------------------------------
ssize_t MulticastSendTransport::send( void* buf, size_t sz )
{
    return udp->sendTo(buf, sz, saddr);
}
// -------------------------------------------------------------------------
