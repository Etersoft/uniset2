#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include "UniSetTypes.h"
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveaddr = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20048; // conf->getArgInt("--mbs-inet-port");
static string addr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static ObjectId slaveID = 6004; // conf->getObjectID( conf->getArgParam("--mbs-name"));
static ModbusTCPMaster* mb = nullptr;
static UInterface* ui = nullptr;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf!=nullptr );

    if( ui == nullptr )
    {
        ui = new UInterface();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(slaveID,5000) );
    }
}
// -----------------------------------------------------------------------------

TEST_CASE("MBTCPMultiMaster","[modbus][mbmaster][mbtcpmultimaster]")
{
    FAIL("Tests for MBTCPMaster not yet");
}
