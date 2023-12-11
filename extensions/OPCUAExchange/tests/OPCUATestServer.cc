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
    opcConfig->maxSessionTimeout = 5000;
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
        it->second->node.writeValueScalar(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, num), std::to_string(num));
    vnode.writeDataType(opcua::DataTypeId::Int32);
    vnode.writeValueScalar(val);
    vnode.writeDisplayName({"en", std::to_string(num)});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    imap.emplace(num, unisetstd::make_unique<IONode>(vnode));
    //    setX(num, val, opcua::DataTypeId::Int32);
}
// -------------------------------------------------------------------------
void OPCUATestServer::setF32( const std::string& varname, float val )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.writeValueScalar(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, varname), varname);
    vnode.writeDataType(opcua::DataTypeId::Float);
    vnode.writeValueScalar(val);
    vnode.writeDisplayName({"en", varname});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    smap.emplace(varname, unisetstd::make_unique<IONode>(vnode));
}
// -------------------------------------------------------------------------
void OPCUATestServer::setX( int num, int32_t val, opcua::DataTypeId type )
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
        case opcua::DataTypeId::Int32:
            it->second->node.writeValueScalar(val);
            break;

        case opcua::DataTypeId::UInt32:
            it->second->node.writeValueScalar((uint32_t) val);
            break;

        case opcua::DataTypeId::Int16:
            it->second->node.writeValueScalar((int16_t) val);
            break;

        case opcua::DataTypeId::UInt16:
            it->second->node.writeValueScalar((uint16_t) val);
            break;

        case opcua::DataTypeId::Int64:
            it->second->node.writeValueScalar((int64_t) val);
            break;

        case opcua::DataTypeId::UInt64:
            it->second->node.writeValueScalar((uint64_t) val);
            break;

        case opcua::DataTypeId::Byte:
            it->second->node.writeValueScalar((uint8_t) val);
            break;

        default:
            it->second->node.writeValueScalar((int32_t)val);
            break;
    };

}
// -------------------------------------------------------------------------
void OPCUATestServer::setI32( const std::string& varname, int32_t val )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.writeValueScalar(val);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, varname), varname);
    vnode.writeDataType(opcua::DataTypeId::Int32);
    vnode.writeDisplayName({"en", varname});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.writeValueScalar(val);
    smap[varname] = unisetstd::make_unique<IONode>(vnode);
}
// -------------------------------------------------------------------------
void OPCUATestServer::setBool( const std::string& varname, bool set )
{
    auto it = smap.find(varname);

    if( it != smap.end() )
    {
        it->second->node.writeValueScalar(set);
        return;
    }

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, varname), varname);
    vnode.writeDataType(opcua::DataTypeId::Boolean);
    vnode.writeDisplayName({"en", varname});
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.writeValueScalar(set);
    smap[varname] = unisetstd::make_unique<IONode>(vnode);
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getX( int num, opcua::DataTypeId type )
{
    auto it = imap.find(num);

    if( it == imap.end() )
        return 0;

    if( type  == opcua::DataTypeId::Int32 )
        return (int32_t)it->second->node.readValueScalar<int32_t>();

    if( type  == opcua::DataTypeId::UInt32 )
        return (int32_t)it->second->node.readValueScalar<uint32_t>();

    if( type  == opcua::DataTypeId::Int16 )
        return (int32_t)it->second->node.readValueScalar<int16_t>();

    if( type  == opcua::DataTypeId::UInt16 )
        return (int32_t)it->second->node.readValueScalar<uint16_t>();

    if( type  == opcua::DataTypeId::Int64 )
        return (int32_t)it->second->node.readValueScalar<int64_t>();

    if( type  == opcua::DataTypeId::UInt64 )
        return (int32_t)it->second->node.readValueScalar<uint64_t>();

    if( type  == opcua::DataTypeId::Byte )
        return (int32_t)it->second->node.readValueScalar<uint8_t>();

    return 0;
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getI32( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.readValueScalar<int32_t>();

    return 0;
}
// -------------------------------------------------------------------------
int32_t OPCUATestServer::getI32( int num )
{
    auto it = imap.find(num);

    if( it != imap.end() )
        return it->second->node.readValueScalar<int32_t>();

    return 0;
}
// -------------------------------------------------------------------------
float OPCUATestServer::getF32( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.readValueScalar<float>();

    return 0;
}
// -------------------------------------------------------------------------
bool OPCUATestServer::getBool( const std::string& name )
{
    auto it = smap.find(name);

    if( it != smap.end() )
        return it->second->node.readValueScalar<bool>();

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
