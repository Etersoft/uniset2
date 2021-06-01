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
 *          <item id="3000" unet_port="2048" unet_multicast_ip="192.168.0.255" unet_port2="2048" unet_multicast_ip2="192.169.0.255">
                <multicast>
                    <receive>
                        <group addr="224.0.0.1" addr2="224.0.0.1"/>
                    </receive>
                    <send>
                        <group addr="224.0.0.1"/>
                    </send>
                </multicast>
            </item>
 */
std::unique_ptr<MulticastReceiveTransport> MulticastReceiveTransport::createFromXml( UniXML::iterator it, const std::string& defaultIP, int numChan, const std::string& section )
{
    ostringstream fieldIp;
    fieldIp << "unet_multicast_ip";

    if( numChan > 0 )
        fieldIp << numChan;

    const string h = it.getProp2(fieldIp.str(), defaultIP);

    if( h.empty() )
    {
        ostringstream err;
        err << "(MulticastReceiveTransport): Unknown multicast IP for " << it.getProp("name");
        throw uniset::SystemError(err.str());
    }

    ostringstream fieldPort;
    fieldPort << "unet_multicast_port";

    if( numChan > 0 )
        fieldPort << numChan;

    int p = it.getPIntProp(fieldPort.str(), it.getIntProp("id"));

    if( !it.find("multicast") )
        throw SystemError("(MulticastReceiveTransport): not found <multicast> node");

    if( !it.goChildren() )
        throw SystemError("(MulticastReceiveTransport): empty <multicast> node");

    if( !it.find(section) )
        throw SystemError("(MulticastReceiveTransport): not found <" + section + "> in <multicast>");

    if( !it.goChildren() )
        throw SystemError("(MulticastReceiveTransport): empty <" + section + "> groups");

    std::vector<Poco::Net::IPAddress> groups;

    ostringstream fieldAddr;
    fieldAddr << "addr";

    if( numChan > 0 )
        fieldAddr << numChan;

    for( ; it; it++ )
    {
        Poco::Net::IPAddress a(it.getProp(fieldAddr.str()), Poco::Net::IPAddress::IPv4);

        if( !a.isMulticast() )
        {
            ostringstream err;
            err << "(MulticastReceiveTransport): " << it.getProp(fieldAddr.str()) << " is not multicast address";
            throw SystemError(err.str());
        }

        groups.push_back(a);
    }

    return unisetstd::make_unique<MulticastReceiveTransport>(h, p, std::move(groups));
}
// -------------------------------------------------------------------------
MulticastReceiveTransport::MulticastReceiveTransport( const std::string& _bind, int _port, const std::vector<Poco::Net::IPAddress>& _joinGroups ):
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
bool MulticastReceiveTransport::isReadyForReceive( timeout_t tout )
{
    return udp->poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_READ);
}
// -------------------------------------------------------------------------
std::vector<Poco::Net::IPAddress> MulticastReceiveTransport::getGroups()
{
    return groups;
}

// -------------------------------------------------------------------------
/*
 *          <item id="3000" unet_port="2048" unet_multicast_ip="192.168.0.255" unet_port2="2048" unet_multicast_ip2="192.169.0.255">
                <multicast>
                    <receive>
                        <group addr="224.0.0.1" addr2="224.0.0.1"/>
                    </receive>
                    <send>
                        <group addr="224.0.0.1"/>
                    </send>
                </multicast>
            </item>
 */
std::unique_ptr<MulticastSendTransport> MulticastSendTransport::createFromXml( UniXML::iterator it, const std::string& defaultIP, int numChan )
{
    ostringstream fieldIp;
    fieldIp << "unet_multicast_ip";

    if( numChan > 0 )
        fieldIp << numChan;

    const string h = it.getProp2(fieldIp.str(), defaultIP);

    if( h.empty() )
    {
        ostringstream err;
        err << "(MulticastSendTransport): Unknown multicast IP for " << it.getProp("name");
        throw uniset::SystemError(err.str());
    }

    ostringstream fieldPort;
    fieldPort << "unet_multicast_port";

    if( numChan > 0 )
        fieldPort << numChan;

    int p = it.getPIntProp(fieldPort.str(), it.getIntProp("id"));

    if( !it.find("multicast") )
        throw SystemError("(MulticastSendTransport): not found <multicast> node");

    if( !it.goChildren() )
        throw SystemError("(MulticastSendTransport): empty <multicast> node");

    if( !it.find("send") )
        throw SystemError("(MulticastSendTransport): not found <send> node");

    int ttl = it.getPIntProp("ttl", 1);

    if( !it.goChildren() )
        throw SystemError("(MulticastSendTransport): empty <send> groups");

    ostringstream fieldAddr;
    fieldAddr << "addr";

    if( numChan > 0 )
        fieldAddr << numChan;

    std::vector<Poco::Net::IPAddress> groups;

    for( ; it; it++ )
    {
        Poco::Net::IPAddress a(it.getProp(fieldAddr.str()), Poco::Net::IPAddress::IPv4);

        if( !a.isMulticast() )
        {
            ostringstream err;
            err << "(MulticastSendTransport): " << it.getProp(fieldAddr.str()) << " is not multicast address";
            throw SystemError(err.str());
        }

        groups.push_back(a);
    }

    return unisetstd::make_unique<MulticastSendTransport>(h, p, std::move(groups), ttl);
}
// -------------------------------------------------------------------------
MulticastSendTransport::MulticastSendTransport( const std::string& _host, int _port, const std::vector<Poco::Net::IPAddress>& _sendGroups, int _ttl ):
    saddr(_host, _port),
    groups(_sendGroups),
    ttl(_ttl)
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
void MulticastSendTransport::setTimeToLive( int _ttl )
{
    ttl = ttl;

    if( udp )
        udp->setTimeToLive(_ttl);
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
        udp->setTimeToLive(ttl);
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
ssize_t MulticastSendTransport::send( const void* buf, size_t sz )
{
    return udp->sendTo(buf, sz, saddr);
}
// -------------------------------------------------------------------------
std::vector<Poco::Net::IPAddress> MulticastSendTransport::getGroups()
{
    return groups;
}
