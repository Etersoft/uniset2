/*
 * Simplified OPC UA test server for unit tests.
 */
#ifndef TESTS_OPCUA_TEST_SERVER_H_
#define TESTS_OPCUA_TEST_SERVER_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <functional>
#include <optional>

#include "open62541pp/open62541pp.hpp"
// --------------------------------------------------------------------------
static const char* kEndpointUrl = "opc.tcp://127.0.0.1:15480";
static const char* kNodeInt = "TestInt";
static const char* kNodeFloat = "TestFloat";
static const char* kNodeBool = "TestBool";
// --------------------------------------------------------------------------
class OPCUATestServer
{
    public:
        OPCUATestServer( const std::string& host, uint16_t port = 15480 );
        ~OPCUATestServer();

        bool start();
        void stop();
        bool isRunning() const noexcept;

        void setInt( const std::string& name, int32_t value );
        void setFloat( const std::string& name, float value );
        void setBool( const std::string& name, bool value );

        int32_t getInt( const std::string& name ) const;
        float getFloat( const std::string& name ) const;
        bool getBool( const std::string& name ) const;

        std::string nodeIdString( const std::string& name ) const;

    private:
        void ensureNode( const std::string& name, opcua::DataTypeId type );
        void writeValue( const std::string& name, opcua::DataTypeId type, const std::function<void(opcua::Node<opcua::Server>&)>& fn );
        void run();

        struct NodeEntry
        {
            opcua::Node<opcua::Server> node;
            opcua::DataTypeId type;

            NodeEntry(const opcua::Node<opcua::Server>& n, opcua::DataTypeId t):
                node(n), type(t) {}
        };

        std::string host;
        uint16_t port;
        std::unique_ptr<opcua::Server> server;
        std::optional<opcua::Node<opcua::Server>> ioNode;
        mutable std::unordered_map<std::string, std::unique_ptr<NodeEntry>> nodes;
        std::thread worker;
        std::atomic_bool running { false };
};

#endif // TESTS_OPCUA_TEST_SERVER_H_
