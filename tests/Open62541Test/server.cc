#include <iostream>

#include "open62541pp/open62541pp.hpp"

int main()
{
    opcua::ServerConfig conf;
    conf.setApplicationName("open62541pp server example");
    conf.setLogger([](opcua::LogLevel level, opcua::LogCategory category, auto msg)
    {
        std::cout
                << "[" << (int)level << "] "
                << "[" << (int)category << "] "
                << msg << std::endl;
    });

    opcua::Server server(std::move(conf));

    const auto        myIntegerNodeId = opcua::NodeId(1, "the.answer");
    const std::string myIntegerName   = "the answer";

    // create node
    opcua::Node parentNode{server, opcua::ObjectId::ObjectsFolder};
    opcua::Node myIntegerNode = parentNode.addVariable(myIntegerNodeId, myIntegerName);

    // set node attributes
    myIntegerNode.writeDisplayName({"en-US", "the answer"});
    myIntegerNode.writeDescription({"en-US", "the answer"});

    // write value
    myIntegerNode.writeValue(opcua::Variant{42});

    // read value
    std::cout << "The answer is: " << myIntegerNode.readValue().to<int>() << std::endl;

    server.run();

    return 0;
}
