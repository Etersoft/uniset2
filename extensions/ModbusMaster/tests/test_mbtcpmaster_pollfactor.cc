#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <memory>
#include <unordered_set>
#include <limits>
#include <Poco/Net/NetException.h>
#include "UniSetTypes.h"
#include "MBTCPTestServer.h"
#include "MBTCPMaster.h"
#include "UniSetActivator.h"
#include "VTypes.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveADDR = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20048; // conf->getArgInt("--mbs-inet-port");
static string iaddr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static unordered_set<ModbusRTU::ModbusAddr> vaddr = { slaveADDR, 0x02, 0x03 };
static shared_ptr<MBTCPTestServer> mbs;
static shared_ptr<UInterface> ui;
static std::shared_ptr<SMInterface> smi;
static ObjectId mbID = 6004; // MBTCPMaster1
static int polltime = 100; // conf->getArgInt("--mbtcp-polltime");
static ObjectId slaveNotRespond = 10; // Slave_Not_Respond_S
// -----------------------------------------------------------------------------
extern std::shared_ptr<SharedMemory> shm;
extern std::shared_ptr<MBTCPMaster> mbm;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(slaveNotRespond, 8000) );
    }

    if( !smi )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            throw SystemError("UInterface don`t initialize..");

        smi = make_shared<SMInterface>(shm->getId(), ui, mbID, shm );
    }

    if( !mbs )
    {
        try
        {
            mbs = make_shared<MBTCPTestServer>(vaddr, iaddr, port, false);
        }
        catch( const Poco::Net::NetException& e )
        {
            ostringstream err;
            err << "(mbs): Can`t create socket " << iaddr << ":" << port << " err: " << e.message() << endl;
            cerr << err.str() << endl;
            throw SystemError(err.str());
        }
        catch( const std::exception& ex )
        {
            cerr << "(mbs): Can`t create socket " << iaddr << ":" << port << " err: " << ex.what() << endl;
            throw;
        }

        //      mbs->setVerbose(true);
        CHECK( mbs != nullptr );
        mbs->execute();

        for( int i = 0; !mbs->isRunning() && i < 10; i++ )
            msleep(200);

        CHECK( mbs->isRunning() );
        msleep(2000);
        CHECK( ui->getValue(slaveNotRespond) == 0 );
    }

    REQUIRE( mbm != nullptr );
}
// -----------------------------------------------------------------------------
static bool exchangeIsOk()
{
    PassiveTimer pt(5000);

    while( !pt.checkTime() && ui->getValue(slaveNotRespond) )
        msleep(300);

    return !pt.checkTime();
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x05", "[modbus][0x05][mbmaster][mbtcpmaster][pollfactor]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    msleep(polltime * 2); //ждем несколько циклов обмена
    ui->setValue(13, 0);
    REQUIRE( ui->getValue(13) == 0 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(260) == 0 );
    msleep(polltime * 5);
    CHECK( mbs->getWriteRegisterCount(260) == 1 );

    ui->setValue(13, 1);
    REQUIRE( ui->getValue(13) == 1 );
    msleep(polltime + 200);
    CHECK( mbs->getLastWriteRegister(260) == 1 );
    msleep(polltime * 5);
    CHECK( mbs->getWriteRegisterCount(260) == 2 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x06/0x10", "[modbus][0x06/0x10][mbmaster][mbtcpmaster][pollfactor]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );

    std::vector<int> regs = {297, 298, 300, 302, 304, 306, 310, 311, 312, 314, 316, 318, 320};
    int i = 14;

    for(auto&& it : regs)
    {
        ui->setValue(i, 0);
        REQUIRE( ui->getValue(i) == 0 );
        msleep(polltime + 200);
        REQUIRE( mbs->getLastWriteRegister(it) == 0 );
        msleep(polltime * 5);
        REQUIRE( mbs->getWriteRegisterCount(it) == 1 );
    }

    ui->setValue(14, 77);
    REQUIRE( ui->getValue(14) == 77 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(297) == 77 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(297) == 2 );

    ui->setValue(15, 1);
    REQUIRE( ui->getValue(15) == 1 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(298) == 2 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(298) == 2 );

    ui->setValue(16, 1);
    REQUIRE( ui->getValue(16) == 1 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(298) == 6 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(298) == 3 );

    ui->setValue(17, 88);
    REQUIRE( ui->getValue(17) == 88 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(300) == 88 );//LSB(88) = 88
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(300) == 2 );

    ui->setValue(18, 102);
    REQUIRE( ui->getValue(18) == 102 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterByte(301) == 26112 );//MSB(102) = 26112
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(301) == 2 );

    ui->setValue(19, 4132);
    REQUIRE( ui->getValue(19) == 4132 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterF2(302) == (float)41.32 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(302) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(303) == 2 );

    ui->setValue(20, 5670);//0x237
    REQUIRE( ui->getValue(20) == 5670 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterF2r(304) == (float)56.7 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(304) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(305) == 2 );

    ui->setValue(21, 99);
    REQUIRE( ui->getValue(21) == 99 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterF4(306) == (float)99 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(306) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(307) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(308) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(309) == 2 );

    ui->setValue(22, 17);
    REQUIRE( ui->getValue(22) == 17 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(310) == 17 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(310) == 2 );

    ui->setValue(23, -67);
    REQUIRE( ui->getValue(23) == -67 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(311) == -67 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(311) == 2 );

    ui->setValue(24, 82);
    REQUIRE( ui->getValue(24) == 82 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterI2(312) == 82 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(312) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(313) == 2 );

    ui->setValue(25, 33);
    REQUIRE( ui->getValue(25) == 33 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterI2r(314) == 33 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(314) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(315) == 2 );

    ui->setValue(26, 26);
    REQUIRE( ui->getValue(26) == 26 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterU2(316) == 26 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(316) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(317) == 2 );

    ui->setValue(27, 55);
    REQUIRE( ui->getValue(27) == 55 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegisterU2r(318) == 55 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(318) == 2 );
    REQUIRE( mbs->getWriteRegisterCount(319) == 2 );

    ui->setValue(28, 2);
    REQUIRE( ui->getValue(28) == 2 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(320) == 2 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(320) == 2 );

    ui->setValue(29, 1);
    REQUIRE( ui->getValue(29) == 1 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(320) == 6 );
    msleep(polltime * 5);
    REQUIRE( mbs->getWriteRegisterCount(320) == 3 );
}
// -----------------------------------------------------------------------------
