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
        cout << "(OPCUATestServer::init): addr=" << addr << endl;

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
    vnode.setDisplayName(std::to_string(num), "en");
    vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    vnode.write(val);
    imap[num] = unisetstd::make_unique<IONode>(vnode);
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
