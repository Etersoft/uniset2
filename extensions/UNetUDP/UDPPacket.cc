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
// -------------------------------------------------------------------------
#include <cstdint>
#include "UDPPacket.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // ---------------------------------------------------------------------
    using namespace std;
    using namespace UniSetUDP;
    // ---------------------------------------------------------------------
#define USE_CRC_TAB 1 // при расчёте использовать таблицы
    // ---------------------------------------------------------------------
#ifdef USE_CRC_TAB
    static unsigned short crc_16_tab[] =
    {
        0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
        0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
        0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
        0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
        0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
        0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
        0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
        0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
        0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
        0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
        0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
        0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
        0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
        0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
        0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
        0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
        0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
        0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
        0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
        0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
        0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
        0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
        0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
        0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
        0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
        0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
        0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
        0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
        0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
        0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
        0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
        0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
    };
#endif
    // -------------------------------------------------------------------------
    /* CRC-16 is based on the polynomial x^16 + x^15 + x^2 + 1.  Bits are */
    /* sent LSB to MSB. */
    static int get_crc_16( uint16_t crc, unsigned char* buf, int size ) noexcept
    {

        while( size-- )
        {
#ifdef USE_CRC_TAB
            crc = (crc >> 8) ^ crc_16_tab[ (crc ^ * (buf++)) & 0xff ];
#else
            register int i, ch;
            ch = *(buf++);

            for (i = 0; i < 8; i++)
            {
                if ((crc ^ ch) & 1)
                    crc = (crc >> 1) ^ 0xa001;
                else
                    crc >>= 1;

                ch >>= 1;
            }

#endif
            // crc = crc & 0xffff;
        }

        return crc;
    }
    // -------------------------------------------------------------------------
    uint16_t UniSetUDP::makeCRC( unsigned char* buf, size_t len ) noexcept
    {
        uint16_t crc = 0xffff;
        crc = get_crc_16(crc, (unsigned char*)(buf), len);
        return crc;
    }
    // -----------------------------------------------------------------------------
    std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPMessage& p )
    {
        os << "nodeID=" << p.pb.nodeid()
           << " procID=" << p.pb.procid()
           << " dcount=" << p.pb.data().did_size()
           << " acount=" << p.pb.data().aid_size()
           << " pnum=" << p.pb.num()
           << endl;

        os << "DIGITAL:" << endl;

        for( size_t i = 0; i < (size_t)p.pb.data().did_size(); i++ )
            os << "[" << i << "]={" << p.pb.data().did(i) << "," << p.pb.data().dvalue(i) << "}" << endl;

        os << "ANALOG:" << endl;

        for( size_t i = 0; i < (size_t)p.pb.data().aid_size(); i++ )
            os << "[" << i << "]={" << p.pb.data().aid(i) << "," << p.pb.data().avalue(i) << "}" << endl;

        return os;
    }
    // -----------------------------------------------------------------------------
    UDPMessage::UDPMessage()
    {
        pb.set_magic(UniSetUDP::UNETUDP_MAGICNUM);
        pb.set_num(0);
        pb.set_procid(0);
        pb.set_nodeid(0);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::initFromBuffer( uint8_t* rbuf, size_t sz )
    {
        return pb.ParseFromArray(rbuf, sz);
    }
    // -----------------------------------------------------------------------------
    std::string UDPMessage::getDataAsString() const noexcept
    {
        return pb.SerializeAsString();
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::getDataAsArray( uint8_t* buf, int sz ) const noexcept
    {
        if( !pb.SerializeToArray(buf, sz) )
            return 0;

        return pb.ByteSizeLong();
    }
    // -----------------------------------------------------------------------------
    uint32_t UDPMessage::magic() const noexcept
    {
        return pb.magic();
    }
    // -----------------------------------------------------------------------------
    void UDPMessage::setNum( long num ) noexcept
    {
        pb.set_num(num);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::num() const noexcept
    {
        return pb.num();
    }
    // -----------------------------------------------------------------------------
    void UDPMessage::setNodeID( long num ) noexcept
    {
        pb.set_nodeid(num);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::nodeID() const noexcept
    {
        return pb.nodeid();
    }
    // -----------------------------------------------------------------------------
    void UDPMessage::setProcID( long num ) noexcept
    {
        pb.set_procid(num);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::procID() const noexcept
    {
        return pb.procid();
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::addAData( int64_t id, int64_t val) noexcept
    {
        if( (size_t)pb.data().aid_size() >= MaxACount )
            return MaxACount;

        pb.mutable_data()->add_aid(id);
        pb.mutable_data()->add_avalue(val);
        return pb.data().aid_size() - 1;
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::setAData( size_t index, int64_t val ) noexcept
    {
        if( index < (size_t)pb.data().aid_size() )
        {
            pb.mutable_data()->set_avalue(index, val);
            return true;
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::addDData( int64_t id, bool val ) noexcept
    {
        if( (size_t)pb.data().did_size()  >= MaxDCount )
            return MaxDCount;

        pb.mutable_data()->add_did(id);
        pb.mutable_data()->add_dvalue(val);
        return pb.data().did_size() - 1;
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::setDData( size_t index, bool val ) noexcept
    {
        if( index < (size_t)pb.data().did_size() )
        {
            pb.mutable_data()->set_dvalue(index, val);
            return true;
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::dID( size_t index ) const noexcept
    {
        if( index >= (size_t)pb.data().did_size() )
            return uniset::DefaultObjectId;

        return pb.data().did(index);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::dValue( size_t index ) const noexcept
    {
        if( index >= MaxDCount )
            return false;

        return pb.data().dvalue(index);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::aValue(size_t index) const noexcept
    {
        return pb.data().avalue(index);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::aID(size_t index) const noexcept
    {
        if( index >= (size_t)pb.data().aid_size() )
            return uniset::DefaultObjectId;

        return pb.data().aid(index);
    }
    // -----------------------------------------------------------------------------
    uint16_t UDPMessage::dataCRC() const noexcept
    {
        const std::string s = pb.data().SerializeAsString();
        return makeCRC((unsigned char*)s.data(), s.size());
    }
    // -----------------------------------------------------------------------------
    uint16_t UDPMessage::dataCRCWithBuf( uint8_t* buf, size_t sz ) const noexcept
    {
        if( !pb.data().SerializeToArray(buf, sz) )
            return 0;

        return makeCRC((unsigned char*)buf, pb.data().ByteSizeLong());
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::getDataID() const noexcept
    {
        // в качестве идентификатора берётся ID первого датчика в данных
        // приоритет имеет аналоговые датчики
        if( pb.data().aid_size() > 0 )
            return pb.data().aid(0);

        if( pb.data().did_size() > 0 )
            return pb.data().did(0);

        // если нет данных(?) просто возвращаем номер пакета
        return pb.num();
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isOk() const noexcept
    {
        return ( pb.IsInitialized() && pb.magic() == UniSetUDP::UNETUDP_MAGICNUM );
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isAFull() const noexcept
    {
        return ((size_t)pb.data().aid_size() >= MaxACount);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isDFull() const noexcept
    {
        return ((size_t)pb.data().did_size() >= MaxDCount);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isFull() const noexcept
    {
        return !(((size_t)pb.data().did_size() < MaxDCount) && ((size_t)pb.data().aid_size() < MaxACount));
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::UDPMessage::dsize() const noexcept
    {
        return pb.data().did_size();
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::asize() const noexcept
    {
        return pb.data().aid_size();
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
