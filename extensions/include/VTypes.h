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
#include <stdint.h>
#include <cmath>
#include <cstring>
#include <ostream>
#include "modbus/ModbusTypes.h"
// -----------------------------------------------------------------------------
namespace uniset
{
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
		std::string type2str( VType t ) noexcept;           /*!< преобразование строки в тип */
		VType str2type( const std::string& s ) noexcept;    /*!< преобразование названия в строку */
		int wsize( VType t ) noexcept;                      /*!< длина данных в словах */
		// -------------------------------------------------------------------------
		class F2
		{
			public:

				// ------------------------------------------
				static const size_t f2Size = 2;
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[f2Size];
					float val; //
				} F2mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				F2() noexcept
				{
					memset(raw.v, 0, sizeof(raw.v));
				}

				F2( const float& f ) noexcept
				{
					raw.val = f;
				}
				F2( const ModbusRTU::ModbusData* data, size_t size ) noexcept
				{
					for( size_t i = 0; i < wsize() && i < size; i++ )
						raw.v[i] = data[i];
				}

				~F2() noexcept {}
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
				F2r() noexcept
				{
					raw_backorder.val = 0;
				}

				F2r( const float& f ) noexcept: F2(f)
				{
					raw_backorder = raw;
					std::swap(raw_backorder.v[0], raw_backorder.v[1]);
				}
				F2r( const ModbusRTU::ModbusData* data, size_t size ) noexcept: F2(data, size)
				{
					// принимаем в обратном порядке.. поэтому переворачиваем raw
					raw_backorder = raw;
					std::swap(raw.v[0], raw.v[1]);
				}

				~F2r() noexcept {}

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
					uint16_t v[f4Size];
					float val; //
				} F4mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				F4() noexcept
				{
					memset(raw.v, 0, sizeof(raw.v));
				}

				F4( const float& f ) noexcept
				{
					memset(raw.v, 0, sizeof(raw.v));
					raw.val = f;
				}
				F4( const ModbusRTU::ModbusData* data, size_t size ) noexcept
				{
					for( size_t i = 0; i < wsize() && i < size; i++ )
						raw.v[i] = data[i];
				}

				~F4() noexcept {}
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
					uint16_t w;
					uint8_t b[bsize];
				} Bytemem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				Byte() noexcept
				{
					raw.w = 0;
				}

				Byte( uint8_t b1, uint8_t b2 ) noexcept
				{
					raw.b[0] = b1;
					raw.b[1] = b2;
				}

				Byte( const ModbusRTU::ModbusData dat ) noexcept
				{
					raw.w = dat;
				}

				~Byte() noexcept {}
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
				operator uint16_t()
				{
					return raw.w;
				}

				uint8_t operator[]( const size_t i )
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
				Unsigned() noexcept: raw(0) {}

				Unsigned( const long& val ) noexcept
				{
					raw = val;
				}

				Unsigned( const ModbusRTU::ModbusData dat ) noexcept
				{
					raw = dat;
				}

				~Unsigned() noexcept {}
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

				uint16_t raw;
		};
		// --------------------------------------------------------------------------
		class Signed
		{
			public:

				// ------------------------------------------
				// конструкторы на разные случаи...
				Signed() noexcept: raw(0) {}

				Signed( const long& val ) noexcept
				{
					raw = val;
				}

				Signed( const ModbusRTU::ModbusData dat ) noexcept
				{
					raw = dat;
				}

				~Signed() noexcept {}
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

				int16_t raw;
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
					uint16_t v[i2Size];
					int32_t val; //
				} I2mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				I2() noexcept
				{
					memset(raw.v, 0, sizeof(raw.v));
				}

				I2( int32_t v ) noexcept
				{
					raw.val = v;
				}
				I2( const ModbusRTU::ModbusData* data, size_t size ) noexcept
				{
					for( size_t i = 0; i < wsize() && i < size; i++ )
						raw.v[i] = data[i];
				}

				~I2() noexcept {}
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
				operator int32_t()
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
				I2r() noexcept
				{
					raw_backorder.val = 0;
				}

				I2r( const int32_t v ) noexcept: I2(v)
				{
					raw_backorder = raw;
					std::swap(raw_backorder.v[0], raw_backorder.v[1]);
				}

				I2r( const ModbusRTU::ModbusData* data, size_t size ) noexcept: I2(data, size)
				{
					// принимаем в обратном порядке.. поэтому переворачиваем raw
					raw_backorder = raw;
					std::swap(raw.v[0], raw.v[1]);
				}

				~I2r() noexcept {}

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
					uint16_t v[u2Size];
					uint32_t val; //
				} U2mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				U2() noexcept
				{
					memset(raw.v, 0, sizeof(raw.v));
				}

				U2( uint32_t v ) noexcept
				{
					raw.val = v;
				}
				U2( const ModbusRTU::ModbusData* data, size_t size ) noexcept
				{
					for( size_t i = 0; i < wsize() && i < size; i++ )
						raw.v[i] = data[i];
				}

				~U2() noexcept {}
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
				operator uint32_t()
				{
					return raw.val;
				}

				operator long()
				{
					return raw.val;
				}

				operator unsigned long()
				{
					return (uint32_t)raw.val;
				}

				U2mem raw;
		};
		// --------------------------------------------------------------------------
		class U2r:
			public U2
		{
			public:
				U2r() noexcept
				{
					raw_backorder.val = 0;
				}

				U2r( int32_t v ) noexcept: U2(v)
				{
					raw_backorder = raw;
					std::swap(raw_backorder.v[0], raw_backorder.v[1]);
				}

				U2r( const ModbusRTU::ModbusData* data, size_t size ) noexcept: U2(data, size)
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
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif // _RTUTypes_H_
// -----------------------------------------------------------------------------
