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

    auto opcConfig = UA_Server_getConfig(server->handle());
    opcConfig->maxSubscriptions = 10;
    opcConfig->maxSessions = 10;
    opcConfig->logger = UA_Log_Stdout_withLevel( UA_LOGLEVEL_ERROR );

    auto uroot = server->getRootNode().addFolder(opcua::NodeId(0, "uniset"), "uniset");
    uroot.writeDescription({"en", "uniset i/o"});
    const auto uname = "localhost";
    auto unode = uroot.addFolder(opcua::NodeId(0, uname), uname);
    unode.writeDisplayName({"en", uname});

    ioNode = unisetstd::make_unique<IONode>(unode.addFolder(opcua::NodeId(0, "io"), "I/O"));
    ioNode->node.writeDescription({"en-US", "I/O"});

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
        it->second->node.writeScalar(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, num), std::to_string(num));
    vnode.writeDataType(opcua::Type::Int32);
    vnode.writeScalar(val);
    vnode.writeDisplayName({"en", std::to_string(num)});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    imap.emplace(num, unisetstd::make_unique<IONode>(vnode));
    //    setX(num, val, opcua::Type::Int32);
}
// -------------------------------------------------------------------------
void OPCUATestServer::setF32( const std::string& varname, float val )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.writeScalar(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, varname), varname);
    vnode.writeDataType(opcua::Type::Float);
    vnode.writeScalar(val);
    vnode.writeDisplayName({"en", varname});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    smap.emplace(varname, unisetstd::make_unique<IONode>(vnode));
}
// -------------------------------------------------------------------------
void OPCUATestServer::setX( int num, int32_t val, opcua::Type type )
{
    auto it = imap.find(num);

    if( it == imap.end() )
    {
        auto vnode = ioNode->node.addVariable(opcua::NodeId(0, num), std::to_string(num));
        vnode.writeDataType(type);
        vnode.writeDisplayName({ "en", std::to_string(num)});
        vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        auto ret = imap.emplace(num, unisetstd::make_unique<IONode>(vnode));
        it = ret.first;
    }

    switch(type)
    {
        case opcua::Type::Int32:
            it->second->node.writeScalar(val);
            break;

        case opcua::Type::UInt32:
            it->second->node.writeScalar((uint32_t) val);
            break;

        case opcua::Type::Int16:
            it->second->node.writeScalar((int16_t) val);
            break;

        case opcua::Type::UInt16:
            it->second->node.writeScalar((uint16_t) val);
            break;

        case opcua::Type::Int64:
            it->second->node.writeScalar((int64_t) val);
            break;

        case opcua::Type::UInt64:
            it->second->node.writeScalar((uint64_t) val);
            break;

        case opcua::Type::Byte:
            it->second->node.writeScalar((uint8_t) val);
            break;

        default:
            it->second->node.writeScalar((int32_t)val);
            break;
    };

}
// -------------------------------------------------------------------------
void OPCUATestServer::setI32( const std::string& varname, int32_t val )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.writeScalar(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, varname), varname);
    vnode.writeDataType(opcua::Type::Int32);
    vnode.writeDisplayName({"en", varname});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.writeScalar(val);
    smap[varname] = unisetstd::make_unique<IONode>(vnode);
}
// -------------------------------------------------------------------------
void OPCUATestServer::setBool( const std::string& varname, bool set )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.writeScalar(set);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, varname), varname);
    vnode.writeDataType(opcua::Type::Boolean);
    vnode.writeDisplayName({"en", varname});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.writeScalar(set);
    smap[varname] = unisetstd::make_unique<IONode>(vnode);
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getX( int num, opcua::Type type )
{
    auto it = imap.find(num);

    if( it == imap.end() )
        return 0;

    if( type  == opcua::Type::Int32 )
        return (int32_t)it->second->node.readScalar<int32_t>();

    if( type  == opcua::Type::UInt32 )
        return (int32_t)it->second->node.readScalar<uint32_t>();

    if( type  == opcua::Type::Int16 )
        return (int32_t)it->second->node.readScalar<int16_t>();

    if( type  == opcua::Type::UInt16 )
        return (int32_t)it->second->node.readScalar<uint16_t>();

    if( type  == opcua::Type::Int64 )
        return (int32_t)it->second->node.readScalar<int64_t>();

    if( type  == opcua::Type::UInt64 )
        return (int32_t)it->second->node.readScalar<uint64_t>();

    if( type  == opcua::Type::Byte )
        return (int32_t)it->second->node.readScalar<uint8_t>();

    return 0;
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getI32( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.readScalar<int32_t>();

    return 0;
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getI32( int num )
{
    auto it = imap.find(num);

    if( it != imap.end() )
        return it->second->node.readScalar<int32_t>();

    return 0;
}
// -------------------------------------------------------------------------
float OPCUATestServer::getF32( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.readScalar<float>();

    return 0;
}
// -------------------------------------------------------------------------
bool OPCUATestServer::getBool( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.readScalar<bool>();

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
