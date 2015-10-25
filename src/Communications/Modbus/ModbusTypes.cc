#include <assert.h>
#include <string>
#include <iostream>
#include <limits>
#include <sstream>
#include <iomanip>
#include "modbus/ModbusTypes.h"
#include "UniSetTypes.h"
// -------------------------------------------------------------------------
using namespace ModbusRTU;
using namespace std;
// -------------------------------------------------------------------------
#define USE_CRC_TAB 1 // при расчёте использовать таблицы 
// -------------------------------------------------------------------------

unsigned short ModbusRTU::SWAPSHORT( unsigned short x )
{
	return ((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00));
}

// -------------------------------------------------------------------------
// Lav: не используется, см. ниже

#ifdef USE_CRC_TAB
#if 0
static unsigned short crc_ccitt_tab[] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
#endif

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
// Lav: отключил, раз не используем
#if 0
// -------------------------------------------------------------------------
static int get_crc_ccitt( unsigned short crc, unsigned char* buf, int size )
{
	while( size-- )
	{
#ifdef USE_CRC_TAB
		crc = (crc << 8) ^ crc_ccitt_tab[ (crc >> 8) ^ * (buf++) ];
#else
		register int i;
		crc ^= (unsigned short)(*(buf++)) << 8;

		for (i = 0; i < 8; i++)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}

#endif
	}

	return crc;
}
#endif
// -------------------------------------------------------------------------
/* CRC-16 is based on the polynomial x^16 + x^15 + x^2 + 1.  Bits are */
/* sent LSB to MSB. */
static int get_crc_16( unsigned short crc, unsigned char* buf, int size )
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

/*! \todo Необходимо разобраться с разными версиями и функциями расчёта CRC
    и по возможности вынести их в отдельный модуль или класс
*/
ModbusCRC ModbusRTU::checkCRC( ModbusByte* buf, int len )
{
	unsigned short crc = 0xffff;
	crc = get_crc_16(crc, (unsigned char*)(buf), len);

	//    crc = SWAPSHORT(crc);
	return crc;
}
// -------------------------------------------------------------------------
bool ModbusRTU::isWriteFunction( SlaveFunctionCode c )
{
	if( c == fnWriteOutputRegisters ||
			c == fnWriteOutputSingleRegister ||
			c == fnForceSingleCoil ||
			c == fnForceMultipleCoils )
		return true;

	return false;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::mbPrintMessage( std::ostream& os, ModbusByte* m, int len )
{
	// Чтобы не менять настройки 'os'
	// сперва создаём свой поток вывода...
	ostringstream s;

	// << setiosflags(ios::showbase) // для вывода в формате 0xNN
	s << hex << showbase << setfill('0'); // << showbase;

	for( ssize_t i = 0; i < len; i++ )
		s << setw(2) << (short)(m[i]) << " ";

	//        s << "<" << setw(2) << (int)(m[i]) << ">";

	return os << s.str();
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ModbusHeader& m )
{
	return mbPrintMessage(os, (ModbusByte*)&m, sizeof(m));
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ModbusHeader* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------

ModbusMessage::ModbusMessage():
	len(0)
{
	addr = 0;
	func = 0;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ModbusMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.len);
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ModbusMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
ErrorRetMessage::ErrorRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ErrorRetMessage& ErrorRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ErrorRetMessage::init( ModbusMessage& m )
{
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, szModbusHeader);
	ecode     = m.data[0];
	memcpy( &crc, &(m.data[1]), szCRC);
}
// -------------------------------------------------------------------------
ErrorRetMessage::ErrorRetMessage( ModbusAddr _from,
								  ModbusByte _func, ModbusByte _ecode )
{
	addr = _from;
	func = _func | MBErrMask; // выставляем старший бит
	ecode = _ecode;
}
// -------------------------------------------------------------------------
ModbusMessage ErrorRetMessage::transport_msg()
{
	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	memcpy(&mm.data, &ecode, sizeof(ecode));

	int ind = sizeof(ecode);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );
	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);

	ind += szCRC;

	// длина сообщения...
	mm.len = ind; // szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ErrorRetMessage& m )
{
	//    return mbPrintMessage(os,(ModbusByte*)(&m),sizeof(m));
	return os << "addr=" << addr2str(m.addr)
		   << " func=" << (int)m.func
		   << " errcode=" << dat2str(m.ecode);
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ErrorRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
ReadCoilMessage::ReadCoilMessage( ModbusAddr a, ModbusData s, ModbusData c ):
	start(s),
	count(c)
{
	addr = a;
	func = fnReadCoilStatus;
}
// -------------------------------------------------------------------------
ModbusMessage ReadCoilMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ReadCoilMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(count) };

	int last = sizeof(d); // индекс в массиве данных ( байтовый массив!!! )

	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	mm.len = szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------
ReadCoilMessage::ReadCoilMessage( ModbusMessage& m )
{
	init(m);
}

// -------------------------------------------------------------------------
ReadCoilMessage& ReadCoilMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadCoilMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadCoilStatus );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len

	// переворачиваем слова
	start = SWAPSHORT(start);
	count = SWAPSHORT(count);
}

// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadCoilMessage& m )
{
	//    return mbPrintMessage(os,(ModbusByte*)(&m),sizeof(m));
	return os << "addr=" << addr2str(m.addr)
		   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
		   << " count=" << dat2str(m.count) << "(" << (int)(m.count) << ")";
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadCoilMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
DataBits::DataBits( std::string s ):
	b(s)
{

}
// -------------------------------------------------------------------------
DataBits::DataBits( ModbusByte ubyte )
{
	(*this) = ubyte;
}
// -------------------------------------------------------------------------
DataBits::DataBits()
{
	b.reset();
}
// -------------------------------------------------------------------------
DataBits::operator ModbusByte()
{
	return mbyte();
}
// -------------------------------------------------------------------------
ModbusByte DataBits::mbyte()
{
	ModbusByte ubyte = 0;

	for( unsigned int i = 0; i < b.size(); i++ )
	{
		if( b[i] )
			ubyte |= 1 << i;
	}

	return ubyte;
}
// -------------------------------------------------------------------------
const DataBits& DataBits::operator=( const ModbusByte& r )
{
	for( unsigned int i = 0; i < b.size(); i++ )
		b[i] = r & (1 << i);

	return (*this);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, DataBits& d )
{
	os << "[";

	for( int i = d.b.size() - 1; i >= 0; i-- )
		os << d.b[i];

	os << "]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, DataBits* d )
{
	return os << (*d);
}
// -------------------------------------------------------------------------
DataBits16::DataBits16( const std::string& s ):
	b(s)
{

}
// -------------------------------------------------------------------------
DataBits16::DataBits16( ModbusData d )
{
	(*this) = d;
}
// -------------------------------------------------------------------------
DataBits16::DataBits16()
{
	b.reset();
}
// -------------------------------------------------------------------------
DataBits16::operator ModbusData()
{
	return mdata();
}
// -------------------------------------------------------------------------
ModbusData DataBits16::mdata()
{
	ModbusData udata = 0;

	for( unsigned int i = 0; i < b.size(); i++ )
	{
		if( b[i] )
			udata |= 1 << i;
	}

	return std::move(udata);
}
// -------------------------------------------------------------------------
const DataBits16& DataBits16::operator=( const ModbusData& r )
{
	const size_t sz = b.size();

	for( unsigned int i = 0; i < sz; i++ )
		b[i] = r & (1 << i);

	return (*this);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, DataBits16& d )
{
	os << "[";

	for( int i = d.b.size() - 1; i >= 0; i-- )
		os << d.b[i];

	os << "]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, DataBits16* d )
{
	return os << (*d);
}
// -------------------------------------------------------------------------
ReadCoilRetMessage::ReadCoilRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ReadCoilRetMessage& ReadCoilRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadCoilRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadCoilStatus );

	memset(this, 0, sizeof(*this));
	addr = m.addr;
	func = m.func;

	bcnt = m.data[0];

	if( bcnt > MAXPDULEN )
		throw mbException(erPacketTooLong);

	memcpy(&data, &(m.data[1]), bcnt);
	memcpy(&crc, &(m.data[bcnt + 1]), szCRC);
}
// -------------------------------------------------------------------------
int ReadCoilRetMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return m.data[0];
}
// -------------------------------------------------------------------------
ReadCoilRetMessage::ReadCoilRetMessage( ModbusAddr _addr ):
	bcnt(0)
{
	addr = _addr;
	func = fnReadCoilStatus;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool ReadCoilRetMessage::setBit( unsigned char dnum, unsigned char bnum, bool state )
{
	if( dnum < bcnt && bnum < BitsPerByte )
	{
		DataBits d(data[dnum]);
		d.b[bnum] = state;
		data[dnum] = d;
		return true;
	}

	return false;
}
// -------------------------------------------------------------------------
bool ReadCoilRetMessage::addData( DataBits d )
{
	if( isFull() )
		return false;

	data[bcnt++] = d.mbyte();
	return true;
}
// -------------------------------------------------------------------------
bool ReadCoilRetMessage::getData( unsigned char dnum, DataBits& d )
{
	if( dnum < bcnt )
	{
		d = data[dnum];
		return true;
	}

	return false;
}
// -------------------------------------------------------------------------
void ReadCoilRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage ReadCoilRetMessage::transport_msg()
{
	ModbusMessage mm;
	//    assert(sizeof(ModbusMessage)>=sizeof(ReadCoilRetMessage));
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	memcpy(&mm.data, &bcnt, sizeof(bcnt));
	int ind = sizeof(bcnt);

	// копируем данные
	memcpy(&(mm.data[ind]), data, bcnt);
	ind += bcnt;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(bcnt) + bcnt );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t ReadCoilRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + bcnt + szCRC;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadCoilRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadCoilRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
ReadInputStatusMessage::ReadInputStatusMessage( ModbusAddr a, ModbusData s, ModbusData c ):
	start(s),
	count(c)
{
	addr = a;
	func = fnReadInputStatus;
}
// -------------------------------------------------------------------------
ModbusMessage ReadInputStatusMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ReadInputStatusMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(count) };

	int last = sizeof(d); // индекс в массиве данных ( байтовый массив!!! )

	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	mm.len = szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------
ReadInputStatusMessage::ReadInputStatusMessage( ModbusMessage& m )
{
	init(m);
}

// -------------------------------------------------------------------------
ReadInputStatusMessage& ReadInputStatusMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadInputStatusMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadInputStatus );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len

	// переворачиваем слова
	start = SWAPSHORT(start);
	count = SWAPSHORT(count);

	if( count > MAXDATALEN )
		throw mbException(erPacketTooLong);
}

// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputStatusMessage& m )
{
	//    return mbPrintMessage(os,(ModbusByte*)(&m),sizeof(m));
	return os << "addr=" << addr2str(m.addr)
		   << " start=" << dat2str(m.start) << "(" << (int)m.start << ")"
		   << " count=" << dat2str(m.count) << "(" << (int)m.count << ")";
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputStatusMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
ReadInputStatusRetMessage::ReadInputStatusRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ReadInputStatusRetMessage& ReadInputStatusRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadInputStatusRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadInputStatus );

	memset(this, 0, sizeof(*this));
	addr = m.addr;
	func = m.func;

	bcnt = m.data[0];

	if( bcnt > MAXPDULEN )
		throw mbException(erPacketTooLong);

	memcpy(&data, &(m.data[1]), bcnt);
	memcpy(&crc, &(m.data[bcnt + 1]), szCRC);
}
// -------------------------------------------------------------------------
int ReadInputStatusRetMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return m.data[0];
}
// -------------------------------------------------------------------------
ReadInputStatusRetMessage::ReadInputStatusRetMessage( ModbusAddr _addr ):
	bcnt(0)
{
	addr = _addr;
	func = fnReadInputStatus;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool ReadInputStatusRetMessage::setBit( unsigned char dnum, unsigned char bnum, bool state )
{
	if( dnum < bcnt && bnum < BitsPerByte )
	{
		DataBits d(data[dnum]);
		d.b[bnum] = state;
		data[dnum] = d;
		return true;
	}

	return false;
}
// -------------------------------------------------------------------------
bool ReadInputStatusRetMessage::addData( DataBits d )
{
	if( isFull() )
		return false;

	data[bcnt++] = d.mbyte();
	return true;
}
// -------------------------------------------------------------------------
bool ReadInputStatusRetMessage::getData( unsigned char dnum, DataBits& d )
{
	if( dnum < bcnt )
	{
		d = data[dnum];
		return true;
	}

	return false;
}
// -------------------------------------------------------------------------
void ReadInputStatusRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage ReadInputStatusRetMessage::transport_msg()
{
	ModbusMessage mm;
	//    assert(sizeof(ModbusMessage)>=sizeof(ReadCoilRetMessage));
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	memcpy(&mm.data, &bcnt, sizeof(bcnt));
	int ind = sizeof(bcnt);

	// копируем данные
	memcpy(&(mm.data[ind]), data, bcnt);
	ind += bcnt;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(bcnt) + bcnt );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t ReadInputStatusRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + bcnt + szCRC;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputStatusRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputStatusRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
ReadOutputMessage::ReadOutputMessage( ModbusAddr a, ModbusData s, ModbusData c ):
	start(s),
	count(c)
{
	addr = a;
	func = fnReadOutputRegisters;
}
// -------------------------------------------------------------------------
ModbusMessage ReadOutputMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ReadOutputMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(count) };

	int last(sizeof(d)); // индекс в массиве данных ( байтовый массив!!! )

	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();

	return std::move(mm);
}
// -------------------------------------------------------------------------
ReadOutputMessage::ReadOutputMessage( ModbusMessage& m )
{
	init(m);
}

// -------------------------------------------------------------------------
ReadOutputMessage& ReadOutputMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadOutputMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadOutputRegisters );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len

	// переворачиваем слова
	start = SWAPSHORT(start);
	count = SWAPSHORT(count);

	if( count > MAXDATALEN )
		throw mbException(erPacketTooLong);
}

// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadOutputMessage& m )
{
	//    return mbPrintMessage(os,(ModbusByte*)(&m),sizeof(m));
	return os << "addr=" << addr2str(m.addr)
		   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
		   << " count=" << dat2str(m.count) << "(" << (int)m.count << ")";

}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadOutputMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
ReadOutputRetMessage::ReadOutputRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ReadOutputRetMessage& ReadOutputRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadOutputRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadOutputRegisters );

	memset(this, 0, sizeof(*this));
	addr = m.addr;
	func = m.func;

	// bcnt = m.data[0];
	unsigned int cnt = m.data[0] / sizeof(ModbusData);

	if( cnt > MAXLENPACKET / sizeof(ModbusData) )
	{
		//        cerr << "(ReadOutputRetMessage): BAD bcnt="
		//            << (int)bcnt << " count=" << cnt << endl;
		throw mbException(erPacketTooLong);
	}

	count     = cnt;
	bcnt     = m.data[0];
	memcpy(&data, &(m.data[1]), bcnt);

	// переворачиваем данные
	for( unsigned int i = 0; i < cnt; i++ )
		data[i] = SWAPSHORT(data[i]);

	memcpy(&crc, &(m.data[bcnt + 1]), szCRC);
}
// -------------------------------------------------------------------------
int ReadOutputRetMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return m.data[0];
	/*
	    ReadOutputMessage rm(m);
	    return (int)(rm.bcnt);
	*/
}
// -------------------------------------------------------------------------
ReadOutputRetMessage::ReadOutputRetMessage( ModbusAddr _addr ):
	bcnt(0),
	count(0)
{
	addr = _addr;
	func = fnReadOutputRegisters;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool ReadOutputRetMessage::addData( ModbusData d )
{
	if( isFull() )
		return false;

	data[count++] = d;
	bcnt = count * sizeof(ModbusData);
	return true;
}
// -------------------------------------------------------------------------
void ReadOutputRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	count    = 0;
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage ReadOutputRetMessage::transport_msg()
{
	ModbusMessage mm;
	//    assert(sizeof(ModbusMessage)>=sizeof(ReadOutputRetMessage));
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;
	bcnt    = count * sizeof(ModbusData);

	// copy bcnt
	memcpy(&mm.data, &bcnt, sizeof(bcnt));
	ind += sizeof(bcnt);

	// копируем данные
	//    int dlen = count*sizeof(ModbusData);
	//    прямое копирование
	//    memcpy(&(mm.data[sizeof(bcnt)]),data,dlen);

	// Создаём временно массив, переворачиваем байты
	ModbusData* dtmp = new ModbusData[count];

	for( ssize_t i = 0; i < count; i++ )
		dtmp[i] = SWAPSHORT(data[i]);

	// копируем
	memcpy(&(mm.data[ind]), dtmp, bcnt);

	delete[] dtmp;

	ind += bcnt;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(bcnt) + bcnt );

	//    crc = SWAPSHORT(crc);

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;

	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t ReadOutputRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + count * sizeof(ModbusData) + szCRC;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadOutputRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadOutputRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
ReadInputMessage::ReadInputMessage( ModbusAddr a, ModbusData s, ModbusData c ):
	start(s),
	count(c)
{
	addr = a;
	func = fnReadInputRegisters;
}
// -------------------------------------------------------------------------
ModbusMessage ReadInputMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ReadInputMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(count) };

	int last(sizeof(d)); // индекс в массиве данных ( байтовый массив!!! )

	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------
ReadInputMessage::ReadInputMessage( ModbusMessage& m )
{
	init(m);
}

// -------------------------------------------------------------------------
ReadInputMessage& ReadInputMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadInputMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadInputRegisters );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len

	// переворачиваем слова
	start = SWAPSHORT(start);
	count = SWAPSHORT(count);

	if( count > MAXDATALEN )
		throw mbException(erPacketTooLong);
}

// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputMessage& m )
{
	//    return mbPrintMessage(os,(ModbusByte*)(&m),sizeof(m));
	return os << "addr=" << addr2str(m.addr)
		   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
		   << " count=" << dat2str(m.count)
		   << "(" << (int)m.count << ")";
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
ReadInputRetMessage::ReadInputRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ReadInputRetMessage& ReadInputRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ReadInputRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadInputRegisters );

	memset(this, 0, sizeof(*this));
	addr = m.addr;
	func = m.func;

	// bcnt = m.data[0];
	unsigned int cnt = m.data[0] / sizeof(ModbusData);

	if( cnt > MAXLENPACKET / sizeof(ModbusData) )
		throw mbException(erPacketTooLong);

	count     = cnt;
	bcnt     = m.data[0];
	memcpy(&data, &(m.data[1]), bcnt);

	// переворачиваем данные
	swapData();

	memcpy(&crc, &(m.data[bcnt + 1]), szCRC);
}
// -------------------------------------------------------------------------
void ReadInputRetMessage::swapData()
{
	// переворачиваем данные
	for( ssize_t i = 0; i < count; i++ )
		data[i] = SWAPSHORT(data[i]);
}
// -------------------------------------------------------------------------
int ReadInputRetMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return m.data[0];
}
// -------------------------------------------------------------------------
ReadInputRetMessage::ReadInputRetMessage( ModbusAddr _addr ):
	bcnt(0),
	count(0)
{
	addr = _addr;
	func = fnReadInputRegisters;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool ReadInputRetMessage::addData( ModbusData d )
{
	if( isFull() )
		return false;

	data[count++] = d;
	bcnt = count * sizeof(ModbusData);
	return true;
}
// -------------------------------------------------------------------------
void ReadInputRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	count    = 0;
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage ReadInputRetMessage::transport_msg()
{
	ModbusMessage mm;
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;
	bcnt    = count * sizeof(ModbusData);

	// copy bcnt
	memcpy(&mm.data, &bcnt, sizeof(bcnt));
	ind += sizeof(bcnt);

	// Создаём временно массив, переворачиваем байты
	ModbusData* dtmp = new ModbusData[count];

	for( ssize_t i = 0; i < count; i++ )
		dtmp[i] = SWAPSHORT(data[i]);

	// копируем
	memcpy(&(mm.data[ind]), dtmp, bcnt);

	delete[] dtmp;

	ind += bcnt;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(bcnt) + bcnt );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t ReadInputRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + count * sizeof(ModbusData) + szCRC;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadInputRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
ForceCoilsMessage::ForceCoilsMessage( ModbusAddr a, ModbusData s ):
	start(s),
	quant(0),
	bcnt(0)
{
	addr = a;
	func = fnForceMultipleCoils;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool ForceCoilsMessage::addData( DataBits d )
{
	if( isFull() )
		return false;

	data[bcnt++] = d.mbyte();
	quant += BitsPerByte;
	return true;
}
// -------------------------------------------------------------------------
int ForceCoilsMessage::addBit( bool state )
{
	int qnum = quant % BitsPerByte;

	if( qnum == 0 )
		bcnt++;

	DataBits b(data[bcnt - 1]);
	b.b[qnum] = state;
	data[bcnt - 1] = b.mbyte();
	quant++;
	return (quant - 1);
}
// -------------------------------------------------------------------------
bool ForceCoilsMessage::setBit( int nbit, bool state )
{
	if( nbit < 0 || nbit >= quant )
		return false;

	int bnum = nbit / BitsPerByte;
	int qnum = nbit % BitsPerByte;

	DataBits b(data[bnum]);
	b.b[qnum] = state;
	data[bnum] = b.mbyte();
	return true;
}
// -------------------------------------------------------------------------
bool ForceCoilsMessage::getData( unsigned char dnum, DataBits& d )
{
	if( dnum < bcnt )
	{
		d = data[dnum];
		return true;
	}

	return false;
}
// -------------------------------------------------------------------------
void ForceCoilsMessage::clear()
{
	memset(data, 0, sizeof(data));
	start    = 0;
	quant    = 0;
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage ForceCoilsMessage::transport_msg()
{
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );
	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;

	// данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(quant) };
	ind += sizeof(d);

	// копируем
	memcpy(mm.data, &d, ind);

	// copy bcnt
	memcpy(&(mm.data[ind]), &bcnt, sizeof(bcnt));
	ind += sizeof(bcnt);

	// копируем данные
	memcpy(&(mm.data[ind]), data, bcnt);

	ind += bcnt;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------

ForceCoilsMessage::ForceCoilsMessage( ModbusMessage& m )
{
	init(m);
}

ForceCoilsMessage& ForceCoilsMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ForceCoilsMessage::init( ModbusMessage& m )
{
	assert( m.func == fnForceMultipleCoils );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	// Сперва переворачиваем обратно слова
	start = SWAPSHORT(start);
	quant = SWAPSHORT(quant);

	// потом проверяем
	if( !checkFormat() )
	{
#ifdef DEBUG
		cerr << "(ForceCoilsMessage): BAD format!" << endl;
		cerr << "bcnt=" << (int)bcnt
			 << " quant=" << (int)quant
			 << endl;
#endif
		// Если данные не корректны
		// чистим сообщение (в безопасные значения)
		//        start=0;
		//        quant=0; // это нельзя обнулять (иначе данные станут корректны!!!)
		bcnt = 0;
		memset(data, 0, sizeof(data));
	}

	// последний элемент это CRC
	memcpy(&crc, &(m.data[m.len - szCRC]), szCRC);
}
// -------------------------------------------------------------------------
bool ForceCoilsMessage::checkFormat()
{
	return ( func == fnForceMultipleCoils );
}
// -------------------------------------------------------------------------
size_t ForceCoilsMessage::szData()
{
	return szHead() + bcnt + szCRC;
}
// -------------------------------------------------------------------------
int ForceCoilsMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	ForceCoilsMessage wm(m);
	return (int)(wm.bcnt);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceCoilsMessage& m )
{
	os << "addr=" << addr2str(m.addr)
	   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
	   << " quant=" << dat2str(m.quant) << "(" << (int)(m.quant) << ")"
	   << " bcnt=" << b2str(m.bcnt)
	   << " data[" << (int)m.quant << "]={ ";

	for( ssize_t i = 0; i < m.bcnt; i++ )
	{
		DataBits d(m.data[i]);
		os << "" << d << "  ";
	}

	os << "}";
	return os;
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceCoilsMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
ForceCoilsRetMessage::ForceCoilsRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ForceCoilsRetMessage& ForceCoilsRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ForceCoilsRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnForceMultipleCoils );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	/*! \todo (WriteOutputRetMessage): необходимо встроить проверку на корректность данных */

	// Сперва переворачиваем обратно слова
	start = SWAPSHORT(start);
	quant = SWAPSHORT(quant);

	int ind = sizeof(quant) + sizeof(start);

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&crc, &(m.data[ind]), szCRC);
}
// -------------------------------------------------------------------------
ForceCoilsRetMessage::ForceCoilsRetMessage( ModbusAddr _from,
		ModbusData s, ModbusData q )
{
	addr     = _from;
	func     = fnForceMultipleCoils;
	start     = s;
	quant     = q;
}
// -------------------------------------------------------------------------
void ForceCoilsRetMessage::set( ModbusData s, ModbusData q )
{
	start     = s;
	quant    = q;
}
// -------------------------------------------------------------------------
ModbusMessage ForceCoilsRetMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ForceCoilsRetMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(quant) };
	int last(sizeof(d));
	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + last );
	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();

	return std::move(mm);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceCoilsRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), sizeof(m));
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceCoilsRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
WriteOutputMessage::WriteOutputMessage( ModbusAddr a, ModbusData s ):
	start(s),
	quant(0),
	bcnt(0)
{
	addr = a;
	func = fnWriteOutputRegisters;
}
// -------------------------------------------------------------------------
bool WriteOutputMessage::addData( ModbusData d )
{
	if( isFull() )
		return false;

	data[quant++] = d;
	bcnt = quant * sizeof(ModbusData);
	return true;
}
// -------------------------------------------------------------------------
void WriteOutputMessage::clear()
{
	memset(data, 0, sizeof(data));
	start    = 0;
	quant    = 0;
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage WriteOutputMessage::transport_msg()
{
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );
	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;

	// данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(quant) };
	ind += sizeof(d);

	// копируем
	memcpy(mm.data, &d, ind);

	// copy bcnt
	bcnt    = quant * sizeof(ModbusData);
	memcpy(&(mm.data[ind]), &bcnt, sizeof(bcnt));
	ind += sizeof(bcnt);

	// Создаём временно массив, переворачиваем байты
	ModbusData* dtmp = new ModbusData[quant];

	for( ssize_t i = 0; i < quant; i++ )
		dtmp[i] = SWAPSHORT(data[i]);

	// копируем данные
	memcpy(&(mm.data[ind]), dtmp, bcnt);
	delete[] dtmp;

	ind += bcnt;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------

WriteOutputMessage::WriteOutputMessage( ModbusMessage& m )
{
	init(m);
}

WriteOutputMessage& WriteOutputMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void WriteOutputMessage::init( ModbusMessage& m )
{
	assert( m.func == fnWriteOutputRegisters );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	// Сперва переворачиваем обратно слова
	start = SWAPSHORT(start);
	quant = SWAPSHORT(quant);

	// потом проверяем
	if( !checkFormat() )
	{
#ifdef DEBUG
		cerr << "(WriteOutputMessage): BAD format!" << endl;
		cerr << "bcnt=" << (int)bcnt
			 << " quant=" << (int)quant
			 << endl;
#endif
		// Если данные не корректны
		// чистим сообщение (в безопасные значения)
		//        start=0;
		//        quant=0; // это нельзя обнулять (иначе данные станут корректны!!!)
		bcnt = 0;
		memset(data, 0, sizeof(data));
	}

	// последний элемент это CRC
	memcpy(&crc, &(m.data[m.len - szCRC]), szCRC);

	int count( bcnt / sizeof(ModbusData) );

	for( ssize_t i = 0; i < count; i++ )
		data[i] = SWAPSHORT(data[i]);
}
// -------------------------------------------------------------------------
bool WriteOutputMessage::checkFormat()
{
	// return ( quant*sizeof(ModbusData) == bcnt ) && ( func == fnWriteOutputRegisters );
	return ( (bcnt == (quant * sizeof(ModbusData))) && (func == fnWriteOutputRegisters) );
}
// -------------------------------------------------------------------------
size_t WriteOutputMessage::szData()
{
	return szHead() + bcnt + szCRC;
}
// -------------------------------------------------------------------------
int WriteOutputMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	// копируем только часть заголовка возвращаем count
	// считается, что в ModbusMessage необходимая часть уже получена...
	//    memcpy(&m,&wm,szModbusHeader+szHead());

	WriteOutputMessage wm(m); // может просто смотреть m.data[0] ?!

	//#warning Может ли быть адрес нулевым или отрицательным?!
	//    assert( wm.start > 0 ); // ???

	return (int)(wm.bcnt);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteOutputMessage& m )
{
	//  вывод потока байт (с неперевёрнутыми словами)
	//    mbPrintMessage(os,(ModbusByte*)(&m), szModbusHeader + m.szData() - szCRC );
	//    return mbPrintMessage(os,(ModbusByte*)(&m.crc), szCRC );

	//    интелектуальный вывод :)
	os << "addr=" << addr2str(m.addr)
	   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
	   << " quant=" << dat2str(m.quant) << "(" << (int)(m.quant) << ")"
	   << " bcnt=" << dat2str(m.bcnt)
	   << " data[" << (int)m.quant << "]={ ";

	for( ssize_t i = 0; i < m.quant; i++ )
		os << "" << dat2str(m.data[i]) << "  ";

	os << "}";
	return os;
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteOutputMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
WriteOutputRetMessage::WriteOutputRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
WriteOutputRetMessage& WriteOutputRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void WriteOutputRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnWriteOutputRegisters );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	/*! \todo (WriteOutputRetMessage): необходимо встроить проверку на корректность данных */

	// Сперва переворачиваем обратно слова
	start = SWAPSHORT(start);
	quant = SWAPSHORT(quant);

	int ind = sizeof(quant) + sizeof(start);

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&crc, &(m.data[ind]), szCRC);
}
// -------------------------------------------------------------------------
WriteOutputRetMessage::WriteOutputRetMessage( ModbusAddr _from,
		ModbusData s, ModbusData q )
{
	addr     = _from;
	func     = fnWriteOutputRegisters;
	start     = s;
	quant     = q;
}
// -------------------------------------------------------------------------
void WriteOutputRetMessage::set( ModbusData s, ModbusData q )
{
	start     = s;
	quant    = q;
}
// -------------------------------------------------------------------------
ModbusMessage WriteOutputRetMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(WriteOutputRetMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(quant) };
	int last(sizeof(d));
	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + last );
	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();

	return std::move(mm);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteOutputRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), sizeof(m));
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteOutputRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
ForceSingleCoilMessage::ForceSingleCoilMessage( ModbusAddr a, ModbusData r , bool cmd ):
	start(r)
{
	addr = a;
	func = fnForceSingleCoil;
	data = cmd ? 0xFF00 : 0x0000;
}
// --------------------------------------------------------------------------------
ModbusMessage ForceSingleCoilMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ForceSingleCoilMessage));

	ModbusMessage mm;
	memcpy(&mm, this, szModbusHeader);
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(data) };
	int last = sizeof(d); // индекс в массиве данных ( байтовый массив!!! )
	memcpy(mm.data, &d, last);
	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + last );
	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);
	mm.len = szData();
	return std::move(mm);
}
// --------------------------------------------------------------------------------

ForceSingleCoilMessage::ForceSingleCoilMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ForceSingleCoilMessage& ForceSingleCoilMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ForceSingleCoilMessage::init( ModbusMessage& m )
{
	assert( m.func == fnForceSingleCoil );
	memset(this, 0, sizeof(*this));

	// копируем данные вместе с CRC
	memcpy(this, &m, szModbusHeader + m.len + szCRC);

	// Сперва переворачиваем обратно слова
	start     = SWAPSHORT(start);
	data     = SWAPSHORT(data);

	// потом проверяем
	if( !checkFormat() )
	{
#ifdef DEBUG
		cerr << "(ForceSingleCoil): BAD format!" << endl;
#endif
		// Если собщение некорректно
		// чистим сообщение (в безопасные значения)
		data = 0;
	}
}

// -------------------------------------------------------------------------
bool ForceSingleCoilMessage::checkFormat()
{
	return (func == fnForceSingleCoil);
}
// -------------------------------------------------------------------------
size_t ForceSingleCoilMessage::szData()
{
	return szHead() + sizeof(ModbusData) + szCRC;
}
// -------------------------------------------------------------------------
int ForceSingleCoilMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return sizeof(ModbusData); // data;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceSingleCoilMessage& m )
{
	return os << "addr=" << addr2str(m.addr)
		   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
		   << " data=" << dat2str(m.data) << "(" << (int)(m.data) << ")"
		   << "  ";
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceSingleCoilMessage* m )
{
	return os << (*m);
}

// -------------------------------------------------------------------------
ForceSingleCoilRetMessage::ForceSingleCoilRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ForceSingleCoilRetMessage& ForceSingleCoilRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void ForceSingleCoilRetMessage::init( ModbusMessage& m )
{
	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	/*! \todo (ForceSingleCoilRetMessage): необходимо встроить проверку на корректность данных */

	// переворачиваем обратно слова
	start     = SWAPSHORT(start);
	data     = SWAPSHORT(data);
}
// -------------------------------------------------------------------------
ForceSingleCoilRetMessage::ForceSingleCoilRetMessage( ModbusAddr _from )
{
	addr     = _from;
	func     = fnForceSingleCoil;
}

// -------------------------------------------------------------------------
void ForceSingleCoilRetMessage::set( ModbusData s, bool cmd )
{
	start     = s;
	data     = cmd ? 0xFF00 : 0x0000;
}

// -------------------------------------------------------------------------
ModbusMessage ForceSingleCoilRetMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(ForceSingleCoilRetMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(data) };

	int last(sizeof(d)); // индекс в массиве данных ( байтовый массив!!! )

	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();

	return std::move(mm);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceSingleCoilRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), sizeof(m));
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, ForceSingleCoilRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------

WriteSingleOutputMessage::WriteSingleOutputMessage( ModbusAddr a, ModbusData r , ModbusData d ):
	start(r),
	data(d)
{
	addr = a;
	func = fnWriteOutputSingleRegister;
}
// --------------------------------------------------------------------------------
ModbusMessage WriteSingleOutputMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(WriteSingleOutputMessage));

	ModbusMessage mm;
	memcpy(&mm, this, szModbusHeader);
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(data) };
	int last = sizeof(d); // индекс в массиве данных ( байтовый массив!!! )
	memcpy(mm.data, &d, last);
	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + last );
	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);
	mm.len = szData();
	return std::move(mm);
}
// --------------------------------------------------------------------------------

WriteSingleOutputMessage::WriteSingleOutputMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
WriteSingleOutputMessage& WriteSingleOutputMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void WriteSingleOutputMessage::init( ModbusMessage& m )
{
	assert( m.func == fnWriteOutputSingleRegister );
	memset(this, 0, sizeof(*this));

	// копируем данные вместе с CRC
	memcpy(this, &m, szModbusHeader + m.len + szCRC);

	// Сперва переворачиваем обратно слова
	start     = SWAPSHORT(start);
	data     = SWAPSHORT(data);

	// потом проверяем
	if( !checkFormat() )
	{
#ifdef DEBUG
		cerr << "(WriteSingleOutputMessage): BAD format!" << endl;
#endif
		// Если собщение некорректно
		// чистим сообщение (в безопасные значения)
		data = 0;
	}
}

// -------------------------------------------------------------------------
bool WriteSingleOutputMessage::checkFormat()
{
	// return ( quant*sizeof(ModbusData) == bcnt ) && ( func == fnWriteOutputRegisters );
	return ( (func == fnWriteOutputSingleRegister) );
}
// -------------------------------------------------------------------------
size_t WriteSingleOutputMessage::szData()
{
	return szHead() + sizeof(ModbusData) + szCRC;
}
// -------------------------------------------------------------------------
int WriteSingleOutputMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return sizeof(ModbusData); // data;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteSingleOutputMessage& m )
{
	//  вывод потока байт (с неперевёрнутыми словами)
	//    mbPrintMessage(os,(ModbusByte*)(&m), szModbusHeader + m.szData() - szCRC );
	//    return mbPrintMessage(os,(ModbusByte*)(&m.crc), szCRC );

	//    интелектуальный вывод :)
	return os << "addr=" << addr2str(m.addr)
		   << " start=" << dat2str(m.start) << "(" << (int)(m.start) << ")"
		   << " data=" << dat2str(m.data) << "(" << (int)(m.data) << ")"
		   << "  ";
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteSingleOutputMessage* m )
{
	return os << (*m);
}

// -------------------------------------------------------------------------
WriteSingleOutputRetMessage::WriteSingleOutputRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
WriteSingleOutputRetMessage& WriteSingleOutputRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void WriteSingleOutputRetMessage::init( ModbusMessage& m )
{
	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	/*! \todo (WriteSingleOutputRetMessage): необходимо встроить проверку на корректность данных */

	// переворачиваем обратно слова
	start     = SWAPSHORT(start);
	data     = SWAPSHORT(data);
}
// -------------------------------------------------------------------------
WriteSingleOutputRetMessage::WriteSingleOutputRetMessage( ModbusAddr _from, ModbusData s )
{
	addr     = _from;
	func     = fnWriteOutputSingleRegister;
	start     = s;
}

// -------------------------------------------------------------------------
void WriteSingleOutputRetMessage::set( ModbusData s, ModbusData d )
{
	start     = s;
	data    = d;
}

// -------------------------------------------------------------------------
ModbusMessage WriteSingleOutputRetMessage::transport_msg()
{
	assert(sizeof(ModbusMessage) >= sizeof(WriteSingleOutputRetMessage));

	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(start), SWAPSHORT(data) };

	int last(sizeof(d)); // индекс в массиве данных ( байтовый массив!!! )

	// копируем
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();

	return std::move(mm);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteSingleOutputRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), sizeof(m));
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, WriteSingleOutputRetMessage* m )
{
	return os << (*m);
}

// -------------------------------------------------------------------------
int ModbusRTU::szRequestDiagnosticData( DiagnosticsSubFunction f )
{
	if( f == subEcho )
		return 1;     // тут странно, вроде в стандарте количество динамическое

	// но везде вроде в примерах.. "одно слово"..

	if( f == dgRestartComm )
		return 1;

	if( f == dgDiagReg )
		return 1;

	if( f == dgChangeASCII )
		return 1;

	if( f == dgForceListen )
		return 1;

	if( f == dgClearCounters)
		return 1;

	if( f == dgBusMsgCount )
		return 1;

	if( f == dgBusErrCount )
		return 1;

	if( f == dgBusExceptCount )
		return 1;

	if( f == dgMsgSlaveCount )
		return 1;

	if( f == dgNoNoResponseCount )
		return 1;

	if( f == dgSlaveNAKCount )
		return 1;

	if( f == dgSlaveBusyCount )
		return 1;

	if( f == dgBusCharOverrunCount )
		return 1;

	if( f == dgClearOverrunCounter )
		return 1;

	return -1;
}
// -------------------------------------------------------------------------
DiagnosticMessage::DiagnosticMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
DiagnosticMessage& DiagnosticMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void DiagnosticMessage::init( ModbusMessage& m )
{
	assert( m.func == fnDiagnostics );

	memset(this, 0, sizeof(*this));
	addr = m.addr;
	func = m.func;

	memcpy( &subf, &(m.data[0]), sizeof(subf) );
	int last = sizeof(subf);

	subf =     SWAPSHORT(subf);
	count = szRequestDiagnosticData((DiagnosticsSubFunction)subf );

	if( count > MAXDATALEN )
		throw mbException(erPacketTooLong);

	if( count < 0 )
		throw mbException(erBadDataValue);

	memcpy(&data, &(m.data[last]), sizeof(ModbusData)*count);
	last += sizeof(ModbusData) * count;

	// переворачиваем данные
	for( ssize_t i = 0; i < count; i++ )
		data[i] = SWAPSHORT(data[i]);

	memcpy(&crc, &(m.data[last]), szCRC);
}
// -------------------------------------------------------------------------
int DiagnosticMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	// data[0] = subfunction
	ModbusData sf;
	memcpy( &sf, &(m.data[0]), sizeof(sf) );
	sf = SWAPSHORT(sf);

	int sz = szRequestDiagnosticData( (DiagnosticsSubFunction)sf );

	if( sz >= 0 )
		return sz * sizeof(ModbusData);

	return    0;
}
// -------------------------------------------------------------------------
DiagnosticMessage::DiagnosticMessage( ModbusAddr _addr, DiagnosticsSubFunction sf, ModbusData d ):
	count(0)
{
	addr = _addr;
	func = fnDiagnostics;
	subf = sf;
	memset(data, 0, sizeof(data));
	addData(d);
}
// -------------------------------------------------------------------------
bool DiagnosticMessage::addData( ModbusData d )
{
	if( isFull() )
		return false;

	data[count++] = d;
	return true;
}
// -------------------------------------------------------------------------
void DiagnosticMessage::clear()
{
	memset(data, 0, sizeof(data));
	count    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage DiagnosticMessage::transport_msg()
{
	ModbusMessage mm;
	//    assert(sizeof(ModbusMessage)>=sizeof(DiagnosticMessage));
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;
	// copy bcnt
	ModbusData d = SWAPSHORT(subf);
	memcpy(&mm.data, &d, sizeof(d));
	ind += sizeof(subf);

	// Создаём временно массив, переворачиваем байты
	ModbusData* dtmp = new ModbusData[count];

	for( ssize_t i = 0; i < count; i++ )
		dtmp[i] = SWAPSHORT(data[i]);

	// копируем
	memcpy(&(mm.data[ind]), dtmp, sizeof(ModbusData)*count);

	delete[] dtmp;

	ind += sizeof(ModbusData) * count;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(subf) + sizeof(ModbusData) * count );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t DiagnosticMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(subf) + count * sizeof(ModbusData) + szCRC;
}
// -------------------------------------------------------------------------
DiagnosticRetMessage::DiagnosticRetMessage( ModbusMessage& m ):
	DiagnosticMessage(m)
{
}
// -------------------------------------------------------------------------
DiagnosticRetMessage::DiagnosticRetMessage( DiagnosticMessage& m ):
	DiagnosticMessage(m)
{

}
// -------------------------------------------------------------------------
DiagnosticRetMessage::DiagnosticRetMessage( ModbusAddr a, DiagnosticsSubFunction subf, ModbusData d ):
	DiagnosticMessage(a, subf, d)
{
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, DiagnosticMessage& m )
{
	//    return mbPrintMessage(os,(ModbusByte*)(&m), szModbusHeader + m.szData() );
	os << "addr=" << addr2str(m.addr)
	   << " subf=" << dat2str(m.subf)
	   << " data[" << m.count << "]={";

	for( ssize_t i = 0; i < m.count; i++ )
		os << dat2str(m.data[i]) << "  ";

	os << "}";

	return os;
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, DiagnosticMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, DiagnosticRetMessage& m )
{
	//return mbPrintMessage(os,(ModbusByte*)(&m),sizeof(m));
	os << "addr=" << addr2str(m.addr)
	   << " subf=" << dat2str(m.subf)
	   << " data[" << m.count << "]={";

	for( ssize_t i = 0; i < m.count; i++ )
		os << dat2str(m.data[i]) << "  ";

	os << "}";

	return os;
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, DiagnosticRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
MEIMessageRDI::MEIMessageRDI( ModbusAddr a, ModbusByte dev, ModbusByte oid ):
	type(0x0E),
	devID(dev),
	objID(oid)
{
	addr = a;
	func = fnMEI;
}
// -------------------------------------------------------------------------
ModbusMessage MEIMessageRDI::transport_msg()
{
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );
	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);
	mm.data[0] = type;
	mm.data[1] = devID;
	mm.data[2] = objID;
	int ind = 3;

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------

MEIMessageRDI::MEIMessageRDI( ModbusMessage& m )
{
	init(m);
}

MEIMessageRDI& MEIMessageRDI::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void MEIMessageRDI::init( ModbusMessage& m )
{
	assert( m.func == fnMEI );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	// потом проверяем
	if( !checkFormat() )
	{
#ifdef DEBUG
		cerr << "(MEIMessageRDI): BAD format!" << endl;
		cerr << "MEI type != 0x0E (read '" << type << "')"
			 << endl;
#endif
	}

	// последний элемент это CRC
	memcpy(&crc, &(m.data[m.len - szCRC]), szCRC);
}
// -------------------------------------------------------------------------
bool MEIMessageRDI::checkFormat()
{
	return ( type == 0x0E );
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, MEIMessageRDI& m )
{
	os << "addr=" << addr2str(m.addr)
	   << " func=0x" << b2str(m.func)
	   << " type=0x" << b2str(m.type)
	   << " devID=0x" << b2str(m.devID)
	   << " objID=0x" << b2str(m.objID);

	return os;
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, MEIMessageRDI* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
MEIMessageRetRDI::MEIMessageRetRDI( ModbusMessage& m ):
	type(0),
	devID(0),
	conformity(0),
	mf(0),
	objID(0),
	objNum(0),
	bcnt(0)
{
	init(m);
}
// -------------------------------------------------------------------------
MEIMessageRetRDI& MEIMessageRetRDI::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
RDIObjectInfo::RDIObjectInfo( ModbusByte id, ModbusByte* dat, ModbusByte len ):
	id(id),
	val("")
{
	val.reserve(len);

	for( ssize_t i = 0; i < len; i++ )
		val.push_back( (char)dat[i] );
}
// -------------------------------------------------------------------------
void MEIMessageRetRDI::pre_init( ModbusMessage& m )
{
	assert( m.func == fnMEI );

	addr = m.addr;
	func = m.func;

	type = m.data[0];

	if( type != 0x0E )
		throw mbException(erInvalidFormat);

	if( m.len < 6 )
		throw mbException(erInvalidFormat);

	devID = m.data[1];
	conformity = m.data[2];
	mf = m.data[3];
	objID = m.data[4];
	objNum = m.data[5];
}
// -------------------------------------------------------------------------
void MEIMessageRetRDI::init( ModbusMessage& m )
{
	assert( m.func == fnMEI );

	addr = m.addr;
	func = m.func;
	type = m.data[0];

	if( type != 0x0E )
		throw mbException(erInvalidFormat);

	if( m.len < 6 )
		throw mbException(erInvalidFormat);

	devID = m.data[1];
	conformity = m.data[2];
	mf = m.data[3];
	objID = m.data[4];
	objNum = m.data[5];

	bcnt = 0;
	dlist.clear();
	size_t i = 6;

	if( objNum > 0 )
	{
		if( m.len < 7 )
			throw mbException(erInvalidFormat);

		while( i < m.len && dlist.size() < objNum )
		{
			ModbusByte id = m.data[i];
			int dlen = (int)(m.data[i + 1]);

			if( m.len < (i + 1 + dlen) )
				throw mbException(erInvalidFormat);

			RDIObjectInfo rdi(id, &(m.data[i + 2]), dlen );
			dlist.push_back(rdi);
			bcnt += dlen + 2;

			i += (dlen + 2);
		}
	}

	memcpy(&crc, &(m.data[i]), szCRC);
}
// -------------------------------------------------------------------------
MEIMessageRetRDI::MEIMessageRetRDI():
	type(0x00),
	devID(0),
	conformity(0),
	mf(0),
	objID(0),
	objNum(0),
	bcnt(0)
{
	addr = 0;
	func = fnUnknown;
}
// -------------------------------------------------------------------------
MEIMessageRetRDI::MEIMessageRetRDI( ModbusAddr _addr, ModbusByte devID, ModbusByte conformity, ModbusByte mf, ModbusByte objID ):
	type(0x0E),
	devID(devID),
	conformity(conformity),
	mf(mf),
	objID(objID),
	objNum(0),
	bcnt(0)
{
	addr = _addr;
	func = fnMEI;
}
// -------------------------------------------------------------------------
bool MEIMessageRetRDI::addData( ModbusByte id, const std::string& val )
{
	if( isFull() )
		return false;

	RDIObjectInfo r(id, val);
	dlist.push_back(r);
	objNum = dlist.size();

	bcnt += val.size() + 2; // 2 = 'id'(byte) + 'len'(byte)
	return true;
}
// -------------------------------------------------------------------------
void MEIMessageRetRDI::clear()
{
	dlist.clear();
	bcnt = 0;
}
// -------------------------------------------------------------------------
ModbusMessage MEIMessageRetRDI::transport_msg()
{
	ModbusMessage mm;
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	mm.data[0] = type;
	mm.data[1] = devID;
	mm.data[2] = conformity;
	mm.data[3] = mf;
	mm.data[4] = objID;
	mm.data[5] = objNum;
	int ind = 6;

	for( auto it = dlist.begin(); it != dlist.end() && ind <= MAXLENPACKET; ++it )
	{
		mm.data[ind++] = it->id;
		int dlen = it->val.size(); // !! не копируем завершающий символ
		mm.data[ind++] = dlen;
		memcpy(&(mm.data[ind]), it->val.data(), dlen );
		ind += dlen;
	}

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t MEIMessageRetRDI::szData()
{
	// заголовочные поля + фактическое число данных + контрольная сумма
	return 6 + bcnt + szCRC;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, MEIMessageRetRDI& m )
{
	// return mbPrintMessage(os,(ModbusByte*)(&m), szModbusHeader + m.szData() );
	os << "addr=" << addr2str(m.addr)
	   << " func=0x" << b2str(m.func)
	   << " type=0x" << b2str(m.type)
	   << " devID=0x" << b2str(m.devID)
	   << " conformity=0x" << b2str(m.conformity)
	   << " mf=0x" << b2str(m.mf)
	   << " objID=0x" << b2str(m.objID)
	   << " objNum=" << (int)(m.objNum);

	if( !m.dlist.empty() )
	{
		os << endl;

		for( auto& it : m.dlist )
			os << "     " << rdi2str(it.id) << " : " << it.val << endl;
	}

	return os;
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, MEIMessageRetRDI* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, RDIObjectList& dlist )
{
	if( !dlist.empty() )
	{
		for( auto& it : dlist )
			os << "     " << rdi2str(it.id) << " : " << it.val << endl;
	}

	return os;
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, RDIObjectList* l )
{
	return os << (*l);
}
// -------------------------------------------------------------------------

JournalCommandMessage::JournalCommandMessage( ModbusMessage& m )
{
	assert( m.func == fnJournalCommand );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len

	// переворачиваем слова
	cmd = SWAPSHORT(cmd);
	num = SWAPSHORT(num);
}
// -------------------------------------------------------------------------
JournalCommandMessage& JournalCommandMessage::operator=( ModbusMessage& m )
{
	assert( m.func == fnJournalCommand );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len

	// переворачиваем слова
	cmd = SWAPSHORT(cmd);
	num = SWAPSHORT(num);

	return *this;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, JournalCommandMessage& m )
{
	return os << "num=" << (int)m.num << " cmd=" << (int)m.cmd;
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, JournalCommandMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------

JournalCommandRetMessage::JournalCommandRetMessage( ModbusAddr _addr ):
	bcnt(0),
	count(0)
{
	addr = _addr;
	func = fnJournalCommand;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool JournalCommandRetMessage::setData( ModbusByte* buf, int len )
{
	if( isFull() )
		return false;

	if( sizeof(ModbusByte)*len > sizeof(data) )
		return false;

	// стираем старые данные
	memset(data, 0, sizeof(data));
	// копируем
	memcpy( data, buf, len );

	count     = len / sizeof(ModbusData);

	// выравниваем до границы слова..
	if( len % sizeof(ModbusData) )
		count++;

	bcnt = count * sizeof(ModbusData);
	return true;
}
// -------------------------------------------------------------------------
void JournalCommandRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	count    = 0;
	bcnt    = 0;
}
// -------------------------------------------------------------------------
ModbusMessage JournalCommandRetMessage::transport_msg()
{
	ModbusMessage mm;
	//    assert(sizeof(ModbusMessage)>=sizeof(ReadOutputRetMessage));
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;
	bcnt    = count * sizeof(ModbusData);

	// copy bcnt
	memcpy(&mm.data, &bcnt, sizeof(bcnt));
	ind += sizeof(bcnt);

	// --------------------
	// копируем данные
	// --------------------
	// копирование с переворотом данных (для ModbusData)
	ModbusData* dtmp = new ModbusData[count];

	for( ssize_t i = 0; i < count; i++ )
		dtmp[i] = SWAPSHORT(data[i]);

	// копируем
	memcpy(&(mm.data[ind]), dtmp, bcnt);

	delete[] dtmp;

	ind += bcnt;

	// пересчитываем CRC по данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------
size_t JournalCommandRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + count * sizeof(ModbusData) + szCRC;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, JournalCommandRetMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, JournalCommandRetMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
JournalCommandRetOK::JournalCommandRetOK( ModbusAddr _from ):
	JournalCommandRetMessage(_from)
{
}
// -------------------------------------------------------------------------
void JournalCommandRetOK::set( ModbusData cmd, ModbusData ecode )
{
	set(*this, cmd, ecode);
}
// -------------------------------------------------------------------------
void JournalCommandRetOK::set( JournalCommandRetMessage& m,
							   ModbusData cmd, ModbusData ecode )
{
	m.data[0] = cmd;
	m.data[1] = ecode;
	m.count    = 2;
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, JournalCommandRetOK& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}
std::ostream& ModbusRTU::operator<<(std::ostream& os, JournalCommandRetOK* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
float ModbusRTU::dat2f( const ModbusData dat1, const ModbusData dat2 )
{
	ModbusData d[2] = {dat1, dat2};
	float f = 0;
	assert(sizeof(f) >= sizeof(d));
	memcpy(&f, d, sizeof(d));
	return f;
}
// -------------------------------------------------------------------------
ModbusAddr ModbusRTU::str2mbAddr( const std::string& val )
{
	if( val.empty() )
		return 0;

	return (ModbusAddr)UniSetTypes::uni_atoi(val);
}
// -------------------------------------------------------------------------
ModbusData ModbusRTU::str2mbData( const std::string& val )
{
	if( val.empty() )
		return 0;

	return (ModbusData)UniSetTypes::uni_atoi(val);
}
// -------------------------------------------------------------------------
std::string ModbusRTU::dat2str( const ModbusData dat )
{
	ostringstream s;
	s << hex << setfill('0') << showbase << dat;
	return s.str();
}
// -------------------------------------------------------------------------
std::string ModbusRTU::addr2str( const ModbusAddr addr )
{
	ostringstream s;
	s << "0x" << hex << setfill('0') << setw(2) << (unsigned short)addr;
	return s.str();

	//    ostringstream s;
	//    s << hex << setfill('0') << showbase << (int)addr;
	//    return s.str();
}
// -------------------------------------------------------------------------
std::string ModbusRTU::b2str( const ModbusByte b )
{
	ostringstream s;
	s << hex << setfill('0') << setw(2) << (unsigned short)b;
	return s.str();
}
// -------------------------------------------------------------------------

std::string ModbusRTU::mbErr2Str( ModbusRTU::mbErrCode e )
{
	switch( e )
	{
		case erNoError:
			return "";

		case erInvalidFormat:
			return "неправильный формат";

		case erBadCheckSum:
			return "У пакета не сошлась контрольная сумма";

		case erBadReplyNodeAddress:
			return "Ответ на запрос адресован не мне или от станции,которую не спрашивали";

		case erTimeOut:
			return "Тайм-аут при приеме";

		case erUnExpectedPacketType:
			return "Неожидаемый тип пакета";

		case erPacketTooLong:
			return "пакет длинее буфера приема";

		case erHardwareError:
			return "ошибка оборудования";

		case erBadDataAddress:
			return "регистр не существует или запрещён к опросу";

		case erBadDataValue:
			return "значение не входит в разрешённый диапазон";

		case erAnknowledge:
			return "запрос принят в исполнению, но ещё не выполнен";

		case erSlaveBusy:
			return "контроллер занят длительной операцией (повторить запрос позже)";

		case erOperationFailed:
			return "сбой при выполнении операции (например: доступ запрещён)";

		case erMemoryParityError:
			return "ошибка паритета при чтении памяти";

		default:
			return "Неизвестный код ошибки";
	}
}
// -------------------------------------------------------------------------
SetDateTimeMessage::SetDateTimeMessage( ModbusMessage& m )
{
	assert( m.func == fnSetDateTime );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len
}
// -------------------------------------------------------------------------
SetDateTimeMessage& SetDateTimeMessage::operator=( ModbusMessage& m )
{
	assert( m.func == fnSetDateTime );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len
	return *this;
}
// -------------------------------------------------------------------------
SetDateTimeMessage::SetDateTimeMessage()
{
	func = fnSetDateTime;
	memset(this, 0, sizeof(*this));
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, SetDateTimeMessage& m )
{
	ostringstream s;
	s << setfill('0')
	  << setw(2) << (int)m.day << "-"
	  << setw(2) << (int)m.mon << "-"
	  << setw(2) << (int)m.century
	  << setw(2) << (int)m.year << " "
	  << setw(2) << (int)m.hour << ":"
	  << setw(2) << (int)m.min << ":"
	  << setw(2) << (int)m.sec;

	return os << s.str();
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, SetDateTimeMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
bool SetDateTimeMessage::checkFormat()
{
	/*
	    // Lav: проверка >=0 бессмысленна, потому что в типе данных Modbusbyte не могут храниться отрицательные числа
	    return     ( hour>=0 && hour<=23 ) &&
	            ( min>=0 && min<=59 ) &&
	            ( sec>=0 && sec<=59 ) &&
	            ( day>=1 && day<=31 ) &&
	            ( mon>=1 && mon<=12 ) &&
	            ( year>=0 && year<=99 ) &&
	            ( century>=19 && century<=20 );
	*/
	return     ( hour <= 23 ) &&
			   ( min <= 59 ) &&
			   ( sec <= 59 ) &&
			   ( day >= 1 && day <= 31 ) &&
			   ( mon >= 1 && mon <= 12 ) &&
			   ( year <= 99 ) &&
			   ( century >= 19 && century <= 20 );
}
// -------------------------------------------------------------------------
SetDateTimeMessage::SetDateTimeMessage( ModbusAddr a )
{
	addr = a;
	func = fnSetDateTime;
}
// -------------------------------------------------------------------------
ModbusMessage SetDateTimeMessage::transport_msg()
{
	ModbusMessage mm;
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);
	/*
	    mm.data[0] = hour;
	    mm.data[1] = min;
	    mm.data[2] = sec;
	    mm.data[3] = day;
	    mm.data[4] = mon;
	    mm.data[5] = year;
	    mm.data[6] = century;
	*/
	int bcnt = 7;
	memcpy( mm.data, &hour, bcnt );

	// пересчитываем CRC
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + bcnt );

	memcpy(&(mm.data[bcnt]), &crc, szCRC);
	// длина сообщения...
	mm.len = szData(); // bcnt + szCRC
	return std::move(mm);
}
// -------------------------------------------------------------------------
SetDateTimeRetMessage::SetDateTimeRetMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
SetDateTimeRetMessage& SetDateTimeRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void SetDateTimeRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnSetDateTime );
	memset(this, 0, sizeof(*this));
	memcpy(this, &m, sizeof(*this)); // m.len
}
// -------------------------------------------------------------------------
SetDateTimeRetMessage::SetDateTimeRetMessage( ModbusAddr _from )
{
	addr = _from;
	func = fnSetDateTime;

	time_t tm = time(0);
	struct tm* tms = localtime(&tm);
	hour     = tms->tm_hour;
	min     = tms->tm_min;
	sec     = tms->tm_sec;
	day     = tms->tm_mday;
	mon        = tms->tm_mon + 1;
	year    = tms->tm_year;
	century = ( tms->tm_year + 1900 >= 2000 ) ? 20 : 19;
}
// -------------------------------------------------------------------------
SetDateTimeRetMessage::SetDateTimeRetMessage( const SetDateTimeMessage& query )
{
	memcpy(this, &query, sizeof(*this));
}
// -------------------------------------------------------------------------
void SetDateTimeRetMessage::cpy( SetDateTimeRetMessage& reply,
								 SetDateTimeMessage& query )
{
	reply = query;
}
// -----------------------------------------------------------------------
ModbusMessage SetDateTimeRetMessage::transport_msg()
{
	ModbusMessage mm;
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	/*
	    mm.data[0] = hour;
	    mm.data[1] = min;
	    mm.data[2] = sec;
	    mm.data[3] = day;
	    mm.data[4] = mon;
	    mm.data[5] = year;
	    mm.data[6] = century;
	*/
	int bcnt = 7;
	memcpy( mm.data, &hour, bcnt );

	// пересчитываем CRC
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + bcnt );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[bcnt]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData(); // bcnt + szCRC

	return std::move(mm);
}
// -------------------------------------------------------------------------
RemoteServiceMessage::RemoteServiceMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
RemoteServiceMessage& RemoteServiceMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
void RemoteServiceMessage::init( ModbusMessage& m )
{
	assert( m.func == fnRemoteService );
	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	// последний элемент это CRC
	memcpy(&crc, &(m.data[m.len - szCRC]), szCRC);
}
// -------------------------------------------------------------------------
size_t RemoteServiceMessage::szData()
{
	return szHead() + bcnt + szCRC;
}
// -------------------------------------------------------------------------
int RemoteServiceMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	//    RemoteServiceMessage wm(m);
	//    return (int)(wm.bcnt);

	return (int)(m.data[0]);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, RemoteServiceMessage& m )
{
	return mbPrintMessage(os, (ModbusByte*)(&m), szModbusHeader + m.szData() );
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, RemoteServiceMessage* m )
{
	return os << (*m);
}
// -------------------------------------------------------------------------
RemoteServiceRetMessage::RemoteServiceRetMessage( ModbusAddr _from ):
	bcnt(0),
	count(0)
{
	addr = _from;
	func = fnRemoteService;
	memset(data, 0, sizeof(data));
}
// -------------------------------------------------------------------------
bool RemoteServiceRetMessage::setData( ModbusByte* buf, int len )
{
	if( isFull() )
		return false;

	if( len * sizeof(ModbusByte) > sizeof(data) )
		return false;

	// стираем старые данные
	memset(data, 0, sizeof(data));

	// копируем
	memcpy(data, buf, len);

	bcnt    = len;
	return true;
}
// -------------------------------------------------------------------------
void RemoteServiceRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	count    = 0;
	bcnt    = 0;
}
// -------------------------------------------------------------------------
size_t RemoteServiceRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + count * sizeof(ModbusByte) + szCRC;
}
// -------------------------------------------------------------------------
ModbusMessage RemoteServiceRetMessage::transport_msg()
{
	ModbusMessage mm;
	assert( sizeof(ModbusMessage) >= (unsigned int)szModbusHeader + szData() );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;
	bcnt    = count * sizeof(ModbusByte);

	// copy bcnt
	mm.data[0] = bcnt;
	ind += sizeof(bcnt);

	// --------------------
	// копируем данные
	// --------------------
	memcpy(&(mm.data[1]), data, bcnt);
	ind += bcnt;

	// пересчитываем CRC по данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -------------------------------------------------------------------------
ReadFileRecordMessage::ReadFileRecordMessage( ModbusMessage& m )
{
	init(m);
}
// -------------------------------------------------------------------------
ReadFileRecordMessage& ReadFileRecordMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -------------------------------------------------------------------------
bool ReadFileRecordMessage::checkFormat()
{
	return ( bcnt >= 0x07 && bcnt <= 0xF5 );
}
// -------------------------------------------------------------------------
void ReadFileRecordMessage::init( ModbusMessage& m )
{
	assert( m.func == fnReadFileRecord );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	// потом проверяем
	if( !checkFormat() )
	{
#ifdef DEBUG
		cerr << "(ReadFileRecordMessage): BAD format!" << endl;
		cerr << "bcnt=" << (int)bcnt << endl;
#endif
		bcnt = 0;
		memset(data, 0, sizeof(data));
	}

	// последний элемент это CRC
	memcpy(&crc, &(m.data[m.len - szCRC]), szCRC);

	count = bcnt / sizeof(SubRequest);

	for( ssize_t i = 0; i < count; i++ )
	{
		data[i].numfile = SWAPSHORT(data[i].numfile);
		data[i].numrec = SWAPSHORT(data[i].numrec);
		data[i].reglen = SWAPSHORT(data[i].reglen);
	}
}
// -------------------------------------------------------------------------
size_t ReadFileRecordMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(bcnt) + count * sizeof(SubRequest) + szCRC;
}
// -------------------------------------------------------------------------
int ReadFileRecordMessage::getDataLen( ModbusMessage& m )
{
	if( m.len < 0 )
		return 0;

	return (int)(m.data[0]);

	//    ReadFileRecordMessage rfm(m); // может просто смотреть m.data[0] ?!
	//    return (int)(rfm.bcnt);
}
// -------------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadFileRecordMessage& m )
{
	return os << (&m);
}

std::ostream& ModbusRTU::operator<<(std::ostream& os, ReadFileRecordMessage* m )
{
	return mbPrintMessage(os, (ModbusByte*)m, szModbusHeader + m->szData() );
}
// -------------------------------------------------------------------------
FileTransferMessage::FileTransferMessage( ModbusAddr a, ModbusData nf, ModbusData np ):
	numfile(nf),
	numpacket(np)
{
	addr = a;
	func = fnFileTransfer;
}
// -------------------------------------------------------------------------
ModbusMessage FileTransferMessage::transport_msg()
{
	ModbusMessage mm;

	// копируем заголовок
	memcpy(&mm, this, szModbusHeader);

	// копируем данные (переворачиваем байты)
	ModbusData d[2] = { SWAPSHORT(numfile), SWAPSHORT(numpacket) };
	int last = sizeof(d);
	memcpy(mm.data, &d, last);

	// пересчитываем CRC по перевёрнутым данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + sizeof(d) );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[last]), &crc, szCRC);

	// длина сообщения...
	mm.len = szData();
	return std::move(mm);
}
// -------------------------------------------------------------------------
FileTransferMessage::FileTransferMessage( ModbusMessage& m )
{
	init(m);
}
// -----------------------------------------------------------------------
FileTransferMessage& FileTransferMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -----------------------------------------------------------------------
void FileTransferMessage::init( ModbusMessage& m )
{
	assert( m.func == fnFileTransfer );

	memset(this, 0, sizeof(*this));

	// copy not include CRC
	memcpy(this, &m, szModbusHeader + m.len);

	// последний элемент это CRC
	memcpy(&crc, &(m.data[m.len - szCRC]), szCRC);

	numfile    = SWAPSHORT(numfile);
	numpacket = SWAPSHORT(numpacket);
}
// -----------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, FileTransferMessage& m )
{
	return os << (&m);
}
// -----------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, FileTransferMessage* m )
{
	//    return mbPrintMessage(os,(ModbusByte*)m, szModbusHeader + m->szHead() );
	return os << " numfile=" << m->numfile
		   << " numpacket=" << m->numpacket;
}
// -----------------------------------------------------------------------
FileTransferRetMessage::FileTransferRetMessage( ModbusMessage& m )
{
	init(m);
}
// -----------------------------------------------------------------------
FileTransferRetMessage& FileTransferRetMessage::operator=( ModbusMessage& m )
{
	init(m);
	return *this;
}
// -----------------------------------------------------------------------
void FileTransferRetMessage::init( ModbusMessage& m )
{
	assert( m.func == fnFileTransfer );
	memset(this, 0, sizeof(*this));

	// copy header
	memcpy(this, &m, szModbusHeader);

	bcnt = m.data[0];
	memcpy(&numfile, &(m.data[1]), sizeof(ModbusData));
	memcpy(&numpacks, &(m.data[1 + sizeof(ModbusData)]), sizeof(ModbusData));
	memcpy(&packet, &(m.data[1 + 2 * sizeof(ModbusData)]), sizeof(ModbusData));
	numfile     = SWAPSHORT(numfile);
	numpacks     = SWAPSHORT(numpacks);
	packet         = SWAPSHORT(packet);
	dlen         = m.data[7];
	memcpy(data, &(m.data[8]), dlen);

	memcpy(&crc, &(m.data[8 + dlen]), szCRC);
}
// -----------------------------------------------------------------------
FileTransferRetMessage::FileTransferRetMessage( ModbusAddr _from ):
	numfile(0),
	numpacks(0),
	packet(0),
	dlen(0)
{
	addr = _from;
	func = fnFileTransfer;
	memset(data, 0, sizeof(data));
}
// -----------------------------------------------------------------------
bool FileTransferRetMessage::set( ModbusData nfile, ModbusData fpacks,
								  ModbusData pack, ModbusByte* buf, ModbusByte len )
{
	if( len > sizeof(data) )
		return false;

	clear();
	memcpy(data, buf, len);

	dlen         = len;
	numfile     = nfile;
	numpacks     = fpacks;
	packet         = pack;
	return true;
}
// -----------------------------------------------------------------------
void FileTransferRetMessage::clear()
{
	memset(data, 0, sizeof(data));
	dlen         = 0;
	numfile     = 0;
	numpacks     = 0;
	packet         = 0;
}
// -----------------------------------------------------------------------
int FileTransferRetMessage::getDataLen( ModbusMessage& m )
{
	return m.data[0];
}
// -----------------------------------------------------------------------
size_t FileTransferRetMessage::szData()
{
	// фактическое число данных + контрольная сумма
	return sizeof(ModbusByte) * 2 + sizeof(ModbusData) * 3 + dlen + szCRC;
}
// -----------------------------------------------------------------------
ModbusMessage FileTransferRetMessage::transport_msg()
{
	ModbusMessage mm;
	assert( sizeof(ModbusMessage) >= (unsigned int)(szModbusHeader + szData()) );

	// копируем заголовок и данные
	memcpy(&mm, this, szModbusHeader);

	int ind = 0;
	bcnt = szData() - szCRC - 1; // -1 - это сам байт содержащий количество байт (bcnt)...

	// copy bcnt
	mm.data[ind++] = bcnt;

	// копируем предварительный заголовок
	ModbusData dhead[] = { numfile, numpacks, packet };

	for( size_t i = 0; i < sizeof(dhead) / sizeof(ModbusData); i++ )
		dhead[i] = SWAPSHORT(dhead[i]);

	memcpy(&(mm.data[ind]), dhead, sizeof(dhead));
	ind += sizeof(dhead);

	mm.data[ind++] = dlen;

	// --------------------
	// копируем данные
	// --------------------
	memcpy(&(mm.data[ind]), data, dlen);
	ind += dlen;

	// пересчитываем CRC по данным
	ModbusData crc = checkCRC( (ModbusByte*)(&mm), szModbusHeader + ind );

	// копируем CRC (последний элемент). Без переворачивания...
	memcpy(&(mm.data[ind]), &crc, szCRC);
	ind += szCRC;

	// длина сообщения...
	mm.len = ind;
	return std::move(mm);
}
// -----------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, FileTransferRetMessage& m )
{
	return os << (&m);
}
// -----------------------------------------------------------------------
std::ostream& ModbusRTU::operator<<(std::ostream& os, FileTransferRetMessage* m )
{
	return mbPrintMessage(os, (ModbusByte*)m, szModbusHeader + m->szData() );
}
// -----------------------------------------------------------------------
std::ostream& ModbusTCP::operator<<(std::ostream& os, MBAPHeader& m )
{
	//    m.swapdata();
	for( size_t i = 0; i < sizeof(m); i++ )
		os << ((unsigned char*)(&m))[i];

	//    m.swapdata();
	return os;
}
// -----------------------------------------------------------------------
void ModbusTCP::MBAPHeader::swapdata()
{
	tID = SWAPSHORT(tID);
	pID = SWAPSHORT(pID);
	len = SWAPSHORT(len);
}
// -----------------------------------------------------------------------
std::string ModbusRTU::rdi2str( int id )
{
	if( id == rdiVendorName )
		return "VendorName";

	if( id == rdiProductCode )
		return "ProductName";

	if( id == rdiMajorMinorRevision )
		return "MajorMinorRevision";

	if( id == rdiVendorURL )
		return "VendorURL";

	if( id == rdiProductName )
		return "ProductName";

	if( id == rdiModelName )
		return "ModelName";

	if( id == rdiUserApplicationName )
		return "UserApplicationName";

	ostringstream s;
	s << id;
	return s.str();
}
// -----------------------------------------------------------------------
ModbusRTU::RegID ModbusRTU::genRegID( const ModbusRTU::ModbusData mbreg, const int fn )
{
	// диапазоны:
	// mbreg: 0..65535
	// fn: 0...255
	int max = numeric_limits<ModbusRTU::ModbusData>::max(); // по идее 65535
	int fn_max = numeric_limits<ModbusRTU::ModbusByte>::max(); // по идее 255

	// fn необходимо привести к диапазону 0..max
	return max + mbreg + max + UniSetTypes::lcalibrate(fn, 0, fn_max, 0, max, false);
}
// -----------------------------------------------------------------------
