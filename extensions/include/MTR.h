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
#ifndef _MTR_H_
#define _MTR_H_
// -----------------------------------------------------------------------------
#include <string>
#include <map>
#include <cstdint>
#include <unordered_map>
#include <list>
#include <ostream>
#include <cstring>
#include <cmath>
#include "modbus/ModbusTypes.h"
#include "ComPort.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	class ModbusClient;
	// -----------------------------------------------------------------------------
	namespace MTR
	{
		// реализованные в данном интерфейсе типы данных
		enum MTRType
		{
			mtUnknown,
			mtT1,
			mtT2,
			mtT3,
			mtT4,
			mtT5,
			mtT6,
			mtT7,
			mtT8,
			mtT9,
			mtT10,
			mtT16,
			mtT17,
			mtF1,
			mtT_Str16,
			mtT_Str8
		};
		// -------------------------------------------------------------------------
		std::string type2str( MTRType t );          /*!< преоразование строки в тип */
		MTRType str2type( const std::string& s );   /*!< преобразование названия в строку */
		size_t wsize( MTRType t );                  /*!< длина данных в словах */
		// -------------------------------------------------------------------------
		// Информация
		const ModbusRTU::ModbusData regModelNumber  = 0x01;
		const ModbusRTU::ModbusData regSerialNumber = 0x09;

		std::string getModelNumber( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr );
		std::string getSerialNumber(uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr );
		// -------------------------------------------------------------------------
		// Настройки связи (чтение - read03, запись - write06)
		const ModbusRTU::ModbusData regUpdateConfiguration = 53;
		const ModbusRTU::ModbusData regAddress   = 55;
		const ModbusRTU::ModbusData regBaudRate  = 56;
		const ModbusRTU::ModbusData regStopBit   = 57; /* 0 - Stop bit, 1 - Stop bits */
		const ModbusRTU::ModbusData regParity    = 58;
		const ModbusRTU::ModbusData regDataBits  = 59;

		enum mtrBaudRate
		{
			br1200    = 0,
			br2400    = 1,
			br4800    = 2,
			br9600    = 3,
			br19200   = 4,
			br38400   = 5,
			br57600   = 6,
			br115200  = 7
		};

		enum mtrParity
		{
			mpNoParity   = 0,
			mpOddParity  = 1,
			mpEvenParity = 2
		};

		enum mtrDataBits
		{
			db8Bits = 0,
			db7Bits = 1
		};

		bool setAddress( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusAddr newAddr );
		bool setBaudRate(uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, mtrBaudRate br );
		bool setStopBit(uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, bool state );
		bool setParity(uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, mtrParity p );
		bool setDataBits( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr, mtrDataBits d );
		ComPort::Parity get_parity( ModbusRTU::ModbusData data );
		ComPort::Speed get_speed( ModbusRTU::ModbusData data );
		// -------------------------------------------------------------------------
		enum MTRError
		{
			mtrNoError,
			mtrBadDeviceType,
			mtrDontReadConfile,
			mtrSendParamFailed,
			mtrUnknownError
		};
		std::ostream& operator<<(std::ostream& os, MTRError& e );
		// Настройка из конф. файла
		MTRError update_configuration( uniset::ModbusClient* mb, ModbusRTU::ModbusAddr addr,
									   const std::string& mtrconfile, int verbose = 0 );
		// ---------------------------
		// вспомогательные функции и типы данных
		typedef std::list<ModbusRTU::ModbusData> DataList;
		typedef std::unordered_map<ModbusRTU::ModbusData, DataList> DataMap;
		const int attempts = 3; //
		static const ModbusRTU::ModbusData skip[] = {48, 49, 59};  // registers which should not write

		bool send_param( uniset::ModbusClient* mb, DataMap& dmap, ModbusRTU::ModbusAddr addr, int verb );
		bool read_param( const std::string& str, std::string& str1, std::string& str2 );
		DataMap read_confile( const std::string& f );
		void update_communication_params( ModbusRTU::ModbusAddr reg, ModbusRTU::ModbusData data,
										  uniset::ModbusClient* mb, ModbusRTU::ModbusAddr& addr, int verb );
		// -------------------------------------------------------------------------
		static const size_t u2size = 2;
		// -------------------------------------------------------------------------
		class T1
		{
			public:
				T1(): val(0) {}
				T1( uint16_t v ): val(v) {}
				T1( const ModbusRTU::ModbusData* data ): val(data[0]) {}
				~T1() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 1;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT1;
				}
				// ------------------------------------------
				uint16_t val;
		};
		std::ostream& operator<<(std::ostream& os, T1& t );
		// -------------------------------------------------------------------------
		class T2
		{
			public:
				T2(): val(0) {}
				T2( int16_t v ): val(v) {}
				T2( const ModbusRTU::ModbusData* data ): val(data[0]) {}
				~T2() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 1;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT2;
				}
				// ------------------------------------------
				int16_t val;
		};
		std::ostream& operator<<(std::ostream& os, T2& t );
		// -------------------------------------------------------------------------
		class T3
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					int32_t val; // :32
				} T3mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T3()
				{
					memset(raw.v, 0, sizeof(raw.v));
				}

				T3( int32_t i )
				{
					raw.val = i;
				}

				T3( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
				}

				T3( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// У MTR обратный порядок слов в ответе
						raw.v[0] = data[1];
						raw.v[1] = data[0];
					}
				}

				~T3() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT3;
				}
				// ------------------------------------------
				// функции преобразования к разным типам
				operator long()
				{
					return raw.val;
				}

				T3mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T3& t );
		// --------------------------------------------------------------------------
		class T4
		{
			public:
				// ------------------------------------------
				// конструкторы на разные случаи...
				T4(): sval(""), raw(0) {}
				T4( uint16_t v1 ): raw(v1)
				{
					char c[3];
					memcpy(c, &v1, 2);
					c[2] = '\0';
					sval = std::string(c);
				}

				T4( const ModbusRTU::ModbusData* data ):
					raw(data[0])
				{
					char c[3];
					memcpy(c, &(data[0]), 2);
					c[2] = '\0';
					sval     = std::string(c);
				}

				~T4() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 1;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT4;
				}
				// ------------------------------------------
				std::string sval;
				uint16_t raw;
		};
		std::ostream& operator<<(std::ostream& os, T4& t );
		// --------------------------------------------------------------------------
		class T5
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					struct u_T5
					{
						uint32_t val: 24;
						int8_t exp; // :8
					} __attribute__( ( packed ) ) u2;
					long lval;
				} T5mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T5(): val(0)
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				T5( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
					val = raw.u2.val * pow( (long)10, (long)raw.u2.exp );
				}

				T5( long v )
				{
					raw.lval = v;
					val = raw.u2.val * pow( (long)10, (long)raw.u2.exp );
				}

				T5( const ModbusRTU::ModbusData* data, size_t size ): val(0)
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[0] = data[1];
						raw.v[1] = data[0];
						val = raw.u2.val * pow( (long)10, (long)raw.u2.exp );
					}
				}

				~T5() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT5;
				}
				// ------------------------------------------
				double val;
				T5mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T5& t );
		// --------------------------------------------------------------------------
		class T6
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					struct u_T6
					{
						int32_t val: 24;
						int8_t exp; // :8
					} u2;
					long lval;
				} T6mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T6(): val(0)
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				T6( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
					val = raw.u2.val * pow( (long)10, (long)raw.u2.exp );
				}

				T6( long v )
				{
					raw.lval = v;
					val = raw.u2.val * pow( (long)10, (long)raw.u2.exp );
				}

				T6( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[0] = data[1];
						raw.v[1] = data[0];
						val = raw.u2.val * pow( (long)10, (long)raw.u2.exp );
					}
				}

				~T6() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT6;
				}
				// ------------------------------------------
				double val = { 0.0 };
				T6mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T6& t );
		// --------------------------------------------------------------------------
		class T7
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					struct u_T7
					{
						uint32_t val: 16;
						uint8_t ic; // :8 - Inductive/capacitive
						uint8_t ie; // :8 - Import/export
					} __attribute__( ( packed ) ) u2;
					long lval;
				} T7mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T7(): val(0)
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				T7( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
					val = raw.u2.val * pow( (long)10, (long) - 4 );
				}
				T7( const long v )
				{
					raw.lval = v;
					val = raw.u2.val * pow( (long)10, (long) - 4 );
				}

				T7( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[0] = data[1];
						raw.v[1] = data[0];
						val = raw.u2.val * pow( (long)10, (long) - 4 );
					}
				}

				~T7() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT7;
				}
				// ------------------------------------------
				double val = { 0.0 };
				T7mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T7& t );
		// --------------------------------------------------------------------------
		class T8
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					struct u_T8
					{
						uint16_t mon: 8;
						uint16_t day: 8;
						uint16_t hour: 8;
						uint16_t min: 8;
					} __attribute__( ( packed ) ) u2;
				} T8mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T8()
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				T8( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
				}

				T8( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[1] = data[0];
						raw.v[0] = data[1];
					}
				}

				inline uint16_t day()
				{
					return raw.u2.day;
				}
				inline uint16_t mon()
				{
					return raw.u2.mon;
				}
				inline uint16_t hour()
				{
					return raw.u2.hour;
				}
				inline uint16_t min()
				{
					return raw.u2.min;
				}

				~T8() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT8;
				}
				// ------------------------------------------
				T8mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T8& t );
		// --------------------------------------------------------------------------
		class T9
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					struct u_T9
					{
						uint16_t hour: 8;
						uint16_t min: 8;
						uint16_t sec: 8;
						uint16_t ssec: 8;
					} __attribute__( ( packed ) ) u2;
				} T9mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T9()
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				T9( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
				}

				T9( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[0] = data[1];
						raw.v[1] = data[0];
					}
				}

				inline uint16_t hour()
				{
					return raw.u2.hour;
				}
				inline uint16_t min()
				{
					return raw.u2.min;
				}
				inline uint16_t sec()
				{
					return raw.u2.sec;
				}
				inline uint16_t ssec()
				{
					return raw.u2.ssec;
				}

				~T9() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT9;
				}
				// ------------------------------------------
				T9mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T9& t );
		// -------------------------------------------------------------------------
		class T10
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[u2size];
					struct u_T10
					{
						uint16_t year: 16;
						uint16_t mon: 8;
						uint16_t day: 8;
					} __attribute__( ( packed ) ) u2;
				} T10mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				T10()
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				T10( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
				}

				T10( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[0] = data[1];
						raw.v[1] = data[0];
					}
				}

				inline uint16_t year()
				{
					return raw.u2.year;
				}
				inline uint16_t mon()
				{
					return raw.u2.mon;
				}
				inline uint16_t day()
				{
					return raw.u2.day;
				}

				~T10() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT10;
				}
				// ------------------------------------------
				T10mem raw;
		};
		std::ostream& operator<<(std::ostream& os, T10& t );
		// --------------------------------------------------------------------------

		class T16
		{
			public:
				T16(): val(0) {}
				T16( uint16_t v ): val(v)
				{
					fval = (float)(val) / 100.0;
				}
				T16( const ModbusRTU::ModbusData* data ): val(data[0])
				{
					fval = (float)(val) / 100.0;
				}
				T16( float f ): fval(f)
				{
					val = lroundf(fval * 100);
				}

				~T16() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 1;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT16;
				}
				// ------------------------------------------
				operator float()
				{
					return fval;
				}
				operator uint16_t()
				{
					return val;
				}

				uint16_t val = { 0 };
				float fval = { 0.0 };
		};
		std::ostream& operator<<(std::ostream& os, T16& t );
		// --------------------------------------------------------------------------
		class T17
		{
			public:
				T17(): val(0) {}
				T17( int16_t v ): val(v)
				{
					fval = (float)(v) / 100.0;
				}
				T17( uint16_t v ): val(v)
				{
					fval = (float)( (int16_t)(v) ) / 100.0;
				}

				T17( const ModbusRTU::ModbusData* data ): val(data[0])
				{
					fval = (float)(val) / 100.0;
				}
				T17( float f ): fval(f)
				{
					val = lroundf(fval * 100);
				}
				~T17() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 1;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT17;
				}
				// ------------------------------------------
				operator float()
				{
					return fval;
				}
				operator int16_t()
				{
					return val;
				}

				int16_t val = { 0 };
				float fval = { 0 };
		};
		std::ostream& operator<<(std::ostream& os, T17& t );
		// --------------------------------------------------------------------------
		class F1
		{
			public:
				// ------------------------------------------
				/*! тип хранения в памяти */
				typedef union
				{
					uint16_t v[2];
					float val; //
				} F1mem;
				// ------------------------------------------
				// конструкторы на разные случаи...
				F1()
				{
					memset(raw.v, 0, sizeof(raw.v));
				}
				F1( uint16_t v1, uint16_t v2 )
				{
					raw.v[0] = v1;
					raw.v[1] = v2;
				}

				F1( float f )
				{
					raw.val = f;
				}

				F1( const ModbusRTU::ModbusData* data, size_t size )
				{
					if( size >= u2size )
					{
						// При получении данных от MTR слова необходимо перевернуть
						raw.v[0] = data[1];
						raw.v[1] = data[0];
					}
				}

				~F1() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return u2size;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtF1;
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

				F1mem raw;
		};
		std::ostream& operator<<(std::ostream& os, F1& t );
		// --------------------------------------------------------------------------
		class T_Str16
		{
			public:
				// ------------------------------------------
				// конструкторы на разные случаи...
				T_Str16(): sval("") {}
				T_Str16( const ModbusRTU::ReadInputRetMessage& ret )
				{
					char c[17];
					ModbusRTU::ModbusData data[8];

					for( int i = 0; i < 8; i++ )
						data[i] = ModbusRTU::SWAPSHORT(ret.data[i]);

					memcpy(c, &data, 16);
					c[16] = '\0';
					sval = std::string(c);
				}

				~T_Str16() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 8;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT_Str16;
				}
				// ------------------------------------------
				std::string sval;
		};
		std::ostream& operator<<(std::ostream& os, T_Str16& t );
		// --------------------------------------------------------------------------

		class T_Str8
		{
			public:
				// ------------------------------------------
				// конструкторы на разные случаи...
				T_Str8(): sval("") {}
				T_Str8( const ModbusRTU::ReadInputRetMessage& ret )
				{
					char c[9];
					ModbusRTU::ModbusData data[4];

					for( int i = 0; i < 4; i++ )
						data[i] = ModbusRTU::SWAPSHORT(ret.data[i]);

					memcpy(c, &data, 8);
					c[8] = '\0';
					sval = std::string(c);
				}

				~T_Str8() {}
				// ------------------------------------------
				/*! размер в словах */
				static size_t wsize()
				{
					return 4;
				}
				/*! тип значения */
				static MTRType type()
				{
					return mtT_Str8;
				}
				// ------------------------------------------
				std::string sval;
		};
		std::ostream& operator<<(std::ostream& os, T_Str8& t );
		// --------------------------------------------------------------------------
	} // end of namespace MTR
	// --------------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif // _MTR_H_
// -----------------------------------------------------------------------------
