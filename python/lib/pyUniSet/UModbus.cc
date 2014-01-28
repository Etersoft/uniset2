#include <ostream>
#include <cmath>
#include "UModbus.h"
// -------------------------------------------------------------------------- 
using namespace std;
// --------------------------------------------------------------------------
#if 0
UModbus::UModbus( UTypes::Params* p )throw(UException):
mb(0),
ip(""),
port(512),
tout_msec(5000)
{
    try
    {
        mb = new ModbusTCPMaster();
    }
    catch(...)
    {
        throw UException();
    }
}
//---------------------------------------------------------------------------
UModbus::UModbus( int argc, char** argv )throw(UException):
mb(0),
ip(""),
port(512),
tout_msec(5000)
{
    try
    {
        mb = new ModbusTCPMaster();
    }
    catch(...)
    {
        throw UException();
    }
}
#endif
// --------------------------------------------------------------------------
UModbus::UModbus():
mb(0),
ip(""),
port(512),
tout_msec(5000)
{
    mb = new ModbusTCPMaster();
}
// --------------------------------------------------------------------------
UModbus::~UModbus()
{
    delete mb;
}
// --------------------------------------------------------------------------
void UModbus::prepare( const char* _ip, int _port )throw(UException)
{
    if( !mb )
      throw UException("(connect): mb=NULL?!");

    string s(_ip);
    if( mb->isConnection() )
    {
         if( _port == port && s==ip )
             return;

         mb->disconnect();
    }

    //cerr << "************** Prepare: " << string(_ip) << ":" << port << endl;
    // strncpy(char *dest, const char *src, size_t n);
    ip = s;
    port = _port;
}
// --------------------------------------------------------------------------
void UModbus::connect( const char* _ip, int _port )throw(UException)
{
    if( !mb )
      throw UException("(connect): mb=NULL?!");

    string s(_ip);
    if( mb->isConnection() )
    {
         if( _port == port && s==ip )
             return;

         mb->disconnect();
    }

    ip = s;
    port = _port;
    ost::InetAddress ia(_ip);
    try
    {
//        cerr << "************** Connect: " << ia << ":" << port << " ip:" << ip << endl;
        mb->connect(ia,port);
    }
    catch( ModbusRTU::mbException& ex )
    {
        std::ostringstream err;
        err << ex;
        throw UException(err.str());
    }
    catch(...)
    {
        std::ostringstream err;
        err << "Connection failed: ip=" << ip << " port=" << port;
        throw UException(err.str());
    }    
}
// --------------------------------------------------------------------------
long UModbus::getWord( int addr, int mbreg, int mbfunc )throw(UException)
{
    return mbread(addr,mbreg,mbfunc,"signed");
}
// --------------------------------------------------------------------------
long UModbus::getByte( int addr, int mbreg, int mbfunc )throw(UException)
{
    return mbread(addr,mbreg,mbfunc,"byte");
}
// --------------------------------------------------------------------------
bool UModbus::getBit( int addr, int mbreg, int mbfunc )throw(UException)
{
    return mbread(addr,mbreg,mbfunc,"unsigned");
}
// --------------------------------------------------------------------------
long UModbus::mbread( int mbaddr, int mbreg, int mbfunc, const char* s_vtype, int nbit,
                        const char* new_ip, int new_port )throw(UException)
{
    using namespace VTypes;

//    const char* n_ip = strcmp(new_ip,"") ? new_ip : ip;
    const char* n_ip = (new_ip != 0) ? new_ip : ip.c_str();
    int n_port = ( new_port > 0 ) ? new_port : port;    

    connect(n_ip,n_port);
    
    VType vt = str2type(s_vtype);
    if( vt == vtUnknown )
    {
        std::ostringstream err;
        err << "(mbread): Unknown vtype='" << string(s_vtype) << "'";
        throw UException(err.str());
    }

    if( nbit >= 16 )
    {
        std::ostringstream err;
        err << "(mbread): BAD nbit='%d' (nbit must be <16)" << nbit;
        throw UException(err.str());
    }

    int b_count = wsize(vt);
    try
    {
        switch( mbfunc )
        {
            case ModbusRTU::fnReadInputRegisters:
            {
                ModbusRTU::ReadInputRetMessage ret = mb->read04(mbaddr,mbreg,b_count);
                if( nbit < 0 )
                    return data2value(vt,ret.data);

                ModbusRTU::DataBits16 b(ret.data[0]);
                return b[nbit];
            }
            break;

            case ModbusRTU::fnReadOutputRegisters:
            {
                ModbusRTU::ReadOutputRetMessage ret = mb->read03(mbaddr,mbreg,b_count);
                if( nbit < 0 )
                    return data2value(vt,ret.data);

                ModbusRTU::DataBits16 b(ret.data[0]);
                return b[nbit];
            }
            break;

            case ModbusRTU::fnReadInputStatus:
            {
                ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(mbaddr,mbreg,1);
                ModbusRTU::DataBits b(ret.data[0]);
                return b[0];
            }
            break;

            case ModbusRTU::fnReadCoilStatus:
            {
                ModbusRTU::ReadCoilRetMessage ret = mb->read01(mbaddr,mbreg,1);
                ModbusRTU::DataBits b(ret.data[0]);
                return b[0];
            }
            break;
            default:
            {
                std::ostringstream err;
                err << "(mbread): Unsupported function = '" << mbfunc << "'";
                throw UException(err.str());
            }
            break;
        }
    }
    catch( ModbusRTU::mbException& ex )
    {
        if( ex.err != ModbusRTU::erTimeOut )
            throw UTimeOut();

        std::ostringstream err;
        err << ex;
        throw UException(err.str());
    }
    catch(...)
    {
        throw UException("(mbread): catch...");
    }
}
//---------------------------------------------------------------------------
long UModbus::data2value( VTypes::VType vtype, ModbusRTU::ModbusData* data )
{
#if 0    
    if( vt == VTypes::vtUnknown )
    {
        ModbusRTU::DataBits16 b(data[0]);
        if( nbit >= 0 )
            return b[p->nbit] ? 1 : 0;

        if( p->rnum <= 1 )
            {
                if( p->stype == UniversalIO::DI ||
                    p->stype == UniversalIO::DO )
                {
                    IOBase::processingAsDI( p, data[0], shm, force );
                }
                else
                    IOBase::processingAsAI( p, (signed short)(data[0]), shm, force );

                return true;
            }

            dlog.crit() << myname << "(initSMValue): IGNORE item: rnum=" << p->rnum
                    << " > 1 ?!! for id=" << p->si.id << endl;

            return false;
        }
#endif

    if( vtype == VTypes::vtSigned )
        return (signed short)(data[0]);

    else if( vtype == VTypes::vtUnsigned )
        return (unsigned short)data[0];

    else if( vtype == VTypes::vtByte )
    {
        VTypes::Byte b(data[0]);
        return (long)(b.raw.b[0]);
    }
    else if( vtype == VTypes::vtF2 )
    {
        VTypes::F2 f(data,VTypes::F2::wsize());
        return lroundf( (float)f );
    }
    else if( vtype == VTypes::vtF4 )
    {
        VTypes::F4 f(data,VTypes::F4::wsize());
        return lroundf( (float)f );
    }
    else if( vtype == VTypes::vtI2 )
    {
        VTypes::I2 i2(data,VTypes::I2::wsize());
        return (int)i2;
    }
    else if( vtype == VTypes::vtU2 )
    {
        VTypes::U2 u2(data,VTypes::U2::wsize());
        return (unsigned int)u2;
    }

    return 0;
}
//---------------------------------------------------------------------------
void UModbus::mbwrite( int mbaddr, int mbreg, int val, int mbfunc, const char* new_ip, int new_port )throw(UException)
{
    const char* n_ip = (new_ip != 0) ? new_ip : ip.c_str();
    int n_port = ( new_port > 0 ) ? new_port : port;

    connect(n_ip,n_port);

    try
    {
        switch( mbfunc )
        {
            case ModbusRTU::fnWriteOutputSingleRegister:
            {
                ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06(mbaddr,mbreg,val);
            }
            break;

            case ModbusRTU::fnWriteOutputRegisters:
            {
                ModbusRTU::WriteOutputMessage msg(mbaddr,mbreg);
                msg.addData(val);
                ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
            }
            break;

            case ModbusRTU::fnForceSingleCoil:
            {
                 ModbusRTU::ForceSingleCoilRetMessage ret = mb->write05(mbaddr,mbreg,val);
            }
            break;

            case ModbusRTU::fnForceMultipleCoils:
            {
                ModbusRTU::ForceCoilsMessage msg(mbaddr,mbreg);
                msg.addBit( (val ? true : false) );
                ModbusRTU::ForceCoilsRetMessage ret = mb->write0F(msg);
            }
            break;

            default:
            {
                std::ostringstream err;
                err << "(mbwrite): Unsupported function = '" << mbfunc << "'";
                throw UException(err.str());
            }
            break;
        }
    }
    catch( ModbusRTU::mbException& ex )
    {
        if( ex.err != ModbusRTU::erTimeOut )
            throw UTimeOut();

        std::ostringstream err;
        err << ex;
        throw UException(err.str());
    }
    catch(...)
    {
        throw UException("(mbwrite): catch...");
    }
}
//---------------------------------------------------------------------------
