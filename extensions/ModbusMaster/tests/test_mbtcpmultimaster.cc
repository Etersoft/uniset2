#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include <unordered_set>
#include "UniSetTypes.h"
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <memory>
#include <limits>
#include "UniSetTypes.h"
#include "MBTCPTestServer.h"
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveaddr = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20053; // conf->getArgInt("--mbs-inet-port");
static string iaddr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static int port2 = 20055;
static string iaddr2("127.0.0.1");
static unordered_set<ModbusRTU::ModbusAddr> slaveADDR = { 0x01 };
static shared_ptr<MBTCPTestServer> mbs1;
static shared_ptr<MBTCPTestServer> mbs2;
static shared_ptr<UInterface> ui;
static ObjectId mbID = 6005; // MBTCPMultiMaster1
static int polltime = 100; // conf->getArgInt("--mbtcp-polltime");
static ObjectId slaveNotRespond = 10; // Slave_Not_Respond_S
static ObjectId slave1NotRespond = 12; // Slave1_Not_Respond_S
static ObjectId slave2NotRespond = 13; // Slave2_Not_Respond_S
static const ObjectId exchangeMode = 11; // MBTCPMaster_Mode_AS
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

	if( !mbs1 )
	{
		try
		{
			ost::Thread::setException(ost::Thread::throwException);
			mbs1 = make_shared<MBTCPTestServer>(slaveADDR, iaddr, port, false);
		}
		catch( const ost::SockException& e )
		{
			ostringstream err;
			err << "(mb1): Can`t create socket " << iaddr << ":" << port << " err: " << e.getString() << endl;
			cerr << err.str() << endl;
			throw SystemError(err.str());
		}
		catch( const std::exception& ex )
		{
			cerr << "(mb1): Can`t create socket " << iaddr << ":" << port << " err: " << ex.what() << endl;
			throw;
		}

		CHECK( mbs1 != nullptr );
		mbs1->setReply(0);
		mbs1->execute();

		for( int i = 0; !mbs1->isRunning() && i < 10; i++ )
			msleep(200);

		CHECK( mbs1->isRunning() );
		msleep(7000);
		CHECK( ui->getValue(slaveNotRespond) == 0 );
	}

	if( !mbs2 )
	{
		try
		{
			ost::Thread::setException(ost::Thread::throwException);
			mbs2 = make_shared<MBTCPTestServer>(slaveADDR, iaddr2, port2, false);
		}
		catch( const ost::SockException& e )
		{
			ostringstream err;
			err << "(mb2): Can`t create socket " << iaddr << ":" << port << " err: " << e.getString() << endl;
			cerr << err.str() << endl;
			throw SystemError(err.str());
		}
		catch( const std::exception& ex )
		{
			cerr << "(mb2): Can`t create socket " << iaddr << ":" << port << " err: " << ex.what() << endl;
			throw;
		}

		CHECK( mbs2 != nullptr );
		mbs2->setReply(0);
		mbs2->execute();

		for( int i = 0; !mbs2->isRunning() && i < 10; i++ )
			msleep(200);

		CHECK( mbs2->isRunning() );
	}

}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: rotate channel", "[modbus][mbmaster][mbtcpmultimaster]")
{
	// Т.к. respond/notrespond проверяется по возможности создать соединение
	// а мы имитируем отключение просто отключением обмена
	// то датчик связи всё-равно будет показывать что канал1 доступен
	// поэтому датчик 12 - не проверяем..
	// а просто проверяем что теперь значение приходит по другому каналу
	// (см. setReply)
	// ----------------------------

	InitTest();
	CHECK( ui->isExist(mbID) );

	REQUIRE( ui->getValue(1003) == 0 );
	mbs1->setReply(100);
	mbs2->setReply(10);
	msleep(polltime + 1000);
	REQUIRE( ui->getValue(1003) == 100 );
	mbs1->disableExchange(true);
	mbs2->disableExchange(false);
	msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
	REQUIRE( ui->getValue(1003) == 10 );

	// проверяем что канал остался на втором, хотя на первом мы включили связь
	mbs1->disableExchange(false);
	mbs2->disableExchange(false);
	msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
	REQUIRE( ui->getValue(1003) == 10 );

	mbs2->disableExchange(true);
	mbs1->disableExchange(false);
	msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
	REQUIRE( ui->getValue(1003) == 100 );
}
// -----------------------------------------------------------------------------
