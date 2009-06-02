// --------------------------------------------------------------------------
//!  \version $Id: MTR.cc,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// --------------------------------------------------------------------------
#include <cmath>
#include "modbus/ModbusRTUMaster.h"
#include "MTR.h"
// --------------------------------------------------------------------------
using namespace std;
//using namespace MTR;
// --------------------------------------------------------------------------
namespace MTR
{

MTRType str2type( const std::string s )
{
	if( s == "T1" )
		return mtT1;
	if( s == "T2" )
		return mtT2;
	if( s == "T3" )
		return mtT3;
	if( s == "T4" )
		return mtT4;
	if( s == "T5" )
		return mtT5;
	if( s == "T6" )
		return mtT6;
	if( s == "T7" )
		return mtT7;
	if( s == "T8" )
		return mtT8;
	if( s == "T9" )
		return mtT9;
	if( s == "F1" )
		return mtF1;
	if( s == "T_Str16" )
		return mtT_Str16;
	if( s == "T_Str8" )
		return mtT_Str8;

	return mtUnknown;
}
// -------------------------------------------------------------------------
string type2str( MTRType t )
{
	if( t == mtT1 )
		return "T1";
	if( t == mtT2 )
		return "T2";
	if( t == mtT3 )
		return "T3";
	if( t == mtT4 )
		return "T4";
	if( t == mtT5 )
		return "T5";
	if( t == mtT6 )
		return "T6";
	if( t == mtT7 )
		return "T7";
	if( t == mtT8 )
		return "T8";
	if( t == mtT9 )
		return "T9";
	if( t == mtF1 )
		return "F1";
	if( t == mtT_Str16 )
		return "T_Str16";
	if( t == mtT_Str8 )
		return "T_Str8";

	return "Unknown";
}
// -------------------------------------------------------------------------
int wsize(  MTRType t )
{
	if( t == mtT1 )
		return T1::wsize();
	if( t == mtT2 )
		return T2::wsize();
	if( t == mtT3 )
		return T3::wsize();
	if( t == mtT4 )
		return T4::wsize();
	if( t == mtT5 )
		return T5::wsize();
	if( t == mtT6 )
		return T6::wsize();
	if( t == mtT7 )
		return T7::wsize();
	if( t == mtT8 )
		return T8::wsize();
	if( t == mtT9 )
		return T9::wsize();
	if( t == mtF1 )
		return F1::wsize();
	if( t == mtT_Str16 )
		return T_Str16::wsize();
	if( t == mtT_Str8 )
		return T_Str8::wsize();

	return 0;
}
// -------------------------------------------------------------------------
bool setAddress( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusAddr newAddr )
{
	try
	{
		ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06( addr, regAddress, newAddr );
		if( ret.start == regAddress && ret.data == newAddr )
			return true;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return false;
}
// -----------------------------------------------------------------------------
bool setBaudRate( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrBaudRate br )
{
	try
	{
		ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06( addr, regBaudRate, br );
		if( ret.start == regBaudRate && ret.data == br )
			return true;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return false;
}
// -----------------------------------------------------------------------------	
bool setStopBit( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, bool state )
{
	try
	{
		ModbusRTU::ModbusData dat = state ? 1 : 0;
		ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06( addr, regStopBit, dat );
		if( ret.start == regStopBit && ret.data == dat )
			return true;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return false;
}
// -----------------------------------------------------------------------------
bool setParity( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrParity p )
{
	try
	{
		ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06( addr, regParity, p );
		if( ret.start == regParity && ret.data == p )
			return true;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return false;
}
// -----------------------------------------------------------------------------
bool setDataBits( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrDataBits d )
{
	try
	{
		ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06( addr, regDataBits, d );
		if( ret.start == regDataBits && ret.data == d )
			return true;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return false;
}
// -----------------------------------------------------------------------------
std::string getModelNumber( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr )
{
	try
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04( addr, regModelNumber, T_Str16::wsize() );
		T_Str16 t(ret);
		return t.sval;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return "";
}
// -----------------------------------------------------------------------------	
std::string getSerialNumber( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr )
{
	try
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04( addr, regSerialNumber, T_Str8::wsize() );
		T_Str8 t(ret);
		return t.sval;
	}
	catch(  ModbusRTU::mbException& ex )
	{
	}
	
	return "";
}
// -----------------------------------------------------------------------------
} // end of namespace MTR
// -----------------------------------------------------------------------------
