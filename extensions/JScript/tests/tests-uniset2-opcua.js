load("uniset2-opcua-client.js");

const OPCUA_TEST_ENDPOINT = "opc.tcp://127.0.0.1:15480";
const OPCUA_NODE_INT = "ns=1;s=TestInt";
const OPCUA_NODE_FLOAT = "ns=1;s=TestFloat";
const OPCUA_NODE_BOOL = "ns=1;s=TestBool";

function runOpcuaIntegrationTest(endpoint)
{
    const result = { success: true, intValue: 0, boolValue: 0, floatValue: 0 };
    const client = uniset_createOpcuaClient();

    try
    {
        client.connect({ endpoint: endpoint || OPCUA_TEST_ENDPOINT });

        const status = client.write([
            { nodeId: OPCUA_NODE_INT, value: 55 },
            { nodeId: OPCUA_NODE_FLOAT, value: 18.5, type: "float" },
            { nodeId: OPCUA_NODE_BOOL, value: true, type: "bool" }
        ]);

        if( status !== 0 )
            throw new Error("OPC UA write returned status " + status);

        const values = client.read([OPCUA_NODE_INT, OPCUA_NODE_FLOAT, OPCUA_NODE_BOOL], { keepArray: true });

        if( !Array.isArray(values) || values.length !== 3 )
            throw new Error("OPC UA read invalid reply");

        result.intValue = values[0].value;
        result.floatValue = values[1].value;
        result.boolValue = values[2].value ? 1 : 0;

        client.disconnect();
    }
    catch( e )
    {
        result.success = false;
        result.error = e && e.message ? e.message : "" + e;

        try
        {
            client.disconnect();
        }
        catch( _ ) {}
    }

    return result;
}
