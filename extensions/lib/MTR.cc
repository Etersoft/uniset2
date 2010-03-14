// --------------------------------------------------------------------------
//!  \version $Id: MTR.cc,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// --------------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <algorithm>
#include "modbus/ModbusRTUMaster.h"
#include "modbus/ModbusHelpers.h"
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
DataMap read_confile( const std::string f )
{
	bool start = false;
	DataMap dmap;
	std::ifstream ifs(f.c_str(),std::ios::in);
	if( ifs )
	{
		while( !ifs.eof() )
		{
			std::string str;
			if( getline(ifs,str) )
			{
				if( str.empty() )
					continue;

				if( !start )
				{
					str = str.substr(0,str.size()-1); // remove \n\r
//					cout <<  "check str: " << str << endl;
					if( str == "[Settings]" )
					{
						start = true;
						continue;
					}
					else
						continue;
				}

				string s_reg, s_data;
				if( read_param(str,s_reg,s_data) )
				{
					ModbusRTU::ModbusData reg = ModbusRTU::str2mbData(s_reg);

					/* we can write only registers > 40000 (see MTR-2 manual) */
					if( reg <= 40000 )
						continue;
					reg -= 40000;

//					cout << "reg=" << s_reg
//						 << " data=" << s_data << endl;
					DataList dlst;
					int k=0;
					std::vector<unsigned char> v(4);
					for( int i=0; i<s_data.size(); i++ )
					{
						v[k++] = s_data[i];
						if( k>3 )
						{
							k=0;
							string tmp(v.begin(),v.end());
							tmp = "0x" + tmp;
//							cout << "find data=" << ModbusRTU::str2mbData(tmp) 
//									<< "(" << tmp << ")" << endl;
							dlst.push_back( ModbusRTU::str2mbData(tmp) );
						}
					}
					
					dmap[reg] = dlst;
				}
			}
		}
	}
	
	ifs.close();
	
	return dmap;
}
// --------------------------------------------------------------------------
bool read_param( const std::string str, std::string& str1, std::string& str2 )
{
	string::size_type pos = str.find('=');
	if( pos==string::npos )
		return false;

	str1 = str.substr(0,pos);
	str2 = str.substr(pos+1,str.size());
	return true;
}
// ------------------------------------------------------------------------------------------
ComPort::Speed get_speed( ModbusRTU::ModbusData data )
{
	static const ComPort::Speed speed_conv[] = { ComPort::ComSpeed1200,
		ComPort::ComSpeed2400, ComPort::ComSpeed4800, ComPort::ComSpeed9600,
		ComPort::ComSpeed19200, ComPort::ComSpeed38400, ComPort::ComSpeed57600,
		ComPort::ComSpeed115200 };

	if( data >= sizeof(speed_conv)/sizeof(speed_conv[0]) )
		return ComPort::ComSpeed0;
	return speed_conv[data];
}
// ------------------------------------------------------------------------------------------
ComPort::Parity get_parity( ModbusRTU::ModbusData data )
{
	static const ComPort::Parity parity_conv[] = {
		ComPort::NoParity, ComPort::Odd, ComPort::Even };

	if( data >= sizeof(parity_conv)/sizeof(parity_conv[0]) )
		return ComPort::NoParity;
	return parity_conv[data];
}
// ------------------------------------------------------------------------------------------
void update_communication_params( ModbusRTU::ModbusAddr reg, ModbusRTU::ModbusData data,
				  ModbusRTUMaster* mb, ModbusRTU::ModbusAddr& addr, int verb )
{
	if( reg == 55 )
	{
		addr = data;
		if( verb )
			cout << "(mtr-setup): slaveaddr is set to "
				 << ModbusRTU::addr2str(addr) << endl;
	}
	else if( reg == 56 )
	{
		ComPort::Speed speed = get_speed(data);
		if( speed != ComPort::ComSpeed0 )
		{
			mb->setSpeed(speed);
			if( verb )
				cout << "(mtr-setup): speed is set to "
					 << ComPort::getSpeed(speed) << endl;
		}
	}
	else if( reg == 57 )
	{
		if( data == 0 )
			mb->setStopBits(ComPort::OneBit);
		else if( data == 1)
			mb->setStopBits(ComPort::TwoBits);
		else return;
		if( verb )
			cout << "(mtr-setup): number of stop bits is set to "
				 << data + 1 << endl;
	}
	else if( reg == 58 )
	{
		if (data != 0 && data != 1 && data != 2)
			return;
		mb->setParity(get_parity(data));
		if( verb )
			cout << "(mtr-setup): parity is set to "
				 << (data ? ((data == 1) ? "odd" : "even") : "no") << endl;
	}
}
// ------------------------------------------------------------------------------------------
bool send_param( ModbusRTUMaster* mb, DataMap& dmap, ModbusRTU::ModbusAddr addr, int verb )
{
	if( !mb )
	{
		cerr << "(MTR::send_param): mb=NULL!" << endl;
		return false;
	}

	for( DataMap::iterator it=dmap.begin(); it!=dmap.end(); ++it )
	{
//		ModbusRTU::WriteOutputMessage msg(addr,it->first);
//		cout << "send reg=" << ModbusRTU::dat2str(it->first) 
//							<< "(" << it->first << ")" << endl;
		int reg = it->first;
		bool ok = false;

		for( DataList::iterator it1=it->second.begin(); it1!=it->second.end(); ++it1, reg++ )
		{
			const ModbusRTU::ModbusData *last = skip + sizeof(skip)/sizeof(skip[0]);
			if( std::find(skip, last, reg) != last)
				continue;

			cout << "send reg=" << ModbusRTU::dat2str(reg) 
							<< "(" << reg << ")" 
				<< "=" << ModbusRTU::dat2str( (*it1) )  << endl;
//			ok=true;
//			continue;
			for( int i=0; i<attempts; i++ )
			{
				try
				{
					ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06(addr,reg,(*it1));
					if( verb )
						cout << "(mtr-setup): write reply: " << ret << endl;
					update_communication_params(reg, *it1, mb, addr, verb);
					ok = true;
					break;
				}
				catch( ModbusRTU::mbException& ex )
				{
					/* if speed is changed we receive a timeout error */
					if( reg == 56 && it->first == ModbusRTU::erTimeOut )
					{
						update_communication_params(reg, *it1, mb, addr, verb);
						ok = true;
						break;
					}
					else
					{
						cerr << "(mtr-setup): error for write reg="
							 << ModbusRTU::dat2str(reg)
							 << "(" << it->first << "): " << ex << endl;
					}
				}
			}
		}

//		if( !ok )
//			return false;
	}
	
	return true;
}
// ------------------------------------------------------------------------------------------
bool update_configuration( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr slaveaddr,
						    const std::string mtrconfile,  int verb )
{
	std::string m = MTR::getModelNumber(mb, slaveaddr);
	if( m != "MTR315Transducer" )
	{
		cerr << "(mtr-setup): model number != 'MTR315Transducer' (read: " << m << ")" << endl;
		return false;
	}

	DataMap dmap = MTR::read_confile(mtrconfile);
	if( dmap.empty() )
	{
		cerr << "(mtr-setup): error read confile=" << mtrconfile << endl;
		return false;
	}

	if( send_param(mb,dmap,slaveaddr,verb) )
		return true;

	return false;
}
// ------------------------------------------------------------------------------------------
} // end of namespace MTR
// -----------------------------------------------------------------------------
