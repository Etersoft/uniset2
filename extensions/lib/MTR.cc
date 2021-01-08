/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include "modbus/ModbusClient.h"
#include "modbus/ModbusRTUMaster.h"
#include "modbus/ModbusHelpers.h"
#include "MTR.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace uniset
{
    // --------------------------------------------------------------------------
    namespace MTR
    {

        MTRType str2type( const std::string& s )
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

            if( s == "T10" )
                return mtT10;

            if( s == "T16" )
                return mtT16;

            if( s == "T17" )
                return mtT17;

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

            if( t == mtT10 )
                return "T10";

            if( t == mtT16 )
                return "T16";

            if( t == mtT17 )
                return "T17";

            if( t == mtF1 )
                return "F1";

            if( t == mtT_Str16 )
                return "T_Str16";

            if( t == mtT_Str8 )
                return "T_Str8";

            return "Unknown";
        }
        // -------------------------------------------------------------------------
        size_t wsize(  MTRType t )
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

            if( t == mtT10 )
                return T10::wsize();

            if( t == mtT16 )
                return T16::wsize();

            if( t == mtT17 )
                return T17::wsize();

            if( t == mtF1 )
                return F1::wsize();

            if( t == mtT_Str16 )
                return T_Str16::wsize();

            if( t == mtT_Str8 )
                return T_Str8::wsize();

            return 0;
        }
        // -------------------------------------------------------------------------
        bool setAddress( ModbusClient* mb, ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusAddr newAddr )
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
        bool setBaudRate( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, mtrBaudRate br )
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
        bool setStopBit(ModbusClient* mb, ModbusRTU::ModbusAddr addr, bool state )
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
        bool setParity( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, mtrParity p )
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
        bool setDataBits( ModbusClient* mb, ModbusRTU::ModbusAddr addr, mtrDataBits d )
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
        std::string getModelNumber( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr )
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
        std::string getSerialNumber( ModbusClient* mb, ModbusRTU::ModbusAddr addr )
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
        DataMap read_confile( const std::string& f )
        {
            DataMap dmap;
            std::ifstream ifs(f.c_str(), std::ios::in);

            if( ifs )
            {
                bool start = false;

                while(true)
                {
                    std::string str;

                    if( getline(ifs, str) )
                    {
                        if( ifs.eof() )
                            break;

                        if( str.empty() )
                            continue;

                        if( !start )
                        {
                            str = str.substr(0, str.size() - 1); // remove \n\r

                            //                    cout <<  "check str: " << str << endl;
                            if( str == "[Settings]" )
                            {
                                start = true;
                                continue;
                            }
                            else
                                continue;
                        }

                        string s_reg, s_data;

                        if( read_param(str, s_reg, s_data) )
                        {
                            ModbusRTU::ModbusData reg = ModbusRTU::str2mbData(s_reg);

                            /* we can write only registers > 40000 (see MTR-2 manual) */
                            if( reg <= 40000 )
                                continue;

                            reg -= 40000;

                            //                    cout << "reg=" << s_reg
                            //                         << " data=" << s_data << endl;
                            DataList dlst;
                            int k = 0;
                            std::vector<unsigned char> v(4);

                            for( size_t i = 0; i < s_data.size(); i++ )
                            {
                                v[k++] = s_data[i];

                                if( k > 3 )
                                {
                                    k = 0;
                                    string tmp(v.begin(), v.end());
                                    tmp = "0x" + tmp;
                                    //                            cout << "find data=" << ModbusRTU::str2mbData(tmp)
                                    //                                    << "(" << tmp << ")" << endl;
                                    dlst.emplace_back( ModbusRTU::str2mbData(tmp) );
                                }
                            }

                            dmap[reg] = dlst;
                        }
                    }
                };
            }

            ifs.close();

            return dmap;
        }
        // --------------------------------------------------------------------------
        bool read_param( const std::string& str, std::string& str1, std::string& str2 )
        {
            string::size_type pos = str.find('=');

            if( pos == string::npos )
                return false;

            str1 = str.substr(0, pos);
            str2 = str.substr(pos + 1, str.size());
            return true;
        }
        // ------------------------------------------------------------------------------------------
        ComPort::Speed get_speed( ModbusRTU::ModbusData data )
        {
            static const ComPort::Speed speed_conv[] = { ComPort::ComSpeed1200,
                                                         ComPort::ComSpeed2400, ComPort::ComSpeed4800, ComPort::ComSpeed9600,
                                                         ComPort::ComSpeed19200, ComPort::ComSpeed38400, ComPort::ComSpeed57600,
                                                         ComPort::ComSpeed115200
                                                       };

            if( data >= sizeof(speed_conv) / sizeof(speed_conv[0]) )
                return ComPort::ComSpeed0;

            return speed_conv[data];
        }
        // ------------------------------------------------------------------------------------------
        ComPort::Parity get_parity( ModbusRTU::ModbusData data )
        {
            static const ComPort::Parity parity_conv[] =
            {
                ComPort::NoParity, ComPort::Odd, ComPort::Even
            };

            if( data >= sizeof(parity_conv) / sizeof(parity_conv[0]) )
                return ComPort::NoParity;

            return parity_conv[data];
        }
        // ------------------------------------------------------------------------------------------
        void update_communication_params( ModbusRTU::ModbusAddr reg, ModbusRTU::ModbusData data,
                                          ModbusClient* mb, ModbusRTU::ModbusAddr& addr, int verb )
        {
            auto mbrtu = dynamic_cast<ModbusRTUMaster*>(mb);

            if( reg == regAddress )
            {
                addr = data;

                if( verb )
                    cout << "(mtr-setup): slaveaddr is set to "
                         << ModbusRTU::addr2str(addr) << endl;
            }
            else if( reg == regBaudRate )
            {
                ComPort::Speed speed = get_speed(data);

                if( speed != ComPort::ComSpeed0 )
                {
                    if( mbrtu )
                    {
                        mbrtu->setSpeed(speed);

                        if( verb )
                            cout << "(mtr-setup): speed is set to "
                                 << ComPort::getSpeed(speed) << endl;
                    }
                }
            }
            else if( reg == regStopBit )
            {
                if( mbrtu )
                {
                    if( data == 0 )
                        mbrtu->setStopBits(ComPort::OneBit);
                    else if( data == 1)
                        mbrtu->setStopBits(ComPort::TwoBits);
                    else return;

                    if( verb )
                        cout << "(mtr-setup): number of stop bits is set to "
                             << data + 1 << endl;
                }
            }
            else if( reg == regParity )
            {
                if (data != 0 && data != 1 && data != 2)
                    return;

                if( mbrtu )
                {
                    mbrtu->setParity(get_parity(data));

                    if( verb )
                        cout << "(mtr-setup): parity is set to "
                             << (data ? ((data == 1) ? "odd" : "even") : "no") << endl;
                }
            }
        }
        // ------------------------------------------------------------------------------------------
        bool send_param( ModbusClient* mb, DataMap& dmap, ModbusRTU::ModbusAddr addr, int verb )
        {
            if( !mb )
            {
                cerr << "(MTR::send_param): mb=NULL!" << endl;
                return false;
            }

            for( auto it = dmap.begin(); it != dmap.end(); ++it )
            {
                //        ModbusRTU::WriteOutputMessage msg(addr,it->first);
                //        cout << "send reg=" << ModbusRTU::dat2str(it->first)
                //                            << "(" << it->first << ")" << endl;
                int reg = it->first;
                //        bool ok = false;

                for( auto it1 = it->second.begin(); it1 != it->second.end(); ++it1, reg++ )
                {
                    const ModbusRTU::ModbusData* last = skip + sizeof(skip) / sizeof(skip[0]);

                    if( std::find(skip, last, reg) != last)
                        continue;

                    cout << "send reg=" << ModbusRTU::dat2str(reg)
                         << "(" << reg << ")"
                         << "=" << ModbusRTU::dat2str( (*it1) )  << endl;

                    //            ok=true;
                    //            continue;
                    for( unsigned int i = 0; i < attempts; i++ )
                    {
                        try
                        {
                            ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06(addr, reg, (*it1));

                            if( verb )
                                cout << "(mtr-setup): write reply: " << ret << endl;

                            update_communication_params(reg, *it1, mb, addr, verb);
                            //                    ok = true;
                            break;
                        }
                        catch( ModbusRTU::mbException& ex )
                        {
                            /* if speed is changed we receive a timeout error */
                            if( reg == regBaudRate && ex.err == ModbusRTU::erTimeOut )
                            {
                                update_communication_params(reg, *it1, mb, addr, verb);
                                //                        ok = true;
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

                //      if( !ok )
                //          return false;
            }

            try
            {
                ModbusRTU::ModbusData dat = 1;
                ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06( addr, regUpdateConfiguration, dat);

                if( ret.start == regUpdateConfiguration && ret.data == dat )
                {
                    if( verb )
                        cout << "(mtr-setup): save parameters " << endl;

                    return true;
                }
            }
            catch(  const ModbusRTU::mbException& ex )
            {
                if( verb )
                    cout << "(mtr-setup): save parameter error: " << ex << endl;
            }

            if( verb )
                cout << "(mtr-setup): not save parameters " << endl;

            return false;
        }
        // ------------------------------------------------------------------------------------------
        MTR::MTRError update_configuration(ModbusClient* mb, ModbusRTU::ModbusAddr slaveaddr,
                                           const std::string& mtrconfile,  int verb )
        {
            std::string m = MTR::getModelNumber(mb, slaveaddr);

            if( m != "MTR315Transducer" && m != "MTR3-3Transducer" )
            {
                cerr << "(mtr-setup): model number must be 'MTR315Transducer' or 'MTR3-3Transducer' (read: '" << m << "')" << endl;
                return mtrBadDeviceType;
            }

            DataMap dmap = MTR::read_confile(mtrconfile);

            if( dmap.empty() )
            {
                cerr << "(mtr-setup): error read confile=" << mtrconfile << endl;
                return mtrDontReadConfile;
            }

            return send_param(mb, dmap, slaveaddr, verb) ? mtrNoError : mtrSendParamFailed;
        }
        // ------------------------------------------------------------------------------------------
        std::ostream& operator<<(std::ostream& os, MTR::MTRError& e )
        {
            switch(e)
            {
                case mtrNoError:
                    os << "mtrNoError";
                    break;

                case mtrBadDeviceType:
                    os << "mtrBadDeviceType";
                    break;

                case mtrDontReadConfile:
                    os << "mtrDontReadConfile";
                    break;

                case mtrSendParamFailed:
                    os << "mtrSendParamFailed";
                    break;

                default:
                    os << "Unknown error";
                    break;
            }

            return os;
        }
        // ------------------------------------------------------------------------------------------
        std::ostream& operator<<(std::ostream& os, MTR::T1& t )
        {
            return os << t.val;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T2& t )
        {
            return os << t.val;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T3& t )
        {
            return os << (long)(t);
        }
        std::ostream& operator<<(std::ostream& os, MTR::T4& t )
        {
            return os << t.sval;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T5& t )
        {
            return os << t.raw.u2.val << " * 10^" << (int)t.raw.u2.exp
                   << " [" << t.val << "]";
        }
        std::ostream& operator<<(std::ostream& os, MTR::T6& t )
        {
            return os << t.raw.u2.val << " * 10^" << (int)t.raw.u2.exp
                   << " [" << t.val << "]";
        }
        std::ostream& operator<<(std::ostream& os, MTR::T7& t )
        {
            return os << t.val
                   << " [" << ( t.raw.u2.ic == 0xFF ? "CAP" : "IND" ) << "|"
                   << ( t.raw.u2.ie == 0xFF ? "EXP" : "IMP" ) << "]";
        }
        std::ostream& operator<<(std::ostream& os, MTR::T8& t )
        {
            uniset::ios_fmt_restorer l(os);
            os << setfill('0') << hex
               << setw(2) << t.hour() << ":" << setw(2) << t.min()
               << " " << setw(2) << t.day() << "/" << setw(2) << t.mon();
            return os;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T9& t )
        {
            uniset::ios_fmt_restorer l(os);
            os << setfill('0') << hex
               << setw(2) << t.hour() << ":" << setw(2) << t.min()
               << ":" << setw(2) << t.sec() << "." << setw(2) << t.ssec();
            return os;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T10& t )
        {
            uniset::ios_fmt_restorer l(os);
            os << setfill('0') << dec
               << setw(4) << t.year() << "/" << setw(2) << t.mon()
               << "/" << setw(2) << t.day();
            return os;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T16& t )
        {
            return os << t.fval;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T17& t )
        {
            return os << t.fval;
        }
        std::ostream& operator<<(std::ostream& os, MTR::F1& t )
        {
            return os << t.raw.val;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T_Str8& t )
        {
            return os << t.sval;
        }
        std::ostream& operator<<(std::ostream& os, MTR::T_Str16& t )
        {
            return os << t.sval;
        }
        // ------------------------------------------------------------------------------------------
    } // end of namespace MTR
} // end of namespace uniset
// -----------------------------------------------------------------------------
