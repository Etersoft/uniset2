#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include "MBSlave.h"
#include "UniSetTypes.h"
#include "modbus/ModbusTCPMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TEST_CASE("Modbus Slave","[modbus]")
{
	CHECK( conf!=0 );

	ModbusRTU::ModbusAddr slaveaddr = 0x01; // conf->getArgInt("--mbs-my-addr");
	int port = 20048; // conf->getArgInt("--mbs-inet-port");
	string addr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
	ObjectId slaveID = 6004; // conf->getObjectID( conf->getArgParam("--mbs-name"));

	// UI понадобиться для проверки записанных в SM значений.
	UInterface ui;
	CHECK( ui.getObjectIndex() != 0 );
	CHECK( ui.getConf() == UniSetTypes::conf );
	CHECK( ui.waitReady(slaveID,5000) );

	ModbusTCPMaster mb;
    ost::InetAddress ia(addr.c_str()); 
    mb.setTimeout(2000);
    REQUIRE_NOTHROW( mb.connect(ia,port) );

#if 0
	SECTION("(0x01): read coil status")
	{
		// read 1 bit
		{
	 		ModbusRTU::ReadCoilRetMessage ret(slaveaddr);
			REQUIRE_NOTHROW( ret = mb.read01(slaveaddr,1000,1) );
			ModbusRTU::DataBits b(ret.data[0]);
			REQUIRE( b[0] == 1 );
		}
		// read 3 bit
		{
	 		ModbusRTU::ReadCoilRetMessage ret(slaveaddr);
			REQUIRE_NOTHROW( ret = mb.read01(slaveaddr,1000,3) );
			ModbusRTU::DataBits b(ret.data[0]);
			REQUIRE( b[0] == 1 );
			REQUIRE( b[1] == 1 );
			REQUIRE( b[2] == 0 );
		}
	}
#endif
#if 0
	SECTION("(0x02): read input status")
	{
		// read 1 bit
		{
	 		ModbusRTU::ReadInputStatusRetMessage ret(slaveaddr);
			REQUIRE_NOTHROW( ret = mb.read02(slaveaddr,1000,1) );
			ModbusRTU::DataBits b(ret.data[0]);
			REQUIRE( b[0] == 1 );
		}
		// read 3 bit
		{
	 		ModbusRTU::ReadInputStatusRetMessage ret(slaveaddr);
			REQUIRE_NOTHROW( ret = mb.read02(slaveaddr,1000,3) );
			ModbusRTU::DataBits b(ret.data[0]);
			REQUIRE( b[0] == 1 );
			REQUIRE( b[1] == 1 );
			REQUIRE( b[2] == 0 );
		}
	}
#endif
	SECTION("Function (0x03): 'read register outputs or memories or read word outputs or memories'")
	{
		SECTION("Test: read one reg..")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb.read03(slaveaddr,10,1);
			REQUIRE( ret.data[0] == 10 );
		}
		SECTION("Test: read many registers..")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb.read03(slaveaddr,10,3);
			REQUIRE( ret.data[0] == 10 );
			REQUIRE( ret.data[1] == 11 );
			REQUIRE( (signed short)(ret.data[2]) == -10 );
		}
		SECTION("Test: read MAXDATA count..")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb.read03(slaveaddr,10,ModbusRTU::MAXDATALEN);
			REQUIRE( ret.count == ModbusRTU::MAXDATALEN );
		}
		SECTION("Test: read TOO many registers")
		{
			try
			{
				mb.read03(slaveaddr,-23,1200);
			}
			catch( ModbusRTU::mbException& ex )
			{
				REQUIRE( ex.err == ModbusRTU::erTimeOut );
			}
		}
		SECTION("Test: read unknown registers")
		{
			try
			{
				mb.read03(slaveaddr,-23,1);
			}
			catch( ModbusRTU::mbException& ex )
			{
				REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
			}
		}
		SECTION("Test: incorrect number")
		{
			try
			{
				mb.read03(slaveaddr,10,-3);
			}
			catch( ModbusRTU::mbException& ex )
			{
				REQUIRE( ex.err == ModbusRTU::erTimeOut );
			}
		}
	}

	SECTION("Function (0x04): 'read input registers or memories or read word outputs or memories'")
	{
		SECTION("Test: read one reg..")
		{
			ModbusRTU::ReadInputRetMessage ret = mb.read04(slaveaddr,10,1);
			REQUIRE( ret.data[0] == 10 );
		}
		SECTION("Test: read one reg..")
		{
			ModbusRTU::ReadInputRetMessage ret = mb.read04(slaveaddr,10,1);
			REQUIRE( ret.data[0] == 10 );
		}
		SECTION("Test: read many registers..")
		{
			ModbusRTU::ReadInputRetMessage ret = mb.read04(slaveaddr,10,4);
			REQUIRE( ret.data[0] == 10 );
			REQUIRE( ret.data[1] == 11 );
			REQUIRE( (signed short)(ret.data[2]) == -10 );
			REQUIRE( (signed short)(ret.data[3]) == -10000 );
		}
		SECTION("Test: read MAXDATA count..")
		{
			ModbusRTU::ReadInputRetMessage ret = mb.read04(slaveaddr,10,ModbusRTU::MAXDATALEN);
			REQUIRE( ret.count == ModbusRTU::MAXDATALEN );
		}
		SECTION("Test: read TOO many registers")
		{
			try
			{
				mb.read04(slaveaddr,-23,1200);
			}
			catch( ModbusRTU::mbException& ex )
			{
				REQUIRE( ex.err == ModbusRTU::erTimeOut );
			}
		}
		SECTION("Test: read unknown registers")
		{
			try
			{
				mb.read04(slaveaddr,-23,1);
			}
			catch( ModbusRTU::mbException& ex )
			{
				REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
			}
		}
		SECTION("Test: incorrect number")
		{
			try
			{
				mb.read04(slaveaddr,10,-3);
			}
			catch( ModbusRTU::mbException& ex )
			{
				REQUIRE( ex.err == ModbusRTU::erTimeOut );
			}
		}
	}
	SECTION("(0x05): forces a single coil to either ON or OFF")
	{
		ObjectId tID = 1007;
		SECTION("Test: ON")
		{
			ModbusRTU::ForceSingleCoilRetMessage  ret = mb.write05(slaveaddr,14,true);
			CHECK( ret.start == 14 );
			CHECK( ret.cmd() == true );
			CHECK( ui.getValue(tID) == 1 );
		}
		SECTION("Test: OFF")
		{
			ModbusRTU::ForceSingleCoilRetMessage  ret = mb.write05(slaveaddr,14,false);
			CHECK( ret.start == 14 );
			CHECK( ret.cmd() == false );
			CHECK( ui.getValue(tID) == 0 );
		}
	}
#if 0
	SECTION("(0x06): write register outputs or memories")
	{

	}
#endif
#if 0
	SECTION("(0x08): Diagnostics (Serial Line only)")
	{

	}
#endif
#if 0
	SECTION("(0x0F): force multiple coils")
	{

	}
	SECTION("(0x10): write register outputs or memories")
	{

	}
#endif
#if 0
	SECTION("(0x10): write register outputs or memories")
	{
        fnReadFileRecord        = 0x14,    /*!< read file record */
	}
	SECTION("(0x10): write register outputs or memories")
	{
   		fnWriteFileRecord        = 0x15,    /*!< write file record */
	}
	SECTION("(0x10): write register outputs or memories")
	{
        fnMEI                    = 0x2B, /*!< Modbus Encapsulated Interface */
	}
	SECTION("(0x10): write register outputs or memories")
	{
        fnSetDateTime            = 0x50, /*!< set date and time */
	}
	SECTION("(0x10): write register outputs or memories")
	{
        fnRemoteService            = 0x53,    /*!< call remote service */
	}
	SECTION("(0x10): write register outputs or memories")
	{
        fnJournalCommand        = 0x65,    /*!< read,write,delete alarm journal */
	}
	SECTION("(0x10): write register outputs or memories")
	{
        fnFileTransfer            = 0x66    /*!< file transfer */
	}
#endif
}
