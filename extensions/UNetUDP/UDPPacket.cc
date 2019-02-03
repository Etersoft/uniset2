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
#include <endian.h>
#include "UDPPacket.h"
// -------------------------------------------------------------------------
// сделано так, чтобы макросы раскрывались в "пустоту" если не требуется преобразование
// поэтому использование выглядит как LE_TO_H( myvar ), а не
// myvar = LE_TO_H(myvar)
// -------------------------------------------------------------------------
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define LE_TO_H(x) {}
#elif INTPTR_MAX == INT64_MAX
#define LE_TO_H(x) x = le64toh(x)
#elif INTPTR_MAX == INT32_MAX
#define LE_TO_H(x) x = le32toh(x)
#else
#error UNET(LE_TO_H): Unknown byte order or size of pointer
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define BE_TO_H(x) {}
#elif INTPTR_MAX == INT64_MAX
#define BE_TO_H(x) x = be64toh(x)
#elif INTPTR_MAX == INT32_MAX
#define BE_TO_H(x) x = be32toh(x)
#else
#error UNET(BE_TO_H): Unknown byte order or size of pointer
#endif
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
	std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPHeader& p )
	{
		return os << "nodeID=" << p.nodeID
			   << " procID=" << p.procID
			   << " dcount=" << p.dcount
			   << " acount=" << p.acount
			   << " pnum=" << p.num;
	}
	// -----------------------------------------------------------------------------
	std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPHeader* p )
	{
		return os << (*p);
	}
	// -----------------------------------------------------------------------------

	std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPAData& p )
	{
		return os << "id=" << p.id << " val=" << p.val;
	}
	// -----------------------------------------------------------------------------
	std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPMessage& p )
	{
		os << (UDPHeader*)(&p) << endl;

		os << "DIGITAL:" << endl;

		for( size_t i = 0; i < p.dcount; i++ )
			os << "[" << i << "]={" << p.dID(i) << "," << p.dValue(i) << "}" << endl;

		os << "ANALOG:" << endl;

		for( size_t i = 0; i < p.acount; i++ )
			os << "[" << i << "]={" << p.a_dat[i].id << "," << p.a_dat[i].val << "}" << endl;

		return os;
	}
	// -----------------------------------------------------------------------------
	UDPMessage::UDPMessage() noexcept
	{
	}
	// -----------------------------------------------------------------------------
	size_t UDPMessage::addAData( const UniSetUDP::UDPAData& dat ) noexcept
	{
		if( acount >= MaxACount )
			return MaxACount;

		a_dat[acount] = dat;
		acount++;
		return acount - 1;
	}
	// -----------------------------------------------------------------------------
	size_t UDPMessage::addAData( long id, long val) noexcept
	{
		UDPAData d(id, val);
		return addAData(d);
	}
	// -----------------------------------------------------------------------------
	bool UDPMessage::setAData( size_t index, long val ) noexcept
	{
		if( index < MaxACount )
		{
			a_dat[index].val = val;
			return true;
		}

		return false;
	}
	// -----------------------------------------------------------------------------
	size_t UDPMessage::addDData( long id, bool val ) noexcept
	{
		if( dcount >= MaxDCount )
			return MaxDCount;

		// сохраняем ID
		d_id[dcount] = id;

		bool res = setDData( dcount, val );

		if( res )
		{
			dcount++;
			return dcount - 1;
		}

		return MaxDCount;
	}
	// -----------------------------------------------------------------------------
	bool UDPMessage::setDData( size_t index, bool val ) noexcept
	{
		if( index >= MaxDCount )
			return false;

		size_t nbyte = index / 8 * sizeof(unsigned char);
		size_t nbit =  index % 8 * sizeof(unsigned char);

		// выставляем бит
		unsigned char d = d_dat[nbyte];

		if( val )
			d |= (1 << nbit);
		else
			d &= ~(1 << nbit);

		d_dat[nbyte] = d;
		return true;
	}
	// -----------------------------------------------------------------------------
	long UDPMessage::dID( size_t index ) const noexcept
	{
		if( index >= MaxDCount )
			return uniset::DefaultObjectId;

		return d_id[index];
	}
	// -----------------------------------------------------------------------------
	bool UDPMessage::dValue( size_t index ) const noexcept
	{
		if( index >= MaxDCount )
			return uniset::DefaultObjectId;

		size_t nbyte = index / 8 * sizeof(unsigned char);
		size_t nbit =  index % 8 * sizeof(unsigned char);

		return ( d_dat[nbyte] & (1 << nbit) );
	}
	// -----------------------------------------------------------------------------
	size_t UDPMessage::transport_msg( UDPPacket& p ) const noexcept
	{
		memset(&p, 0, sizeof(UDPPacket));

		size_t i = 0;
		memcpy(&(p.data[i]), this, sizeof(UDPHeader));
		i += sizeof(UDPHeader);

		// копируем аналоговые данные
		size_t sz = acount * sizeof(UDPAData);
		memcpy(&(p.data[i]), a_dat, sz);
		i += sz;

		// копируем булевые индексы
		sz = dcount * sizeof(long);
		memcpy(&(p.data[i]), d_id, sz);
		i += sz;

		// копируем булевые данные
		size_t nbyte = dcount / 8 * sizeof(unsigned char);
		size_t nbit =  dcount % 8 * sizeof(unsigned char);
		sz = nbit > 0 ? nbyte + 1 : nbyte;
		memcpy(&(p.data[i]), d_dat, sz);
		i += sz;

		p.len = i;
		return i;
	}
	// -----------------------------------------------------------------------------
	long UDPMessage::getDataID() const noexcept
	{
		// в качестве идентификатора берётся ID первого датчика в данных
		// приоритет имеет аналоговые датчики

		if( acount > 0 )
			return a_dat[0].id;

		if( dcount > 0 )
			return d_id[0];

		// если нет данных(?) просто возвращаем номер пакета
		return num;
	}
	// -----------------------------------------------------------------------------
	size_t UniSetUDP::UDPMessage::sizeOf() const noexcept
	{
		// биты которые не уместились в очередной байт, добавляют ещё один байт
		size_t nbit =  dcount % 8 * sizeof(unsigned char);
		size_t add = nbit > 0 ? 1 : 0;

		return sizeof(UDPHeader) + acount * sizeof(UDPAData) + dcount * sizeof(long) + (dcount / 8 * sizeof(unsigned char) + add);
	}
	// -----------------------------------------------------------------------------
	UDPMessage::UDPMessage( UDPPacket& p ) noexcept
	{
		getMessage(*this, p);
	}
	// -----------------------------------------------------------------------------
	size_t UDPMessage::getMessage( UDPMessage& m, UDPPacket& p ) noexcept
	{
		memset(&m, 0, sizeof(m));

		size_t i = 0;
		memcpy(&m, &(p.data[i]), sizeof(UDPHeader));
		i += sizeof(UDPHeader);

		// byte order from packet
		u_char be_order = m._be_order;

		if( be_order )
		{
			BE_TO_H(m.magic);
			BE_TO_H(m.num);
			BE_TO_H(m.procID);
			BE_TO_H(m.nodeID);
			BE_TO_H(m.dcount);
			BE_TO_H(m.acount);
		}
		else
		{
			LE_TO_H(m.magic);
			LE_TO_H(m.num);
			LE_TO_H(m.procID);
			LE_TO_H(m.nodeID);
			LE_TO_H(m.dcount);
			LE_TO_H(m.acount);
		}

		// set host byte order
#if __BYTE_ORDER == __LITTLE_ENDIAN
		m._be_order = 0;
#elif __BYTE_ORDER == __BIG_ENDIAN
		m. be_order = 1;
#else
#error UNET(getMessage): Unknown byte order!
#endif

		// проверяем наш ли пакет..
		if( m.magic != UniSetUDP::UNETUDP_MAGICNUM )
		{
			m.magic = 0;
			return 0;
		}

		// копируем аналоговые данные
		size_t sz = m.acount * sizeof(UDPAData);

		if( sz > sizeof(m.a_dat) )
			sz = sizeof(m.a_dat);

		memcpy(m.a_dat, &(p.data[i]), sz);
		i += sz;

		// копируем булевые индексы
		sz = m.dcount * sizeof(long);

		if( sz > sizeof(m.d_id) )
			sz = sizeof(m.d_id);

		memcpy(m.d_id, &(p.data[i]), sz);
		i += sz;

		// копируем булевые данные
		size_t nbyte = m.dcount / 8 * sizeof(unsigned char);
		size_t nbit =  m.dcount % 8 * sizeof(unsigned char);
		sz = nbit > 0 ? nbyte + 1 : nbyte;

		if( sz > sizeof(m.d_dat) )
			sz = sizeof(m.d_dat);

		memcpy(m.d_dat, &(p.data[i]), sz);

		// CONVERT DATA TO HOST BYTE ORDER
		// -------------------------------
		for( size_t n = 0; n < m.acount; n++ )
		{
			if( be_order )
			{
				BE_TO_H(m.a_dat[n].id);
				BE_TO_H(m.a_dat[n].val);
			}
			else
			{
				LE_TO_H(m.a_dat[n].id);
				LE_TO_H(m.a_dat[n].val);
			}
		}

		for( size_t n = 0; n < m.dcount; n++ )
		{
			if( be_order )
			{
				BE_TO_H(m.d_id[n]);
			}
			else
			{
				LE_TO_H(m.d_id[n]);
			}
		}

		return i + sz;
	}
	// -----------------------------------------------------------------------------
	uint16_t UDPMessage::getDataCRC() const noexcept
	{
		uint16_t crc[3];
		crc[0] = makeCRC( (unsigned char*)(a_dat), sizeof(a_dat) );
		crc[1] = makeCRC( (unsigned char*)(d_id), sizeof(d_id) );
		crc[2] = makeCRC( (unsigned char*)(d_dat), sizeof(d_dat) );
		return makeCRC( (unsigned char*)(&crc), sizeof(crc) );
	}

	UDPHeader::UDPHeader() noexcept
		: magic(UNETUDP_MAGICNUM)
#if __BYTE_ORDER == __LITTLE_ENDIAN
		, _be_order(0)
#elif __BYTE_ORDER == __BIG_ENDIAN
		, _be_order(1)
#else
#error UNET: Unknown byte order!
#endif
		, num(0)
		, nodeID(0)
		, procID(0)
		, dcount(0)
		, acount(0)
	{}

	// -----------------------------------------------------------------------------
} // end of namespace uniset
