// -------------------------------------------------------------------------
extern "C" {
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
}
#include <sstream>
#include <limits>
#include <iostream>
#include "UniSetTypes.h"
#include "OPCUATestServer.h"
#include "unisetstd.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
OPCUATestServer::OPCUATestServer( const std::string& _addr, uint16_t port ):
    addr(_addr)
{
    if( verbose )
        cout << "(OPCUATestServer::init): addr=" << addr << ":" << port << endl;

    server = unisetstd::make_unique<opcua::Server>((uint16_t)port);
    server->setApplicationName("uniset2 OPC UA gate");
    server->setApplicationUri("urn:uniset2.server.gate");
    server->setProductUri("https://github.com/Etersoft/uniset2/");
    server->setCustomHostname(addr);

    auto opcConfig = server->getConfig();
    opcConfig->maxSubscriptions = 10;
    opcConfig->maxSessions = 10;
    opcConfig->logger = UA_Log_Stdout_withLevel( UA_LOGLEVEL_ERROR );

    auto uroot = server->getRootNode().addFolder(opcua::NodeId("uniset"), "uniset");
    uroot.setDescription("uniset i/o", "en");
    const auto uname = "localhost";
    auto unode = uroot.addFolder(opcua::NodeId(uname), uname);
    unode.setDisplayName(uname, "en");

    ioNode = unisetstd::make_unique<IONode>(unode.addFolder(opcua::NodeId("io"), "I/O"));
    ioNode->node.setDescription("I/O", "en-US");

    serverThread = make_shared< ThreadCreator<OPCUATestServer> >(this, &OPCUATestServer::work);
}

// -------------------------------------------------------------------------
OPCUATestServer::~OPCUATestServer()
{
}
// -------------------------------------------------------------------------
void OPCUATestServer::setI32( int num, int32_t val )
{
    auto it = imap.find(num);

    if( it != imap.end() )
    {
        it->second->node.write(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(num), std::to_string(num), opcua::Type::Int32);
    vnode.write(val);
    vnode.setDisplayName(std::to_string(num), "en");
    vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    imap.emplace(num, unisetstd::make_unique<IONode>(vnode));
    //    setX(num, val, opcua::Type::Int32);
}
// -------------------------------------------------------------------------
void OPCUATestServer::setX( int num, int32_t val, opcua::Type type )
{
    auto it = imap.find(num);

    if( it == imap.end() )
    {
        auto vnode = ioNode->node.addVariable(opcua::NodeId(num), std::to_string(num), type);
        vnode.setDisplayName(std::to_string(num), "en");
        vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        auto ret = imap.emplace(num, unisetstd::make_unique<IONode>(vnode));
        it = ret.first;
    }

    switch(type)
    {
        case opcua::Type::Int32:
            it->second->node.write(val);
            break;

        case opcua::Type::UInt32:
            it->second->node.write((uint32_t) val);
            break;

        case opcua::Type::Int16:
            it->second->node.write((int16_t) val);
            break;

        case opcua::Type::UInt16:
            it->second->node.write((uint16_t) val);
            break;

        case opcua::Type::Int64:
            it->second->node.write((int64_t) val);
            break;

        case opcua::Type::UInt64:
            it->second->node.write((uint64_t) val);
            break;

        case opcua::Type::Byte:
            it->second->node.write((uint8_t) val);
            break;

        default:
            it->second->node.write((int32_t)val);
            break;
    };

}
// -------------------------------------------------------------------------
void OPCUATestServer::setI32( const std::string& varname, int32_t val )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.write(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(varname), varname, opcua::Type::Int32);
    vnode.setDisplayName(varname, "en");
    vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.write(val);
    smap[varname] = unisetstd::make_unique<IONode>(vnode);
}
// -------------------------------------------------------------------------
void OPCUATestServer::setBool( const std::string& varname, bool set )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.write(set);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(varname), varname, opcua::Type::Boolean);
    vnode.setDisplayName(varname, "en");
    vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.write(set);
    smap[varname] = unisetstd::make_unique<IONode>(vnode);
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getX( int num, opcua::Type type )
{
    auto it = imap.find(num);

    if( it == imap.end() )
        return 0;

    if( type  == opcua::Type::Int32 )
        return (int32_t)it->second->node.read<int32_t>();

    if( type  == opcua::Type::UInt32 )
        return (int32_t)it->second->node.read<uint32_t>();

    if( type  == opcua::Type::Int16 )
        return (int32_t)it->second->node.read<int16_t>();

    if( type  == opcua::Type::UInt16 )
        return (int32_t)it->second->node.read<uint16_t>();

    if( type  == opcua::Type::Int64 )
        return (int32_t)it->second->node.read<int64_t>();

    if( type  == opcua::Type::UInt64 )
        return (int32_t)it->second->node.read<uint64_t>();

    if( type  == opcua::Type::Byte )
        return (int32_t)it->second->node.read<uint8_t>();

    return 0;
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getI32( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.read<int32_t>();

    return 0;
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getI32( int num )
{
    auto it = imap.find(num);

    if( it != imap.end() )
        return it->second->node.read<int32_t>();

    return 0;
}
// -------------------------------------------------------------------------
bool OPCUATestServer::getBool( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.read<bool>();

    return false;
}
// -------------------------------------------------------------------------
bool OPCUATestServer::isRunning()
{
    return server->isRunning();
}
// -------------------------------------------------------------------------
void OPCUATestServer::start()
{
    serverThread->start();
}
// -------------------------------------------------------------------------
void OPCUATestServer::work()
{
    server->run();
}
// -------------------------------------------------------------------------
void OPCUATestServer::stop()
{
    server->stop();
}
// -------------------------------------------------------------------------
