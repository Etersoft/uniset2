/*
 * Simplified OPC UA server for JS tests.
 */
#include "OPCUATestServer.h"

extern "C" {
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
}

#include "unisetstd.h"
#include "Exceptions.h"
#include "PassiveTimer.h"
#include "UniSetTypes.h"
#include "unisetstd.h"

using namespace std;
using namespace uniset;

opcua::Node<opcua::Server> createFolder(opcua::Server& srv,
                                        opcua::Node<opcua::Server>& parent,
                                        const std::string& name )
{
    auto node = parent.addFolder(opcua::NodeId(1, name), name);
    node.writeDisplayName({"en-US", name});
    node.writeDescription({"en-US", name});
    return node;
}


OPCUATestServer::OPCUATestServer( const std::string& h, uint16_t p ):
    host(h),
    port(p)
{
    opcua::ServerConfig opcconf(p);
    opcconf.setApplicationName("uniset2-opcua-tests");
    opcconf.setApplicationUri("urn:uniset2.tests.opcua");
    opcconf.setProductUri("https://github.com/Etersoft/uniset2/");
//    opcconf.setCustomHostname(host);

    server = unisetstd::make_unique<opcua::Server>(std::move(opcconf));

    auto cfg = UA_Server_getConfig(server->handle());
    auto slogger = UA_Log_Stdout_new(UA_LOGLEVEL_ERROR);
    cfg->logging = slogger;
    cfg->maxSubscriptions = 8;
    cfg->maxSessions = 4;

    opcua::Node<opcua::Server> rnode{*server, opcua::ObjectId::ObjectsFolder};
    auto testsFolder = createFolder(*server, rnode, "unisetTests");
    ioNode = createFolder(*server, testsFolder, "io");

    //    setInt("TestInt", 10);
    //    setFloat("TestFloat", 3.14f);
    //    setBool("TestBool", false);
    setInt(kNodeInt, 10);
    setFloat(kNodeFloat, 3.14f);
    setBool(kNodeBool, false);
}

OPCUATestServer::~OPCUATestServer()
{
    stop();
}

bool OPCUATestServer::start()
{
    if( running.load() )
        return true;

    worker = std::thread([this]()
    {
        running.store(true);
        server->run();
        running.store(false);
    });

    PassiveTimer waitTimer(2000);

    while( !waitTimer.checkTime() )
    {
        if( running.load() )
            return true;

        msleep(50);
    }

    return running.load();
}

void OPCUATestServer::stop()
{
    if( running.load() )
    {
        server->stop();

        if( worker.joinable() )
            worker.join();
    }
}

bool OPCUATestServer::isRunning() const noexcept
{
    return running.load();
}

void OPCUATestServer::ensureNode( const std::string& name, opcua::DataTypeId type )
{
    auto it = nodes.find(name);

    if( it != nodes.end() )
        return;

    auto vnode = ioNode->addVariable(opcua::NodeId(1, name), name);
    vnode.writeDisplayName({"en-US", name});
    vnode.writeDescription({"en-US", name});
    vnode.writeDataType(type);
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

    switch(type)
    {
        case opcua::DataTypeId::Float:
            vnode.writeValue(opcua::Variant{0.0f});
            break;

        case opcua::DataTypeId::Boolean:
            vnode.writeValue(opcua::Variant{false});
            break;

        default:
            vnode.writeValue(opcua::Variant{int32_t(0)});
            break;
    }

    nodes.emplace(name, std::make_unique<NodeEntry>(vnode, type));
}

void OPCUATestServer::writeValue( const std::string& name, opcua::DataTypeId type,
                                  const std::function<void(opcua::Node<opcua::Server>&)>& fn )
{
    ensureNode(name, type);
    auto it = nodes.find(name);

    if( it == nodes.end() )
        throw SystemError("OPCUATestServer: node not found");

    fn(it->second->node);
}

void OPCUATestServer::setInt( const std::string& name, int32_t value )
{
    writeValue(name, opcua::DataTypeId::Int32, [&](auto & node)
    {
        node.writeValue(opcua::Variant{value});
    });
}

void OPCUATestServer::setFloat( const std::string& name, float value )
{
    writeValue(name, opcua::DataTypeId::Float, [&](auto & node)
    {
        node.writeValue(opcua::Variant{value});
    });
}

void OPCUATestServer::setBool( const std::string& name, bool value )
{
    writeValue(name, opcua::DataTypeId::Boolean, [&](auto & node)
    {
        node.writeValue(opcua::Variant{value});
    });
}

int32_t OPCUATestServer::getInt( const std::string& name ) const
{
    auto it = nodes.find(name);

    if( it == nodes.end() )
        return 0;

    return it->second->node.readValueScalar<int32_t>();
}

float OPCUATestServer::getFloat( const std::string& name ) const
{
    auto it = nodes.find(name);

    if( it == nodes.end() )
        return 0.f;

    return it->second->node.readValueScalar<float>();
}

bool OPCUATestServer::getBool( const std::string& name ) const
{
    auto it = nodes.find(name);

    if( it == nodes.end() )
        return false;

    return it->second->node.readValueScalar<bool>();
}

std::string OPCUATestServer::nodeIdString( const std::string& name ) const
{
    return std::string("ns=1;s=") + name;
}
