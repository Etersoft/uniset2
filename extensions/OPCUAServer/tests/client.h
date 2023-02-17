#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>


static UA_Client* client = nullptr;

static bool opcuaCreateClient( const std::string& addr )
{
    if( client != nullptr )
        return true;

    client = UA_Client_new();

    if( client == nullptr )
        return false;

    UA_StatusCode retval = UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    if( retval != UA_STATUSCODE_GOOD )
    {
        UA_Client_delete(client);
        client = nullptr;
        return false;
    }

    retval = UA_Client_connect(client, addr.c_str());

    if( retval != UA_STATUSCODE_GOOD )
    {
        UA_Client_delete(client);
        client = nullptr;
        return false;
    }

    return true;
}

//static void opcuaDeleteClient()
//{
//    if( client == nullptr )
//        return;
//
//    UA_Client_disconnect(client);
//    UA_Client_delete(client);
//    client = nullptr;
//}

static long opcuaRead( uint16_t nodeId, const std::string& attrName )
{
    if( client == nullptr )
        return 0;

    UA_Int32 value = 0;
    UA_Variant* val = UA_Variant_new();
    UA_StatusCode retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING((UA_UInt16)nodeId, (char*)attrName.c_str()), val);

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) &&
            val->type == &UA_TYPES[UA_TYPES_INT32])
    {
        value = *(UA_Int32*)val->data;
    }

    UA_Variant_delete(val);
    return value;
}

static bool opcuaWrite( uint16_t nodeId, const std::string& attrName, long value )
{
    if( client == nullptr )
        return 0;

    UA_Variant* myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING((UA_UInt16)nodeId, (char*)attrName.c_str()), myVariant);
    UA_Variant_delete(myVariant);
    return retval == UA_STATUSCODE_GOOD;
}

static bool opcuaWriteBool( uint16_t nodeId, const std::string& attrName, bool set )
{
    if( client == nullptr )
        return 0;

    UA_Variant* myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &set, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING((UA_UInt16)nodeId, (char*)attrName.c_str()), myVariant);
    UA_Variant_delete(myVariant);
    return retval == UA_STATUSCODE_GOOD;
}

static bool opcuaReadBool( uint16_t nodeId, const std::string& attrName )
{
    if( client == nullptr )
        return false;

    UA_Boolean value = 0;
    UA_Variant* val = UA_Variant_new();
    UA_StatusCode retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING((UA_UInt16)nodeId, (char*)attrName.c_str()), val);

    if(retval == UA_STATUSCODE_GOOD && val->type == &UA_TYPES[UA_TYPES_BOOLEAN])
    {
        value = *(UA_Boolean*)val->data;
    }

    UA_Variant_delete(val);
    return value;
}
