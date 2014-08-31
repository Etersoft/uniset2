// -----------------------------------------------------------------------------
#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>
#include "modbus/ModbusRTUMaster.h"
#include "RTUStorage.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
RTUStorage::RTUStorage( ModbusRTU::ModbusAddr a ):
    addr(a),
    pingOK(false),
    pollADC(true),
    pollDI(true),
    pollDIO(true),
    pollUNIO(true)
{
    memset(adc,0,sizeof(adc));
    memset(di,0,sizeof(di));
    memset(dio_do,0,sizeof(dio_do));
    memset(dio_di,0,sizeof(dio_di));
    memset(dio_ai,0,sizeof(dio_ai));
    memset(dio_ao,0,sizeof(dio_ao));
    memset(unio_di,0,sizeof(unio_di));
    memset(unio_do,0,sizeof(unio_do));
    memset(unio_ai,0,sizeof(unio_ai));
    memset(unio_ao,0,sizeof(unio_ao));
}
// -----------------------------------------------------------------------------
RTUStorage::~RTUStorage()
{

}
// -----------------------------------------------------------------------------
void RTUStorage::poll( const std::shared_ptr<ModbusRTUMaster>& mb ) throw( ModbusRTU::mbException )
{
    try
    {
        pingOK = true;

        // опрос АЦП
        if( pollADC )
        {
            ModbusRTU::ReadInputRetMessage ret = mb->read04( addr,1016, 16 );
            for( unsigned int i=0,k=0; i<16; i+=2,k++ )
                adc[k] = ModbusRTU::dat2f(ret.data[i],ret.data[i+1]);
        }
        // -----------------------------------
        // опрос 16 DI
        if( pollDI )
        {
            ModbusRTU::ReadInputStatusRetMessage ret = mb->read02( addr,0,16 );
            ModbusRTU::DataBits bits;
            for( unsigned int b=0; b<2; b++ )
            {
                if( ret.getData(b,bits) )
                {
                    for( unsigned int i=0; i<8; i++ )
                        di[i+8*b] = bits[i];
                }
            }
        }
        // -----------------------------------
        // опрос 16DIO DO
        if( pollDIO )
        {
            {
                ModbusRTU::ReadCoilRetMessage ret = mb->read01( addr,0,16 );
                ModbusRTU::DataBits bits;
                for( unsigned int b=0; b<2; b++ )
                {
                    if( ret.getData(b,bits) )
                    {
                        for( unsigned int i=0; i<8; i++ )
                            dio_do[i+8*b] = bits[i];
                    }
                }
            }
            // -----------------------------------
            // опрос 16DIO DI
            {
                ModbusRTU::ReadInputStatusRetMessage ret = mb->read02( addr,16,16 );
                ModbusRTU::DataBits bits;
                for( unsigned int b=0; b<2; b++ )
                {
                    if( ret.getData(b,bits) )
                    {
                        for( unsigned int i=0; i<8; i++ )
                            dio_di[i+8*b] = bits[i];
                    }
                }
            }
            // -----------------------------------
            // опрос 16DIO AI
            {
                ModbusRTU::ReadInputRetMessage ret = mb->read04( addr, 1000, 16 );
                int k = 0;
                for( unsigned int i=0; i<16; i+=2,k++ )
                    dio_ai[k] = ModbusRTU::dat2f(ret.data[i],ret.data[i+1]);
            }
            // -----------------------------------
            // опрос 16DIO AO
            {
                ModbusRTU::ReadOutputRetMessage ret = mb->read03( addr, 1000, 16 );
                int k = 0;
                for( unsigned int i=0; i<16; i+=2,k++ )
                    dio_ao[k] = ModbusRTU::dat2f(ret.data[i],ret.data[i+1]);
            }
            // -----------------------------------
        }

        if( pollUNIO )
        {
            // опрос UNIO48 DO
            {
                ModbusRTU::ReadCoilRetMessage ret = mb->read01( addr,16,48 );
                ModbusRTU::DataBits bits;
                for( unsigned int b=0; b<8; b++ )
                {
                    if( ret.getData(b,bits) )
                    {
                        for( unsigned int i=0; i<8; i++ )
                            unio_do[i+8*b] = bits[i];
                    }
                }
            }
            // -----------------------------------
            // опрос UNIO48 DI
            {
                ModbusRTU::ReadInputStatusRetMessage ret = mb->read02( addr,32,48 );
                ModbusRTU::DataBits bits;
                for( unsigned int b=0; b<8; b++ )
                {
                    if( ret.getData(b,bits) )
                    {
                        for( unsigned int i=0; i<8; i++ )
                            unio_di[i+8*b] = bits[i];
                    }
                }
            }
            // -----------------------------------
            // опрос UNIO48 AI
            {
                ModbusRTU::ReadInputRetMessage ret = mb->read04( addr, 1032, 48 );
                int k = 0;
                for( unsigned int i=0; i<48; i+=2,k++ )
                    unio_ai[k] = ModbusRTU::dat2f(ret.data[i],ret.data[i+1]);
            }
            // -----------------------------------
            // опрос UNIO48 AO
            {
                ModbusRTU::ReadOutputRetMessage ret = mb->read03( addr, 1016, 48 );
                int k = 0;
                for( unsigned int i=0; i<48; i+=2,k++ )
                    unio_ao[k] = ModbusRTU::dat2f(ret.data[i],ret.data[i+1]);
            }
        }
    }
    catch(...)
    {
        pingOK = false;
        throw;
    }
}
// -----------------------------------------------------------------------------
long RTUStorage::getInt( RTUJack jack, unsigned short int chan, UniversalIO::IOType t )
{
    return lroundf( getFloat(jack,chan,t) );
}
// -----------------------------------------------------------------------------
float RTUStorage::getFloat( RTUJack jack, unsigned short int chan, UniversalIO::IOType t )
{
    if( t == UniversalIO::AI )
    {
        switch( jack )
        {
            case nJ1:
                return unio_ai[chan];
            case nJ2:
                return unio_ai[12+chan];
            case nJ5:
                return dio_ai[chan];
            case nX1:
                return adc[chan];
            case nX2:
                return adc[4+chan];

            default:
                break;
        }

        return 0;
    }

    if( t == UniversalIO::AO )
    {
        switch( jack )
        {
            case nJ1:
                return unio_ao[chan];
            case nJ2:
                return unio_ao[12+chan];
            case nJ5:
                return dio_ao[chan];
            case nX1:
                return adc[chan];
            case nX2:
                return adc[4+chan];

            default:
                break;
        }

        return 0;
    }

    return 0;
}
// -----------------------------------------------------------------------------
bool RTUStorage::getState( RTUJack jack, unsigned short int chan, UniversalIO::IOType t )
{
    if( t == UniversalIO::DI )
    {
        switch( jack )
        {
            case nJ1:
                return unio_di[chan];
            case nJ2:
                return unio_di[24+chan];
            case nJ5:
                return dio_di[chan];
            case nX4:
                return di[chan];
            case nX5:
                return di[8+chan];

            default:
                break;
        }

        return false;
    }

    if( t == UniversalIO::DO )
    {
        switch( jack )
        {
            case nJ1:
                return unio_do[chan];
            case nJ2:
                return unio_do[24+chan];
            case nJ5:
                return dio_do[chan];

            default:
                break;
        }

        return false;
    }

    return false;
}
// -----------------------------------------------------------------------------
ModbusRTU::ModbusData RTUStorage::getRegister( RTUJack jack, unsigned short chan, UniversalIO::IOType t )
{
    if( t == UniversalIO::AI )
    {
        switch( jack )
        {
            case nJ1:
                return 1032+chan;
            case nJ2:
                return 1032+24+chan;
            case nJ5:
                return 1000+chan;
            case nX1:
                return 1016+chan;
            case nX2:
                return 1016+4+chan;

            default:
                break;
        }

        return -1;
    }

    if( t == UniversalIO::AO )
    {
        switch( jack )
        {
            case nJ1:
                return 1016+chan;
            case nJ2:
                return 1016+24+chan;
            case nJ5:
                return 1000+chan;
            case nX1:
                return 1016+chan;
            case nX2:
                return 1016+4+chan;

            default:
                break;
        }

        return -1;
    }

    if( t == UniversalIO::DI )
    {
        switch( jack )
        {
            case nJ1:
                return 32+chan;
            case nJ2:
                return 32+24+chan;
            case nJ5:
                return 16+chan;
            case nX4:
                return chan;
            case nX5:
                return 8+chan;

            default:
                break;
        }

        return -1;
    }

    if( t == UniversalIO::DO )
    {
        switch( jack )
        {
            case nJ1:
                return 16+chan;
            case nJ2:
                return 16+24+chan;
            case nJ5:
                return chan;

            default:
                break;
        }

        return -1;
    }

    return -1;
}
// -----------------------------------------------------------------------------
ModbusRTU::SlaveFunctionCode RTUStorage::getFunction( RTUJack jack, unsigned short chan, UniversalIO::IOType t )
{
    if( t == UniversalIO::AI )
    {
        switch( jack )
        {
            case nJ1:
            case nJ2:
            case nJ5:
            case nX1:
            case nX2:
                return ModbusRTU::fnReadInputRegisters;

            default:
                break;
        }

        return ModbusRTU::fnUnknown;
    }

    if( t == UniversalIO::AO )
    {
        switch( jack )
        {
            case nJ1:
            case nJ2:
            case nJ5:
                return ModbusRTU::fnReadOutputRegisters;

            case nX1:
            case nX2:
                return ModbusRTU::fnReadInputRegisters;

            default:
                break;
        }

        return ModbusRTU::fnUnknown;
    }

    if( t == UniversalIO::DI )
    {
        switch( jack )
        {
            case nJ1:
            case nJ2:
            case nJ5:
            case nX4:
            case nX5:
                return ModbusRTU::fnReadInputStatus;

            default:
                break;
        }

        return ModbusRTU::fnUnknown;
    }

    if( t == UniversalIO::DO )
    {
        switch( jack )
        {
            case nJ1:
            case nJ2:
            case nJ5:
                return ModbusRTU::fnReadCoilStatus;

            default:
                break;
        }

        return ModbusRTU::fnUnknown;
    }

    return ModbusRTU::fnUnknown;
}
// -----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, RTUStorage& m )
{
    os << "-------------------" << endl
         << " АЦП (8 каналов): " << endl;
    for( unsigned int i=0; i<8; i++ ) // номера каналов
        os << setw(12) << i << "|";
    os << endl;
    for( unsigned int i=0; i<8; i++ )
        os << setw(12) << m.adc[i] << "|";
    os << endl;
    os << "-------------------" << endl
         << " DI (16 каналов): " << endl;
    for( unsigned int i=0; i<16; i++ ) // номера каналов
        os << setw(2) << i << "|";
    os << endl;
    for( unsigned int i=0; i<16; i++ )
        os << setw(2) << m.di[i] << "|";
    os << endl;

    os << "-------------------" << endl
         << " DIO DO(16 каналов): " << endl;
    for( unsigned int i=0; i<16; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<16; i++ )
        os << setw(2) << m.dio_do[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " DIO DI(16 каналов): " << endl;
    for( unsigned int i=0; i<16; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<16; i++ )
        os << setw(2) << m.dio_di[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " DIO AI (16 каналов): " << endl;
    for( unsigned int i=0; i<16; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<16; i++ )
        os << setw(2) << m.dio_ai[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " DIO AO (16 каналов): " << endl;
    for( unsigned int i=0; i<16; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<16; i++ )
        os << setw(2) << m.dio_ao[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " UNIO48 DI: " << endl;
    for( unsigned int i=0; i<24; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<24; i++ )
        os << setw(2) << m.unio_di[i] << " | ";
    os << endl;
    for( unsigned int i=24; i<48; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=24; i<48; i++ )
        os << setw(2) << m.unio_di[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " UNIO48 DO: " << endl;
    for( unsigned int i=0; i<24; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<24; i++ )
        os << setw(2) << m.unio_do[i] << " | ";
    os << endl;
    for( unsigned int i=24; i<48; i++ ) // номера каналов
        os << setw(2) << i << " | ";
    os << endl;
    for( unsigned int i=24; i<48; i++ )
        os << setw(2) << m.unio_do[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " UNIO48 AI: " << endl;
    for( unsigned int i=0; i<12; i++ ) // номера каналов
        os << setw(6) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<12; i++ )
        os << setw(6) << m.unio_ai[i] << " | ";
    os << endl;
    for( unsigned int i=12; i<24; i++ ) // номера каналов
        os << setw(6) << i << " | ";
    os << endl;
    for( unsigned int i=12; i<24; i++ )
        os << setw(6) << m.unio_ai[i] << " | ";
    os << endl;

    os << "-------------------" << endl
         << " UNIO48 AO: " << endl;
    for( unsigned int i=0; i<12; i++ ) // номера каналов
        os << setw(6) << i << " | ";
    os << endl;
    for( unsigned int i=0; i<12; i++ )
        os << setw(6) << m.unio_ao[i] << " | ";
    os << endl;
    for( unsigned int i=12; i<24; i++ ) // номера каналов
        os << setw(6) << i << " | ";
    os << endl;
    for( unsigned int i=12; i<24; i++ )
        os << setw(6) << m.unio_ao[i] << " | ";
    os << endl;

    return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, RTUStorage* m )
{
    return os << (*m);
}
// -----------------------------------------------------------------------------
void RTUStorage::print()
{
    cout << this;
}
// -----------------------------------------------------------------------------
RTUStorage::RTUJack RTUStorage::s2j( const std::string& jack )
{
    if( jack == "J1" || jack == "j1" )
        return nJ1;
    if( jack == "J2" || jack == "j2" )
        return nJ2;
    if( jack == "J5" || jack == "j5" )
        return nJ5;

    if( jack == "X1" || jack == "x1" )
        return nX1;
    if( jack == "X2" || jack == "x2" )
        return nX2;
    if( jack == "X4" || jack == "x4" )
        return nX4;
    if( jack == "X5" || jack == "x5" )
        return nX5;

    return nUnknown;
}
// -----------------------------------------------------------------------------
std::string RTUStorage::j2s( RTUStorage::RTUJack jack )
{
    if( jack == nJ1 )
        return "J1";
    if( jack == nJ2 )
        return "J2";
    if( jack == nJ5 )
        return "J5";

    if( jack == nX1 )
        return "X1";
    if( jack == nX2 )
        return "X2";
    if( jack == nX4 )
        return "X4";
    if( jack == nX5 )
        return "X5";

    return "";
}
// -----------------------------------------------------------------------------
