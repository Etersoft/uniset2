#include <iostream>

#include "open62541pp/open62541pp.h"

int main()
{
    opcua::Server server;
    server.setApplicationName("open62541pp server example");

    server.setLogger([](opcua::LogLevel level, opcua::LogCategory category, auto msg)
    {
        std::cout
                << "[" << (int)level << "] "
                << "[" << (int)category << "] "
                << msg << std::endl;
    });

    const auto        myIntegerNodeId = opcua::NodeId(1, "the.answer");
    const std::string myIntegerName   = "the answer";

    // create node
    auto parentNode    = server.getObjectsNode();
    auto myIntegerNode = parentNode.addVariable(myIntegerNodeId, myIntegerName);
    myIntegerNode.writeDataType(opcua::Type::Int32);

    // set node attributes
    myIntegerNode.writeDisplayName({"en-US", "the answer"});
    myIntegerNode.writeDescription({"en-US", "the answer"});

    // write value
    myIntegerNode.writeValueScalar(42);

    // read value
    std::cout << "The answer is: " << myIntegerNode.readValueScalar<int>() << std::endl;

    server.run();

    return 0;
}