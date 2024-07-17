#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <limits>
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "client.h"
#include "OPCUAServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static string addr  = "opc.tcp://127.0.0.1:44999";
static std::shared_ptr<UInterface> ui;
static uint16_t nodeId = 0;
static int pause_msec = 200;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = std::make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    REQUIRE( opcuaCreateClient(addr) );
}
// -----------------------------------------------------------------------------
struct nodeIterData
{
    UA_NodeId id;
    UA_Boolean isInverse;
    UA_NodeId referenceTypeID;
    UA_Boolean hit;
};

const int NODE_ITER_DATA_SIZE = 9;

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void* handle)
{
    if (isInverse || (referenceTypeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
                      referenceTypeId.identifier.numeric == UA_NS0ID_HASTYPEDEFINITION))
        return UA_STATUSCODE_GOOD;

    struct nodeIterData* objectsFolderChildren = ( struct nodeIterData*)handle;

    REQUIRE(childId.namespaceIndex == 0);

    REQUIRE(childId.identifierType == UA_NODEIDTYPE_STRING);

    int i;

    for(i = 0; i < NODE_ITER_DATA_SIZE; i++)
    {
        if(UA_NodeId_equal(&childId, &objectsFolderChildren[i].id))
        {
            break;
        }
    }

    REQUIRE(i < NODE_ITER_DATA_SIZE);
    REQUIRE(objectsFolderChildren[i].isInverse == isInverse);

    REQUIRE(!objectsFolderChildren[i].hit);
    objectsFolderChildren[i].hit = UA_TRUE;

    REQUIRE(UA_NodeId_equal(&referenceTypeId, &objectsFolderChildren[i].referenceTypeID));

    return UA_STATUSCODE_GOOD;
}

TEST_CASE("[OPCUAServer]: check structure", "[opcuaserver]")
{
    InitTest();

    /*
    Test structure:
    - localhost
      -- folder1
        --- subfolder1
          ---- AI1_S
        --- AO1_S
      -- folder2
        --- subfolder2
          ---- DI1_S
        --- DO1_S
      -- AO2_S
    */

    /* Проверка только вложенных нод в корневой каталог. Подкаталоги не проверяются! */
    struct nodeIterData objectsFolderChildren[NODE_ITER_DATA_SIZE];
    objectsFolderChildren[0].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"folder1");
    objectsFolderChildren[0].isInverse = UA_FALSE;
    objectsFolderChildren[0].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[0].hit = UA_FALSE;

    objectsFolderChildren[1].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"folder2");
    objectsFolderChildren[1].isInverse = UA_FALSE;
    objectsFolderChildren[1].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[1].hit = UA_FALSE;

    objectsFolderChildren[2].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"AO2_S");
    objectsFolderChildren[2].isInverse = UA_FALSE;
    objectsFolderChildren[2].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[2].hit = UA_FALSE;

    //folder1
    objectsFolderChildren[3].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"folder1.subfolder1");
    objectsFolderChildren[3].isInverse = UA_FALSE;
    objectsFolderChildren[3].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[3].hit = UA_FALSE;

    objectsFolderChildren[4].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"AO1_S");
    objectsFolderChildren[4].isInverse = UA_FALSE;
    objectsFolderChildren[4].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[4].hit = UA_FALSE;

    //folder1.subfolder1
    objectsFolderChildren[5].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"AI1_S");
    objectsFolderChildren[5].isInverse = UA_FALSE;
    objectsFolderChildren[5].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[5].hit = UA_FALSE;

    //folder2
    objectsFolderChildren[6].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"folder2.subfolder2");
    objectsFolderChildren[6].isInverse = UA_FALSE;
    objectsFolderChildren[6].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[6].hit = UA_FALSE;

    objectsFolderChildren[7].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"DO1_S");
    objectsFolderChildren[7].isInverse = UA_FALSE;
    objectsFolderChildren[7].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[7].hit = UA_FALSE;

    //folder2.subfolder2
    objectsFolderChildren[8].id = UA_NODEID_STRING((UA_UInt16)nodeId, (char*)"DI1_S");
    objectsFolderChildren[8].isInverse = UA_FALSE;
    objectsFolderChildren[8].referenceTypeID = UA_NODEID_NUMERIC((UA_UInt16)nodeId, UA_NS0ID_HASCOMPONENT);
    objectsFolderChildren[8].hit = UA_FALSE;

    UA_StatusCode retval =
        UA_Client_forEachChildNodeCall(client, UA_NODEID_STRING(0, (char*)"LocalhostNode"),
                                       nodeIter, &objectsFolderChildren);

    REQUIRE(retval == UA_STATUSCODE_GOOD);

    retval =
        UA_Client_forEachChildNodeCall(client, UA_NODEID_STRING(0, (char*)"folder1"),
                                       nodeIter, &objectsFolderChildren);

    REQUIRE(retval == UA_STATUSCODE_GOOD);

    retval =
        UA_Client_forEachChildNodeCall(client, UA_NODEID_STRING(0, (char*)"folder1.subfolder1"),
                                       nodeIter, &objectsFolderChildren);

    REQUIRE(retval == UA_STATUSCODE_GOOD);

    retval =
        UA_Client_forEachChildNodeCall(client, UA_NODEID_STRING(0, (char*)"folder2"),
                                       nodeIter, &objectsFolderChildren);

    REQUIRE(retval == UA_STATUSCODE_GOOD);

    retval =
        UA_Client_forEachChildNodeCall(client, UA_NODEID_STRING(0, (char*)"folder2.subfolder2"),
                                       nodeIter, &objectsFolderChildren);

    REQUIRE(retval == UA_STATUSCODE_GOOD);

    /* Check if all nodes are hit */
    for (int i = 0; i < NODE_ITER_DATA_SIZE; i++)
    {
        REQUIRE(objectsFolderChildren[i].hit == true);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAServer]: read", "[opcuaserver]")
{
    InitTest();

    ui->setValue(1, 100);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AO1_S") == 100 );

    ui->setValue(3, 1);
    msleep(pause_msec);
    REQUIRE( opcuaReadBool(nodeId, "DO1_S") );

    ui->setValue(5, 100);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AO2_S") == 100 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAServer]: write", "[opcuaserver]")
{
    InitTest();

    REQUIRE( ui->getValue(2) == 0 );
    REQUIRE( opcuaWrite(nodeId, "AI1_S", 10500) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(2) == 10500 );

    REQUIRE( ui->getValue(4) == 0 );
    REQUIRE( opcuaWriteBool(nodeId, "DI1_S", true) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(4) == 1 );
}
// -----------------------------------------------------------------------------
