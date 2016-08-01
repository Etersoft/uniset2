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
// --------------------------------------------------------------------------
#ifndef _RTUTypes_H_
#define _RTUTypes_H_
// -----------------------------------------------------------------------------
#include <string>
#include <cmath>
#include <cstring>
#include <ostream>
#include "modbus/ModbusTypes.h"
// -----------------------------------------------------------------------------
namespace VTypes
{
	/*! Тип переменной для Modbus[RTU|TCP] */
	enum VType
	{
		vtUnknown,
		vtF2,        /*!< двойное слово float(4 байта). В виде строки задаётся как \b "F2". */
		vtF2r,       /*!< двойное слово float(4 байта). С перевёрнутой (reverse) последовательностью слов. \b "F2r". */
		vtF4,        /*!< 8-х байтовое слово (double). В виде строки задаётся как \b "F4". */
		vtByte,      /*!< байт.  В виде строки задаётся как \b "byte". */
		vtUnsigned,  /*!< беззнаковое целое (2 байта).  В виде строки задаётся как \b "unsigned". */
		vtSigned,    /*!< знаковое целое (2 байта). В виде строки задаётся как \b "signed". */
		vtI2,        /*!< целое (4 байта). В виде строки задаётся как \b "I2".*/
		vtI2r,       /*!< целое (4 байта). С перевёрнутой (reverse) последовательностью слов. В виде строки задаётся как \b "I2r".*/
		vtU2,        /*!< беззнаковое целое (4 байта). В виде строки задаётся как \b "U2".*/
		vtU2r        /*!< беззнаковое целое (4 байта). С перевёрнутой (reverse) последовательностью слов. В виде строки задаётся как \b "U2r".*/
	};

	std::ostream& operator<<( std::ostream& os, const VType& vt );

	// -------------------------------------------------------------------------
	std::string type2str( VType t );           /*!< преобразование строки в тип */
	VType str2type( const std::string& s );    /*!< преобразование названия в строку */
	int wsize( VType t );                      /*!< длина данных в словах */
	// -------------------------------------------------------------------------
	class F2
	{
		public:

			// ------------------------------------------
			static const size_t f2Size = 2;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[f2Size];
				float val; //
			} F2mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			F2()
			{
				memset(raw.v, 0, sizeof(raw.v));
			}

			F2( const float& f )
			{
				raw.val = f;
			}
			F2( const ModbusRTU::ModbusData* data, size_t size )
			{
				for( size_t i = 0; i < wsize() && i < size; i++ )
					raw.v[i] = data[i];
			}

			~F2() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return f2Size;
			}
			/*! тип значения */
			static VType type()
			{
				return vtF2;
			}
			// ------------------------------------------
			operator float()
			{
				return raw.val;
			}
			operator long()
			{
				return lroundf(raw.val);
			}
			operator int()
			{
				return lroundf(raw.val);
			}

			F2mem raw;
	};
	// --------------------------------------------------------------------------
	class F2r:
		public F2
	{
		public:

			// ------------------------------------------
			// конструкторы на разные случаи...
			F2r() {}

			F2r( const float& f ): F2(f)
			{
				raw_backorder = raw;
				std::swap(raw_backorder.v[0], raw_backorder.v[1]);
			}
			F2r( const ModbusRTU::ModbusData* data, size_t size ): F2(data, size)
			{
				// принимаем в обратном порядке.. поэтому переворачиваем raw
				raw_backorder = raw;
				std::swap(raw.v[0], raw.v[1]);
			}

			~F2r() {}

			F2mem raw_backorder;
	};
	// --------------------------------------------------------------------------
	class F4
	{
		public:
			// ------------------------------------------
			static const size_t f4Size = 4;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[f4Size];
				float val; //
			} F4mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			F4()
			{
				memset(raw.v, 0, sizeof(raw.v));
			}

			F4( const float& f )
			{
				raw.val = f;
			}
			F4( const ModbusRTU::ModbusData* data, size_t size )
			{
				for( size_t i = 0; i < wsize() && i < size; i++ )
					raw.v[i] = data[i];
			}

			~F4() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return f4Size;
			}
			/*! тип значения */
			static VType type()
			{
				return vtF4;
			}
			// ------------------------------------------
			operator float()
			{
				return raw.val;
			}
			operator long()
			{
				return lroundf(raw.val);
			}

			F4mem raw;
	};
	// --------------------------------------------------------------------------
	class Byte
	{
		public:

			static const size_t bsize = 2;

			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short w;
				unsigned char b[bsize];
			} Bytemem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			Byte()
			{
				raw.w = 0;
			}

			Byte( unsigned char b1, unsigned char b2 )
			{
				raw.b[0] = b1;
				raw.b[1] = b2;
			}
			Byte( const long& val )
			{
				raw.w = val;
			}

			Byte( const ModbusRTU::ModbusData dat )
			{
				raw.w = dat;
			}

			~Byte() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return 1;
			}
			/*! тип значения */
			static VType type()
			{
				return vtByte;
			}
			// ------------------------------------------
			operator long()
			{
				return lroundf(raw.w);
			}
			operator unsigned short()
			{
				return raw.w;
			}

			unsigned char operator[]( const int i )
			{
				return raw.b[i];
			}

			Bytemem raw;
	};
	// --------------------------------------------------------------------------
	class Unsigned
	{
		public:

			// ------------------------------------------
			// конструкторы на разные случаи...
			Unsigned(): raw(0) {}

			Unsigned( const long& val )
			{
				raw = val;
			}

			Unsigned( const ModbusRTU::ModbusData dat )
			{
				raw = dat;
			}

			~Unsigned() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return 1;
			}
			/*! тип значения */
			static VType type()
			{
				return vtUnsigned;
			}
			// ------------------------------------------
			operator long()
			{
				return raw;
			}

			unsigned short raw;
	};
	// --------------------------------------------------------------------------
	class Signed
	{
		public:

			// ------------------------------------------
			// конструкторы на разные случаи...
			Signed(): raw(0) {}

			Signed( const long& val )
			{
				raw = val;
			}

			Signed( const ModbusRTU::ModbusData dat )
			{
				raw = dat;
			}

			~Signed() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return 1;
			}
			/*! тип значения */
			static VType type()
			{
				return vtSigned;
			}
			// ------------------------------------------
			operator long()
			{
				return raw;
			}

			signed short raw;
	};
	// --------------------------------------------------------------------------
	class I2
	{
		public:

			// ------------------------------------------
			static const size_t i2Size = 2;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[i2Size];
				int val; //
			} I2mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			I2()
			{
				memset(raw.v, 0, sizeof(raw.v));
			}

			I2( int v )
			{
				raw.val = v;
			}
			I2( const ModbusRTU::ModbusData* data, size_t size )
			{
				for( size_t i = 0; i < wsize() && i < size; i++ )
					raw.v[i] = data[i];
			}

			~I2() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return i2Size;
			}
			/*! тип значения */
			static VType type()
			{
				return vtI2;
			}
			// ------------------------------------------
			operator int()
			{
				return raw.val;
			}

			I2mem raw;
	};
	// --------------------------------------------------------------------------
	class I2r:
		public I2
	{
		public:
			I2r() {}

			I2r( const int v ): I2(v)
			{
				raw_backorder = raw;
				std::swap(raw_backorder.v[0], raw_backorder.v[1]);
			}
			I2r( const ModbusRTU::ModbusData* data, size_t size ): I2(data, size)
			{
				// принимаем в обратном порядке.. поэтому переворачиваем raw
				raw_backorder = raw;
				std::swap(raw.v[0], raw.v[1]);
			}

			~I2r() {}

			I2mem raw_backorder;
	};
	// --------------------------------------------------------------------------
	class U2
	{
		public:

			// ------------------------------------------
			static const size_t u2Size = 2;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[u2Size];
				unsigned int val; //
			} U2mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			U2()
			{
				memset(raw.v, 0, sizeof(raw.v));
			}

			U2( unsigned int v )
			{
				raw.val = v;
			}
			U2( const ModbusRTU::ModbusData* data, size_t size )
			{
				for( size_t i = 0; i < wsize() && i < size; i++ )
					raw.v[i] = data[i];
			}

			~U2() {}
			// ------------------------------------------
			/*! размер в словах */
			static size_t wsize()
			{
				return u2Size;
			}
			/*! тип значения */
			static VType type()
			{
				return vtU2;
			}
			// ------------------------------------------
			operator unsigned int()
			{
				return raw.val;
			}

			U2mem raw;
	};
	// --------------------------------------------------------------------------
	class U2r:
		public U2
	{
		public:
			U2r() {}

			U2r( int v ): U2(v)
			{
				raw_backorder = raw;
				std::swap(raw_backorder.v[0], raw_backorder.v[1]);
			}
			U2r( const ModbusRTU::ModbusData* data, size_t size ): U2(data, size)
			{
				// принимаем в обратном порядке.. поэтому переворачиваем raw
				raw_backorder = raw;
				std::swap(raw.v[0], raw.v[1]);
			}

			~U2r() {}

			U2mem raw_backorder;
	};
	// --------------------------------------------------------------------------

} // end of namespace VTypes
// --------------------------------------------------------------------------
#endif // _RTUTypes_H_
// -----------------------------------------------------------------------------
