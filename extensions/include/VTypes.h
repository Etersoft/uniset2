// --------------------------------------------------------------------------
//!  \version $Id: RTUTypes.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
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
			vtF2,		/*!< двойное слово float(4 байта). В виде строки задаётся как \b "F2". */
			vtF4,		/*!< 8-х байтовое слово (double). В виде строки задаётся как \b "F4". */
			vtByte,		/*!< байт.  В виде строки задаётся как \b "byte". */
			vtUnsigned,	/*!< беззнаковое целое (2 байта).  В виде строки задаётся как \b "unsigned". */
			vtSigned,	/*!< знаковое целое (2 байта). В виде строки задаётся как \b "signed". */
			vtI2,		/*!< целое (4 байта). В виде строки задаётся как \b "I2".*/
			vtU2		/*!< беззнаковое целое (4 байта). В виде строки задаётся как \b "U2".*/
		};

		std::ostream& operator<<( std::ostream& os, const VType& vt );

		// -------------------------------------------------------------------------
		std::string type2str( VType t );	/*!< преобразование строки в тип */
		VType str2type( const std::string& s );	/*!< преобразование названия в строку */
		int wsize( VType t ); 			/*!< длина данных в словах */
	// -------------------------------------------------------------------------
	class F2
	{
		public:
		
			// ------------------------------------------
			static const int f2Size=2;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[f2Size];
				float val; // 
			} F2mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			F2(){ memset(raw.v,0,sizeof(raw.v)); }
			
			F2( float f ){ raw.val = f; }
			F2( const ModbusRTU::ModbusData* data, int size )
			{
				for( int i=0; i<wsize() && i<size; i++ )
					raw.v[i] = data[i];
			}

			~F2(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return f2Size; }
			/*! тип значения */
			static VType type(){ return vtF2; }
			// ------------------------------------------
			operator float(){ return raw.val; }
			operator long(){ return lroundf(raw.val); }
			
			F2mem raw;
	};
	// --------------------------------------------------------------------------
	class F4
	{
		public:
			// ------------------------------------------
			static const int f4Size=4;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[f4Size];
				float val; // 
			} F4mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			F4(){ memset(raw.v,0,sizeof(raw.v)); }
			
			F4( float f ){ raw.val = f; }
			F4( const ModbusRTU::ModbusData* data, int size )
			{
				for( int i=0; i<wsize() && i<size; i++ )
					raw.v[i] = data[i];
			}

			~F4(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return f4Size; }
			/*! тип значения */
			static VType type(){ return vtF4; }
			// ------------------------------------------
			operator float(){ return raw.val; }
			operator long(){ return lroundf(raw.val); }
			
			F4mem raw;
	};
	// --------------------------------------------------------------------------
	class Byte
	{
		public:
		
			static const int bsize = 2;
		
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short w;
				unsigned char b[bsize];
			} Bytemem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			Byte(){ raw.w = 0; }
			
			Byte( unsigned char b1, unsigned char b2 ){ raw.b[0]=b1; raw.b[1]=b2; }
			Byte( const long val )
			{
				raw.w = val;
			}
			
			Byte( const ModbusRTU::ModbusData dat )
			{
					raw.w = dat;
			}

			~Byte(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 1; }
			/*! тип значения */
			static VType type(){ return vtByte; }
			// ------------------------------------------
			operator long(){ return lroundf(raw.w); }

			unsigned char operator[]( const int i ){ return raw.b[i]; }

			Bytemem raw;
	};
	// --------------------------------------------------------------------------
	class Unsigned
	{
		public:

			// ------------------------------------------
			// конструкторы на разные случаи...
			Unsigned():raw(0){}
			
			Unsigned( const long val )
			{
				raw = val;
			}
			
			Unsigned( const ModbusRTU::ModbusData dat )
			{
				raw = dat;
			}

			~Unsigned(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return sizeof(unsigned short); }
			/*! тип значения */
			static VType type(){ return vtUnsigned; }
			// ------------------------------------------
			operator long(){ return raw; }

			unsigned short raw;
	};
	// --------------------------------------------------------------------------
	class Signed
	{
		public:

			// ------------------------------------------
			// конструкторы на разные случаи...
			Signed():raw(0){}
			
			Signed( const long val )
			{
				raw = val;
			}
			
			Signed( const ModbusRTU::ModbusData dat )
			{
				raw = dat;
			}

			~Signed(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return sizeof(signed short); }
			/*! тип значения */
			static VType type(){ return vtSigned; }
			// ------------------------------------------
			operator long(){ return raw; }

			signed short raw;
	};
	// --------------------------------------------------------------------------
	class I2
	{
		public:
		
			// ------------------------------------------
			static const int i2Size=2;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[i2Size];
				int val; // 
			} I2mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			I2(){ memset(raw.v,0,sizeof(raw.v)); }
			
			I2( int v ){ raw.val = v; }
			I2( const ModbusRTU::ModbusData* data, int size )
			{
				for( int i=0; i<wsize() && i<size; i++ )
					raw.v[i] = data[i];
			}

			~I2(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return i2Size; }
			/*! тип значения */
			static VType type(){ return vtI2; }
			// ------------------------------------------
			operator int(){ return raw.val; }
			
			I2mem raw;
	};
	// --------------------------------------------------------------------------
	class U2
	{
		public:
		
			// ------------------------------------------
			static const int u2Size=2;
			/*! тип хранения в памяти */
			typedef union
			{
				unsigned short v[u2Size];
				unsigned int val; // 
			} U2mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			U2(){ memset(raw.v,0,sizeof(raw.v)); }
			
			U2( unsigned int v ){ raw.val = v; }
			U2( const ModbusRTU::ModbusData* data, int size )
			{
				for( int i=0; i<wsize() && i<size; i++ )
					raw.v[i] = data[i];
			}

			~U2(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return u2Size; }
			/*! тип значения */
			static VType type(){ return vtU2; }
			// ------------------------------------------
			operator unsigned int(){ return raw.val; }
			
			U2mem raw;
	};
	// --------------------------------------------------------------------------

} // end of namespace VTypes
// --------------------------------------------------------------------------
#endif // _RTUTypes_H_
// -----------------------------------------------------------------------------
