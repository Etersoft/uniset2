#include <catch.hpp>

#include <limits>
#include "VTypes.h"

using namespace std;
using namespace UniSetTypes;
using namespace VTypes;

TEST_CASE("VTypes: I2","[vtypes][I2]")
{
	SECTION("Default constructor")
	{
		I2 v;
		REQUIRE( (int)v == 0 );
	}
	SECTION("'int' constructor")
	{
		I2 v(100);
		REQUIRE( (int)v == 100 );

		I2 v2(-1000000);
		REQUIRE( (int)v2 == -1000000 );
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData data[2] = {0,0xFFFF};
		I2 v1(data,2);
		REQUIRE( (int)v1 == -65536 );

		ModbusRTU::ModbusData data3[3] = {0,0xFFFF,0xFFFF};
		I2 v2(data3,3);
		REQUIRE( (int)v2 == -65536 );
	}
}

TEST_CASE("VTypes: I2r","[vtypes][I2r]")
{
	SECTION("Default constructor")
	{
		I2r v;
		REQUIRE( (int)v == 0 );
	}
	SECTION("'int' constructor")
	{
		I2r v(100);
		REQUIRE( (int)v == 100 );

		I2r v1(-1000000);
		REQUIRE( (int)v1 == -1000000 );
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData data[2] = {0,0xFFFF};
		I2r v1(data,2);
		REQUIRE( (int)v1 == 65535 );

		ModbusRTU::ModbusData data3[3] = {0,0xFFFF,0xFFFF};
		I2r v2(data3,3);
		REQUIRE( (int)v2 == 65535 );

		I2r tmp(-100000);
		ModbusRTU::ModbusData d2[2];
		d2[0] = tmp.raw.v[1];
		d2[1] = tmp.raw.v[0];
		I2r v3(d2,2);
		REQUIRE( (int)v3 == -100000 );

	}
}

TEST_CASE("VTypes: U2","[vtypes][U2]")
{
	SECTION("Default constructor")
	{
		U2 v;
		REQUIRE( (unsigned int)v == 0 );
	}
	SECTION("'unsigned int' constructor")
	{
		{
			U2 v( numeric_limits<unsigned int>::max() );
			REQUIRE( (unsigned int)v == numeric_limits<unsigned int>::max() );
			REQUIRE( v.raw.v[0] == 0xffff );
			REQUIRE( v.raw.v[1] == 0xffff );
		}

		{
			U2 v(-1);
			REQUIRE( (unsigned int)v == 4294967295 );
		}
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData data[2] = {0,0xFFFF};
		U2 v1(data,2);
		REQUIRE( (unsigned int)v1 == 0xffff0000 );

		ModbusRTU::ModbusData data3[3] = {0,0xFFFF,0xFFFF};
		U2 v2(data3,3);
		REQUIRE( (unsigned int)v2 == 0xffff0000 );
	}
}

TEST_CASE("VTypes: U2r","[vtypes][U2r]")
{
	SECTION("Default constructor")
	{
		U2r v;
		REQUIRE( (unsigned int)v == 0 );
	}
	SECTION("'unsigned int' constructor")
	{
		U2r v( numeric_limits<unsigned int>::max() );
		REQUIRE( (unsigned int)v == numeric_limits<unsigned int>::max() );
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData data[2] = {0,0xFFFF};
		U2r v1(data,2);
		REQUIRE( (unsigned int)v1 == 0x0000ffff );

		ModbusRTU::ModbusData data3[3] = {0,0xFFFF,0xFFFF};
		U2r v2(data3,3);
		REQUIRE( (unsigned int)v2 == 0x0000ffff );
	}
}

TEST_CASE("VTypes: F2","[vtypes][F2]")
{
	SECTION("Default constructor")
	{
		F2 v;
		REQUIRE( (float)v == 0 );
	}
	SECTION("'float' constructor")
	{
		F2 v( numeric_limits<float>::max() );
		REQUIRE( (float)v == numeric_limits<float>::max() );
	}
	SECTION("Modbus constructor")
	{
		float f=1.5;
		ModbusRTU::ModbusData data[2];
		memcpy(data,&f,sizeof(data));
		F2 v1(data,2);
		REQUIRE( (float)v1 == f );
		REQUIRE( (long)v1 == 2 );
		REQUIRE( (int)v1 == 2 );

		ModbusRTU::ModbusData data3[3];
		memset(data3,0,sizeof(data3));
		memcpy(data3,&f,2*sizeof(ModbusRTU::ModbusData));
		F2 v2(data3,3);
		REQUIRE( (float)v2 == f );
	}
}

TEST_CASE("VTypes: F2r","[vtypes][F2r]")
{
	SECTION("Default constructor")
	{
		F2r v;
		REQUIRE( (float)v == 0 );
	}
	SECTION("'float' constructor")
	{
		F2r v( numeric_limits<float>::max() );
		REQUIRE( (float)v == numeric_limits<float>::max() );
	}
	SECTION("Modbus constructor")
	{
		float f=1.5;
		ModbusRTU::ModbusData data[2];
		memcpy(data,&f,sizeof(data));
		std::swap(data[0],data[1]);
		F2r v1(data,2);
		REQUIRE( (float)v1 == f );
		REQUIRE( (long)v1 == 2 );
		REQUIRE( (int)v1 == 2 );

		ModbusRTU::ModbusData data3[3];
		memset(data3,0,sizeof(data3));
		memcpy(data3,&f,2*sizeof(ModbusRTU::ModbusData));
		std::swap(data3[0],data3[1]);
		F2r v2(data3,3);
		REQUIRE( (float)v2 == f );
	}
}

TEST_CASE("VTypes: F4","[vtypes][F4]")
{
	SECTION("Default constructor")
	{
		F4 v;
		REQUIRE( (float)v == 0 );
	}
	SECTION("'float' constructor")
	{
		{
			F4 v( numeric_limits<float>::max() );
			REQUIRE( (float)v == numeric_limits<float>::max() );
		}
		{
	
			F4 v( numeric_limits<float>::min() );
			REQUIRE( (float)v == numeric_limits<float>::min() );
		}
	}
	SECTION("Modbus constructor")
	{
		{
			float f = numeric_limits<float>::max();
			ModbusRTU::ModbusData data[4];
			memcpy(data,&f,sizeof(data));
			F4 v1(data,4);
			REQUIRE( (float)v1 == f );
		}
		{
			float f = numeric_limits<float>::max();
			ModbusRTU::ModbusData data5[5];
			memset(data5,0,sizeof(data5));
			memcpy(data5,&f,4*sizeof(ModbusRTU::ModbusData));
			F4 v2(data5,5);
			REQUIRE( (float)v2 == f );
		}
		{
			float f = numeric_limits<float>::min();
			ModbusRTU::ModbusData data[4];
			memcpy(data,&f,sizeof(data));
			F4 v1(data,4);
			REQUIRE( (float)v1 == f );
		}
	}
}

TEST_CASE("VTypes: Byte","[vtypes][byte]")
{
	SECTION("Default constructor")
	{
		Byte v;
		REQUIRE( (char)v == 0 );
	}
	SECTION("'2 byte' constructor")
	{
		Byte v(132,255);
		REQUIRE( v[0] == 132 );
		REQUIRE( v[1] == 255 );
	}
	SECTION("'long' constructor")
	{
		long l=0xff;
		Byte v(l);
		REQUIRE( (unsigned char)v == 0xff );
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData d = 255;
		Byte v(d);
		REQUIRE( (unsigned char)v == 255 );
	}
}

TEST_CASE("VTypes: Unsigned","[vtypes][unsigned]")
{
	SECTION("Default constructor")
	{
		Unsigned v;
		REQUIRE( v == 0 );
	}
	SECTION("'long' constructor")
	{
		long l=0xffff;
		Unsigned v(l);
		REQUIRE( (unsigned short)v == 0xffff );
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData d = 65535;
		Unsigned v(d);
		REQUIRE( (unsigned short)v == 65535 );
	}
}

TEST_CASE("VTypes: Signed","[vtypes][signed]")
{
	SECTION("Default constructor")
	{
		Signed v;
		REQUIRE( v == 0 );
	}
	SECTION("'long' constructor")
	{
		long l= -32766;
		Signed v(l);
		REQUIRE( (signed short)v == l );
	}
	SECTION("Modbus constructor")
	{
		ModbusRTU::ModbusData d = 65535;
		Signed v(d);
		REQUIRE( (signed short)v == (signed short)d );
	}
}
