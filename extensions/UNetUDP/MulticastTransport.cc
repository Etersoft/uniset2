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
xmlNode* MulticastReceiveTransport::getReceiveListNode( UniXML::iterator root )
{
    UniXML::iterator it = root;

    if( !it.find("multicast") )
        return nullptr;

    if( !it.goChildren() )
        return nullptr;

    if( !it.find("receive") )
        return nullptr;

    if( !it.goChildren() )
        return nullptr;

    return it.getCurrent();
}
// -------------------------------------------------------------------------
/*
 * <nodes unet_multicast_ip=".." unet_multicast_iface=".." >
 * <item id="3000"
 *    unet_multicast_port="2048"
 *    unet_multicast_ip="224.0.0.1"
 *    unet_multicast_iface="192.168.1.1"
 *    unet_multicast_port2="2049"
 *    unet_multicast_ip2="225.0.0.1"
 *    unet_multicast_iface2="192.169.1.1"
 *    />
  * ...
 * </nodes>
 */
std::unique_ptr<MulticastReceiveTransport> MulticastReceiveTransport::createFromXml( UniXML::iterator root, UniXML::iterator it, int numChan )
{
    ostringstream fieldIp;
    fieldIp << "unet_multicast_ip";

    if( numChan > 0 )
        fieldIp << numChan;

    const string h = it.getProp2(fieldIp.str(), root.getProp(fieldIp.str()));

    if( h.empty() )
    {
        ostringstream err;
        err << "(MulticastReceiveTransport): Unknown multicast IP for " << it.getProp("name");
        throw uniset::SystemError(err.str());
    }

    Poco::Net::IPAddress a(h);

    if( !a.isMulticast() && !a.isWildcard() )
    {
        ostringstream err;
        err << "(MulticastReceiveTransport): " << h << " is not multicast address or 0.0.0.0";
        throw SystemError(err.str());
    }

    ostringstream fieldPort;
    fieldPort << "unet_multicast_port";

    if( numChan > 0 )
        fieldPort << numChan;

    int p = it.getPIntProp(fieldPort.str(), it.getIntProp("id"));

    ostringstream ifaceField;
    ifaceField << "unet_multicast_iface";

    if( numChan > 0 )
        ifaceField << numChan;

    const string iface = it.getProp2(ifaceField.str(), root.getProp(ifaceField.str()) );

    std::vector<Poco::Net::IPAddress> groups{a};
    return unisetstd::make_unique<MulticastReceiveTransport>(h, p, std::move(groups), iface);
}
// -------------------------------------------------------------------------
MulticastReceiveTransport::MulticastReceiveTransport( const std::string& _bind, int _port,
        const std::vector<Poco::Net::IPAddress>& _joinGroups,
        const std::string& _iface ):
    host(_bind),
    port(_port),
    groups(_joinGroups),
    ifaceaddr(_iface)
{
    if( !ifaceaddr.empty() )
    {
        try
        {
            Poco::Net::NetworkInterface iface;

            try
            {
                iface = Poco::Net::NetworkInterface::forName(ifaceaddr);
            }
            catch(...) {}

            if( iface.name().empty() )
                iface = Poco::Net::NetworkInterface::forAddress(Poco::Net::IPAddress(ifaceaddr));

            if( iface.name().empty() )
            {
                ostringstream err;
                err << "(MulticastReceiveTransport): Not found interface for '" << ifaceaddr << "'";
                throw uniset::SystemError(err.str());
            }
        }
        catch( const Poco::Net::InterfaceNotFoundException& ex )
        {
            ostringstream err;
            err << "(MulticastReceiveTransport): Not found interface for '" << ifaceaddr << "'";
            throw uniset::SystemError(err.str());
        }
        catch( const std::exception& ex )
        {
            ostringstream err;
            err << "(MulticastReceiveTransport): Not found interface for '" << ifaceaddr
                << "' err: " << ex.what();
            throw uniset::SystemError(err.str());
        }
    }
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
bool MulticastReceiveTransport::isConnected() const noexcept
{
    return udp != nullptr;
}
// -------------------------------------------------------------------------
std::string MulticastReceiveTransport::ID() const noexcept
{
    return toString();
}
// -------------------------------------------------------------------------
std::string MulticastReceiveTransport::toString() const noexcept
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
        Poco::Net::NetworkInterface iface;

        if( ifaceaddr.empty() )
            iface.addAddress(Poco::Net::IPAddress()); // INADDR_ANY
        else
        {
            try
            {
                iface = Poco::Net::NetworkInterface::forName(ifaceaddr);
            }
            catch(...) {}

            if( iface.name().empty() )
                iface = Poco::Net::NetworkInterface::forAddress(Poco::Net::IPAddress(ifaceaddr));

            if( iface.name().empty() )
            {
                if( throwEx )
                {
                    ostringstream err;
                    err << "(MulticastReceiveTransport): Not found interface or address " << ifaceaddr;
                    throw uniset::SystemError(err.str());
                }

                return false;
            }
        }

        udp = unisetstd::make_unique<MulticastSocketU>(host, port);
        udp->setBlocking(!noblock);

        for( const auto& s : groups )
            udp->joinGroup(s, iface);
    }
    catch( const Poco::Net::InterfaceNotFoundException& ex )
    {
        if( throwEx )
        {
            ostringstream err;
            err << "(MulticastReceiveTransport): Not found interface for address " << ifaceaddr;
            throw uniset::SystemError(err.str());
        }
    }
    catch( const std::exception& e )
    {
        udp = nullptr;

        if( throwEx )
        {
            ostringstream s;
            s << host << ":" << port << "(createConnection): " << e.what();
            throw uniset::SystemError(s.str());
        }
    }
    catch( ... )
    {
        udp = nullptr;

        if( throwEx )
        {
            ostringstream s;
            s << host << ":" << port << "(createConnection): catch...";
            throw uniset::SystemError(s.str());
        }
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
bool MulticastReceiveTransport::isReadyForReceive( timeout_t tout ) noexcept
{
    try
    {
        return udp->poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_READ);
    }
    catch(...) {}

    return false;
}
// -------------------------------------------------------------------------
int MulticastReceiveTransport::available()
{
    return udp->available();
}
// -------------------------------------------------------------------------
std::vector<Poco::Net::IPAddress> MulticastReceiveTransport::getGroups()
{
    return groups;
}
// -------------------------------------------------------------------------
void MulticastReceiveTransport::setLoopBack( bool state )
{
    if( udp )
        udp->setLoopback(state);
}
// -------------------------------------------------------------------------
std::string MulticastReceiveTransport::iface() const
{
    return ifaceaddr;
}
// -------------------------------------------------------------------------
/*
 * <nodes unet_multicast_ip=".." unet_multicast_iface=".." >
 * <item id="3000"
 *    unet_multicast_port="2048"
 *    unet_multicast_ip="224.0.0.1"
 *    unet_multicast_iface="192.168.1.1"
 *    unet_multicast_port2="2049"
 *    unet_multicast_ip2="225.0.0.1"
 *    unet_multicast_iface="eth2"
 *    unet_multicast_ttl="3"/>
 *    ...
 *  </nodes>
 */
std::unique_ptr<MulticastSendTransport> MulticastSendTransport::createFromXml( UniXML::iterator root, UniXML::iterator it, int numChan )
{
    ostringstream fieldIp;
    fieldIp << "unet_multicast_ip";

    if( numChan > 0 )
        fieldIp << numChan;

    const string h = it.getProp2(fieldIp.str(), root.getProp(fieldIp.str()));

    if( h.empty() )
    {
        ostringstream err;
        err << "(MulticastSendTransport): Undefined " << fieldIp.str() << " for " << it.getProp("name");
        throw uniset::SystemError(err.str());
    }

    Poco::Net::IPAddress a(h);

    if( !a.isMulticast() )
    {
        ostringstream err;
        err << "(MulticastSendTransport): " << h << " is not multicast";
        throw SystemError(err.str());
    }

    ostringstream fieldPort;
    fieldPort << "unet_multicast_port";

    if( numChan > 0 )
        fieldPort << numChan;

    int p = it.getPIntProp(fieldPort.str(), it.getIntProp("id"));

    if( p <= 0 )
    {
        ostringstream err;
        err << "(MulticastSendTransport): Undefined " << fieldPort.str() << " for " << it.getProp("name");
        throw SystemError(err.str());
    }

    ostringstream ipIface;
    ipIface << "unet_multicast_iface";

    ostringstream ipField;
    ipField << "unet_multicast_sender_ip";

    if( numChan > 0 )
    {
        ipField << numChan;
        ipIface << numChan;
    }

    string ip = it.getProp2(ipField.str(), it.getProp(ipIface.str()));

    if( ip.empty() )
        ip = root.getProp2(ipField.str(), root.getProp(ipIface.str()));

    if( ip.empty() )
    {
        ostringstream err;
        err << "(MulticastSendTransport): Undefined sender ip " << ipField.str() << " for " << it.getProp("name");
        throw SystemError(err.str());
    }

    Poco::Net::NetworkInterface iface;

    try
    {
        // check if ip is iface
        iface = Poco::Net::NetworkInterface::forName(ip);
    }
    catch( const std::exception& ex ) {}

    if( !iface.name().empty() )
    {
        auto al = iface.addressList();

        if( al.empty() )
        {
            ostringstream err;
            err << "(MulticastSendTransport): Unknown ip for interface " << ip;
            throw SystemError(err.str());
        }

        // get first IP
        ip = al[0].get<0>().toString();
    }

    int ttl = it.getPIntProp("unet_multicast_ttl", root.getPIntProp("unet_multicast_ttl", 1));
    return unisetstd::make_unique<MulticastSendTransport>(ip, p, h, p, ttl);
}
// -------------------------------------------------------------------------
MulticastSendTransport::MulticastSendTransport( const std::string& _host, int _port, const std::string& grHost, int grPort, int _ttl ):
    sockAddr(_host, _port),
    toAddr(grHost, grPort),
    ttl(_ttl)
{

}
// -------------------------------------------------------------------------
MulticastSendTransport::~MulticastSendTransport()
{
    if( udp )
    {
        try
        {
            udp->close();
        }
        catch (...) {}
    }
}
// -------------------------------------------------------------------------
std::string MulticastSendTransport::toString() const noexcept
{
    return sockAddr.toString();
}
// -------------------------------------------------------------------------
bool MulticastSendTransport::isConnected() const noexcept
{
    return udp != nullptr;
}
// -------------------------------------------------------------------------
void MulticastSendTransport::setTimeToLive( int _ttl )
{
    ttl = _ttl;

    if( udp )
        udp->setTimeToLive(_ttl);
}
// -------------------------------------------------------------------------
void MulticastSendTransport::setLoopBack( bool state )
{
    if( udp )
        udp->setLoopback(state);
}
// -------------------------------------------------------------------------
bool MulticastSendTransport::createConnection( bool throwEx, timeout_t sendTimeout )
{
    try
    {
        udp = unisetstd::make_unique<MulticastSocketU>(sockAddr);
        udp->setSendTimeout( UniSetTimer::millisecToPoco(sendTimeout) );
        udp->setTimeToLive(ttl);
    }
    catch( const std::exception& e )
    {
        udp = nullptr;

        if( throwEx )
        {
            ostringstream s;
            s << sockAddr.toString() << "(createConnection): " << e.what();
            throw uniset::SystemError(s.str());
        }
    }
    catch( ... )
    {
        udp = nullptr;

        if( throwEx )
        {
            ostringstream s;
            s << sockAddr.toString() << "(createConnection): catch...";
            throw uniset::SystemError(s.str());
        }
    }

    return (udp != nullptr);
}
// -------------------------------------------------------------------------
int MulticastSendTransport::getSocket() const
{
    return udp->getSocket();
}
// -------------------------------------------------------------------------
bool MulticastSendTransport::isReadyForSend( timeout_t tout ) noexcept
{
    try
    {
        return udp && udp->poll( UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_WRITE );
    }
    catch(...) {}

    return false;
}
// -------------------------------------------------------------------------
ssize_t MulticastSendTransport::send( const void* buf, size_t sz )
{
    return udp->sendTo(buf, sz, toAddr);
}
// -------------------------------------------------------------------------
Poco::Net::SocketAddress MulticastSendTransport::getGroupAddress()
{
    return toAddr;
}
// -------------------------------------------------------------------------

