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
        os << "nodeID=" << p.pack.getNodeID()
           << " procID=" << p.pack.getProcID()
           << " dcount=" << p.pack.getAdata().size()
           << " acount=" << p.pack.getDdata().size()
           << " pnum=" << p.pack.getNum()
           << endl;

        os << "DIGITAL:" << endl;

        int i = 0;

        for( auto&& d : p.pack.getDdata() )
            os << "[" << i++ << "]={" << d.getId() << "," << d.getValue() << "}" << endl;

        os << "ANALOG:" << endl;

        i = 0;

        for( auto&& a : p.pack.getAdata() )
            os << "[" << i++ << "]={" << a.getId() << "," << a.getValue() << "}" << endl;

        return os;
    }
    // -----------------------------------------------------------------------------
    const UDPMessage& UDPMessage::operator=(const UDPMessage& m)
    {
        if( &m == this )
            return *this;

        msg.setRoot(m.pack.asReader());
        pack = msg.getRoot<uniset::UNetPacket>();
        return *this;
    }
    // -----------------------------------------------------------------------------
    UDPMessage::UDPMessage(const UDPMessage& m):
        pack(msg.initRoot<uniset::UNetPacket>())
    {
        msg.setRoot(m.pack.asReader()); // copy
        pack = msg.getRoot<uniset::UNetPacket>();
    }
    // -----------------------------------------------------------------------------
    UDPMessage::UDPMessage():
        pack(msg.initRoot<uniset::UNetPacket>())
    {
        pack.setMagic(UniSetUDP::UNETUDP_MAGICNUM);
        pack.setNum(0);
        pack.setProcID(0);
        pack.setNodeID(0);
        pack.initAdata(MaxACount);
        pack.initDdata(MaxDCount);
        pack.setAnum(0);
        pack.setDnum(0);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::initFromBuffer( uint8_t* rbuf, size_t sz )
    {
        if(reinterpret_cast<uintptr_t>(rbuf) % sizeof(void*) == 0)
        {
            const kj::ArrayPtr<const capnp::word> view(
                reinterpret_cast<const capnp::word*>(&(rbuf[0])),
                reinterpret_cast<const capnp::word*>(&(rbuf[sz])));
            capnp::FlatArrayMessageReader r(view);
            auto m = r.getRoot<uniset::UNetPacket>();
            msg.setRoot(m); // copy
            pack = msg.getRoot<uniset::UNetPacket>();
        }
        else
        {
            auto arr = kj::ArrayPtr<capnp::word>(reinterpret_cast<capnp::word*>(rbuf), sz / sizeof(capnp::word));
            ::capnp::FlatArrayMessageReader r(arr);
            auto m = r.getRoot<uniset::UNetPacket>();
            msg.setRoot(m); // copy
            pack = msg.getRoot<uniset::UNetPacket>();
        }

        return magic() == UNETUDP_MAGICNUM;
    }
    // -----------------------------------------------------------------------------
    std::string UDPMessage::serializeAsString()
    {
        auto a = capnp::messageToFlatArray(msg);
        auto b = a.asBytes();
        return std::string(b.begin(), b.end());
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::serializeToArray( uint8_t* buf, int sz ) noexcept
    {
        auto a = capnp::messageToFlatArray(msg);
        auto b = a.asBytes();

        if( sz < b.size())
            return 0;

        memcpy(buf, b.begin(), b.size());
        return b.size();
    }
    // -----------------------------------------------------------------------------
    uint32_t UDPMessage::magic() const noexcept
    {
        return pack.getMagic();
    }
    // -----------------------------------------------------------------------------
    void UDPMessage::setNum( long num ) noexcept
    {
        pack.setNum(num);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::num() const noexcept
    {
        return pack.getNum();
    }
    // -----------------------------------------------------------------------------
    void UDPMessage::setNodeID( long num ) noexcept
    {
        pack.setNodeID(num);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::nodeID() const noexcept
    {
        return pack.getNodeID();
    }
    // -----------------------------------------------------------------------------
    void UDPMessage::setProcID( long num ) noexcept
    {
        pack.setProcID(num);
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::procID() const noexcept
    {
        return pack.getProcID();
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::addAData( long id, long val ) noexcept
    {
        auto anum = pack.getAnum();

        if( (size_t)anum >= MaxACount )
            return MaxACount;

        auto d = pack.getAdata()[anum];
        d.setId(id);
        d.setValue(val);
        pack.setAnum(anum + 1);
        return anum;
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::setAData( size_t index, long val ) noexcept
    {
        if( index < (size_t)pack.getAdata().size() )
        {
            pack.getAdata()[index].setValue(val);
            return true;
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::addDData( long id, bool val ) noexcept
    {
        auto dnum = pack.getDnum();

        if( (size_t)dnum >= MaxDCount )
            return MaxDCount;

        auto d = pack.getDdata()[dnum];
        d.setId(id);
        d.setValue(val);
        pack.setDnum(dnum + 1);
        return dnum;
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::setDData( size_t index, bool val ) noexcept
    {
        if( index < (size_t)pack.getDdata().size() )
        {
            pack.getDdata()[index].setValue(val);
            return true;
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::dID( size_t index ) noexcept
    {
        if( index >= (size_t)pack.getDdata().size() )
            return uniset::DefaultObjectId;

        return pack.getDdata()[index].getId();
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::dValue( size_t index ) noexcept
    {
        return pack.getDdata()[index].getValue();
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::aValue(size_t index) noexcept
    {
        return pack.getAdata()[index].getValue();
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::aID(size_t index) noexcept
    {
        if( index >= (size_t)pack.getAdata().size() )
            return uniset::DefaultObjectId;

        return pack.getAdata()[index].getId();
    }
    // -----------------------------------------------------------------------------
    uint16_t UDPMessage::dataCRC() const noexcept
    {
        uint16_t crc = 0;

        for( const auto& a : pack.getAdata() )
        {
            auto b = capnp::writeDataStruct(a).asBytes();
            crc += makeCRC((unsigned char*)b.begin(), b.size());
        }

        for( const auto& a : pack.getDdata() )
        {
            auto b = capnp::writeDataStruct(a).asBytes();
            crc += makeCRC((unsigned char*)b.begin(), b.size());
        }

        return crc;
    }
    // -----------------------------------------------------------------------------
    long UDPMessage::getDataID() const noexcept
    {
        // в качестве идентификатора берётся ID первого датчика в данных
        // приоритет имеет аналоговые датчики
        if( (size_t)pack.getAnum() > 0 )
            return pack.getAdata()[0].getId();

        if( (size_t)pack.getDnum() > 0 )
            return pack.getDdata()[0].getId();

        // если нет данных(?) просто возвращаем номер пакета
        return pack.getNum();
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isOk() const noexcept
    {
        return ( magic() == UniSetUDP::UNETUDP_MAGICNUM );
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isAFull() const noexcept
    {
        return ((size_t)pack.getAnum() >= MaxACount);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isDFull() const noexcept
    {
        return ((size_t)pack.getDnum() >= MaxDCount);
    }
    // -----------------------------------------------------------------------------
    bool UDPMessage::isFull() const noexcept
    {
        return isAFull() && isDFull();
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::UDPMessage::dsize() const noexcept
    {
        return (size_t)pack.getDnum();
    }
    // -----------------------------------------------------------------------------
    size_t UDPMessage::asize() const noexcept
    {
        return (size_t)pack.getAnum();
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
