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
#include "UDPTransport.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
std::unique_ptr<UDPReceiveTransport> UDPReceiveTransport::createFromXml( UniXML::iterator it, const std::string& defaultIP, int numChan )
{
    ostringstream fieldIp;
    fieldIp << "unet_broadcast_ip";

    if( numChan > 0 )
        fieldIp << numChan;

    const string h = it.getProp2(fieldIp.str(), defaultIP);

    if( h.empty() )
    {
        ostringstream err;
        err << "(UDPReceiveTransport): Unknown broadcast IP for " << it.getProp("name");
        throw uniset::SystemError(err.str());
    }

    ostringstream fieldPort;
    fieldPort << "unet_port";

    if( numChan > 0 )
        fieldPort << numChan;

    int p = it.getPIntProp(fieldPort.str(), it.getIntProp("id"));
    return unisetstd::make_unique<UDPReceiveTransport>(h, p);
}
// -------------------------------------------------------------------------
UDPReceiveTransport::UDPReceiveTransport( const std::string& _bind, int _port ):
    host(_bind),
    port(_port)
{

}
// -------------------------------------------------------------------------
UDPReceiveTransport::~UDPReceiveTransport()
{
}
// -------------------------------------------------------------------------
bool UDPReceiveTransport::isConnected() const
{
    return udp != nullptr;
}
// -------------------------------------------------------------------------
std::string UDPReceiveTransport::ID() const noexcept
{
    return toString();
}
// -------------------------------------------------------------------------
std::string UDPReceiveTransport::toString() const
{
    ostringstream s;
    s << host << ":" << port;
    return s.str();
}
// -------------------------------------------------------------------------
void UDPReceiveTransport::disconnect()
{
    udp = nullptr;
}
// -------------------------------------------------------------------------
bool UDPReceiveTransport::createConnection( bool throwEx, timeout_t readTimeout, bool noblock )
{
    try
    {
        udp = unisetstd::make_unique<UDPReceiveU>(host, port);
        udp->setBlocking(!noblock);
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
int UDPReceiveTransport::getSocket() const
{
    return udp->getSocket();
}
// -------------------------------------------------------------------------
ssize_t UDPReceiveTransport::receive( void* r_buf, size_t sz )
{
    return udp->receiveBytes(r_buf, sz);
}
// -------------------------------------------------------------------------
bool UDPReceiveTransport::isReadyForReceive( timeout_t tout )
{
    return udp->poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_READ);
}
// -------------------------------------------------------------------------
int UDPReceiveTransport::available()
{
    return udp->available();
}
// -------------------------------------------------------------------------
std::unique_ptr<UDPSendTransport> UDPSendTransport::createFromXml( UniXML::iterator it, const std::string& defaultIP, int numChan )
{
    ostringstream fieldIp;
    fieldIp << "unet_broadcast_ip";

    if( numChan > 0 )
        fieldIp << numChan;

    const string h = it.getProp2(fieldIp.str(), defaultIP);

    if( h.empty() )
    {
        ostringstream err;
        err << "(UDPReceiveTransport): Unknown broadcast IP for " << it.getProp("name");
        throw uniset::SystemError(err.str());
    }

    ostringstream fieldPort;
    fieldPort << "unet_port";

    if( numChan > 0 )
        fieldPort << numChan;

    int p = it.getPIntProp(fieldPort.str(), it.getIntProp("id"));

    return unisetstd::make_unique<UDPSendTransport>(h, p);
}
// -------------------------------------------------------------------------
UDPSendTransport::UDPSendTransport( const std::string& _host, int _port ):
    saddr(_host, _port)
{

}
// -------------------------------------------------------------------------
UDPSendTransport::~UDPSendTransport()
{
}
// -------------------------------------------------------------------------
std::string UDPSendTransport::toString() const
{
    return saddr.toString();
}
// -------------------------------------------------------------------------
bool UDPSendTransport::isConnected() const
{
    return udp != nullptr;
}
// -------------------------------------------------------------------------
bool UDPSendTransport::createConnection( bool throwEx, timeout_t sendTimeout )
{
    try
    {
        udp = unisetstd::make_unique<UDPSocketU>();
        udp->setBroadcast(true);
        udp->setSendTimeout( UniSetTimer::millisecToPoco(sendTimeout) );
        //      udp->setNoDelay(true);
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
int UDPSendTransport::getSocket() const
{
    return udp->getSocket();
}
// -------------------------------------------------------------------------
bool UDPSendTransport::isReadyForSend( timeout_t tout )
{
    return udp && udp->poll( UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_WRITE );
}
// -------------------------------------------------------------------------
ssize_t UDPSendTransport::send( const void* buf, size_t sz )
{
    return udp->sendTo(buf, sz, saddr);
}
// -------------------------------------------------------------------------
