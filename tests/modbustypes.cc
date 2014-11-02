#include <catch.hpp>

#include "modbus/ModbusTypes.h"
using namespace std;

TEST_CASE("Modbus Types", "[modbus][modbus types]" ) 
{
	SECTION("WriteOutputMessage: limit the amount of data verification")
	{
		ModbusRTU::WriteOutputMessage msg(0x01,1);
		for( int i=0; i<(ModbusRTU::MAXDATALEN-1); i++ )
			msg.addData(10+i);

		CHECK_FALSE( msg.isFull() );
		msg.addData(1);
		CHECK( msg.isFull() );
	}

	WARN("Tests for 'Modbus types' incomplete..");
}
