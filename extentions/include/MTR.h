// --------------------------------------------------------------------------
//!  \version $Id: MTR.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// --------------------------------------------------------------------------
#ifndef _MTR_H_
#define _MTR_H_
// -----------------------------------------------------------------------------
#include <string>
#include <cstring>
#include <cmath>
#include "modbus/ModbusTypes.h"
// -----------------------------------------------------------------------------
class ModbusRTUMaster;
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
		mtF1,
		mtT_Str16,
		mtT_Str8
	};
	// -------------------------------------------------------------------------
	std::string type2str( MTRType t );			/*!< преоразование строки в тип */
	MTRType str2type( const std::string s );	/*!< преобразование названия в строку */
	int wsize( MTRType t ); 					/*!< длина данных в словах */
	// -------------------------------------------------------------------------
	// Информация
	const ModbusRTU::ModbusData regModelNumber	= 30001;
	const ModbusRTU::ModbusData regSerialNumber	= 30009;
	
	std::string getModelNumber( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr );
	std::string getSerialNumber( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr );
	// -------------------------------------------------------------------------
	// Настройки связи
	const ModbusRTU::ModbusData regAddress		= 40055;
	const ModbusRTU::ModbusData regBaudRate		= 40056;
	const ModbusRTU::ModbusData regStopBit		= 40057; /* 0 - Stop bit, 1 - Stop bits */
	const ModbusRTU::ModbusData regParity		= 40058;
	const ModbusRTU::ModbusData regDataBits		= 40059;

	enum mtrBaudRate
	{
		br1200 		= 0,
		br2400 		= 1,
		br4800 		= 2,
		br9600 		= 3,
		br19200 	= 4,
		br38400 	= 5,
		br57600		= 6,
		br115200	= 7
	};
	
	enum mtrParity
	{
		mpNoParity		= 0,
		mpOddParity 	= 1,
		mpEvenParity 	= 2
	};

	enum mtrDataBits
	{
		db8Bits	= 0,
		db7Bits	= 1
	};

	bool setAddress( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusAddr newAddr );
	bool setBaudRate( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrBaudRate br );
	bool setStopBit( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, bool state );
	bool setParity( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrParity p );
	bool setDataBits( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrDataBits d );
	
	// -------------------------------------------------------------------------
	class T1
	{
		public:
			T1():val(0){}
			T1( unsigned short v ):val(v){}
			T1( const ModbusRTU::ReadOutputRetMessage& ret ):val(ret.data[0]){}
			T1( const ModbusRTU::ReadInputRetMessage& ret ):val(ret.data[0]){}
			~T1(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 1; }
			/*! тип значения */
			static MTRType type(){ return mtT1; }
			// ------------------------------------------
			unsigned short val;
	};	
	// -------------------------------------------------------------------------
	class T2
	{
		public:
			T2():val(0){}
			T2( signed short v ):val(v){}
			T2( const ModbusRTU::ReadOutputRetMessage& ret ):val(ret.data[0]){}
			T2( const ModbusRTU::ReadInputRetMessage& ret ):val(ret.data[0]){}
			~T2(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 1; }
			/*! тип значения */
			static MTRType type(){ return mtT2; }
			// ------------------------------------------
			signed short val;
	};
	// -------------------------------------------------------------------------
	class T3
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uT3
				{
					unsigned short v1;
					unsigned short v2;
				} u;
				signed int val; // :32
			} T3mem; 
			// ------------------------------------------
			// конструкторы на разные случаи...
			T3(){ raw.u.v1 = 0; raw.u.v2 = 0; }

			T3( unsigned short v1, unsigned short v2 )
			{
				raw.u.v1 = v1;
				raw.u.v2 = v2;
			}
			
			T3( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u.v1 = ret.data[0];
				raw.u.v2 = ret.data[1];
			}
			T3( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u.v1 = ret.data[0];
				raw.u.v2 = ret.data[1];
			}

			~T3(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtT3; }
			// ------------------------------------------
			// функции преобразования к разным типам
			operator long() { return raw.val; }

			T3mem raw;
	};
	// --------------------------------------------------------------------------
	class T4
	{
		public:
			// ------------------------------------------
			// конструкторы на разные случаи...
			T4():sval(""),raw(0){}
			T4( unsigned short v1 ):raw(v1)
			{
				char c[3];
				memcpy(c,&v1,sizeof(c));
				sval = std::string(c);
			}
			
			T4( const ModbusRTU::ReadOutputRetMessage& ret ):
				raw(ret.data[0])
			{
				char c[3];
				memcpy(c,&(ret.data[0]),sizeof(c));
				sval 	= std::string(c);
			}
			T4( const ModbusRTU::ReadInputRetMessage& ret ):
				raw(ret.data[0])
			{
				char c[3];
				memcpy(c,&(ret.data[0]),sizeof(c));
				sval 	= std::string(c);
			}

			~T4(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 1; }
			/*! тип значения */
			static MTRType type(){ return mtT4; }
			// ------------------------------------------
			std::string sval;
			unsigned short raw;
	};
	// --------------------------------------------------------------------------
	class T5
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uT5
				{
					unsigned short v1;
					unsigned short v2;
				} u1;
				struct u_T5
				{
					unsigned int val:24;
					signed char exp; // :8
				} __attribute__( ( packed ) ) u2;
			} T5mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			T5():val(0){ raw.u1.v1 = 0; raw.u1.v2 = 0; }
			T5( unsigned short v1, unsigned short v2 )
			{
				raw.u1.v1 = v1;
				raw.u1.v1 = v2;
				val = raw.u2.val * pow(10,raw.u2.exp);
			}
			
			T5( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
				val = raw.u2.val * pow(10,raw.u2.exp);
			}
			T5( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
				val = raw.u2.val * pow(10,raw.u2.exp);
			}

			~T5(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtT5; }
			// ------------------------------------------
			double val;
			T5mem raw;
	};
	// --------------------------------------------------------------------------
	class T6
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uT6
				{
					unsigned short v1;
					unsigned short v2;
				} u1;
				struct u_T6
				{
					signed int val:24;
					signed char exp; // :8
				} u2;
			} T6mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			T6():val(0){ raw.u1.v1 = 0; raw.u1.v2 = 0; }
			T6( unsigned short v1, unsigned short v2 )
			{
				raw.u1.v1 = v1;
				raw.u1.v1 = v2;
				val = raw.u2.val * pow(10,raw.u2.exp);
			}
			
			T6( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
				val = raw.u2.val * pow(10,raw.u2.exp);
			}
			T6( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
				val = raw.u2.val * pow(10,raw.u2.exp);
			}

			~T6(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtT6; }
			// ------------------------------------------
			double val;
			T6mem raw;
	};
	// --------------------------------------------------------------------------
	class T7
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uT7
				{
					unsigned short v1;
					unsigned short v2;
				} u1;
				struct u_T7
				{
					unsigned int val:16;
					unsigned char ic; // :8 - Inductive/capacitive
					unsigned char ie; // :8 - Import/export 
				}__attribute__( ( packed ) ) u2;
			} T7mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			T7():val(0){ raw.u1.v1 = 0; raw.u1.v2 = 0; }
			T7( unsigned short v1, unsigned short v2 )
			{
				raw.u1.v1 = v1;
				raw.u1.v1 = v2;
				val = raw.u2.val * pow(10,-4);
			}
			
			T7( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
				val = raw.u2.val * pow(10,-4);
			}
			T7( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
				val = raw.u2.val * pow(10,-4);
			}

			~T7(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtT7; }
			// ------------------------------------------
			double val;
			T7mem raw;
	};
	// --------------------------------------------------------------------------
	class T8
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uT8
				{
					unsigned short v1;
					unsigned short v2;
				} u1;
				struct u_T8
				{
					unsigned short mon:8;
					unsigned short day:8;
					unsigned short hour:8;
					unsigned short min:8;
				}__attribute__( ( packed ) ) u2;
			} T8mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			T8(){ raw.u1.v1 = 0; raw.u1.v2 = 0; }
			T8( unsigned short v1, unsigned short v2 )
			{
				raw.u1.v1 = v1;
				raw.u1.v1 = v2;
			}
			
			T8( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
			}
			T8( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
			}
			
			inline unsigned short day(){ return raw.u2.day; }
			inline unsigned short mon(){ return raw.u2.mon; }
			inline unsigned short hour(){ return raw.u2.hour; }
			inline unsigned short min(){ return raw.u2.min; }

			~T8(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtT8; }
			// ------------------------------------------
			T8mem raw;
	};
	// --------------------------------------------------------------------------
	class T9
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uT9
				{
					unsigned short v1;
					unsigned short v2;
				} u1;
				struct u_T9
				{
					unsigned short hour:8;
					unsigned short min:8;
					unsigned short sec:8;
					unsigned short ssec:8;
				}__attribute__( ( packed ) ) u2;
			} T9mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			T9(){ raw.u1.v1 = 0; raw.u1.v2 = 0; }
			T9( unsigned short v1, unsigned short v2 )
			{
				raw.u1.v1 = v1;
				raw.u1.v1 = v2;
			}
			
			T9( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
			}
			T9( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u1.v1 = ret.data[0];
				raw.u1.v2 = ret.data[1];
			}

			inline unsigned short hour(){ return raw.u2.hour; }
			inline unsigned short min(){ return raw.u2.min; }
			inline unsigned short sec(){ return raw.u2.sec; }
			inline unsigned short ssec(){ return raw.u2.ssec; }

			~T9(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtT9; }
			// ------------------------------------------
			T9mem raw;
	};
	// --------------------------------------------------------------------------
	class F1
	{
		public:
			// ------------------------------------------
			/*! тип хранения в памяти */
			typedef union
			{
				struct uF1
				{
					unsigned short v1;
					unsigned short v2;
				} u;
				float val; // 
			} F1mem;
			// ------------------------------------------
			// конструкторы на разные случаи...
			F1(){ raw.u.v1 = 0; raw.u.v2 = 0; }
			F1( unsigned short v1, unsigned short v2 )
			{
				raw.u.v1 = v1;
				raw.u.v1 = v2;
			}
			
			F1( const ModbusRTU::ReadOutputRetMessage& ret )
			{
				raw.u.v1 = ret.data[0];
				raw.u.v2 = ret.data[1];
			}
			F1( const ModbusRTU::ReadInputRetMessage& ret )
			{
				raw.u.v1 = ret.data[0];
				raw.u.v2 = ret.data[1];
			}

			~F1(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 2; }
			/*! тип значения */
			static MTRType type(){ return mtF1; }
			// ------------------------------------------
			operator float(){ return raw.val; }
			operator long(){ return lroundf(raw.val); }
			
			F1mem raw;
	};
	// --------------------------------------------------------------------------
	class T_Str16
	{
		public:
			// ------------------------------------------
			// конструкторы на разные случаи...
			T_Str16():sval(""){}
			T_Str16( const ModbusRTU::ReadInputRetMessage& ret )
			{
				char c[16];
				memcpy(c,&ret.data,sizeof(c));
				sval = std::string(c);
			}

			~T_Str16(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 8; }
			/*! тип значения */
			static MTRType type(){ return mtT_Str16; }
			// ------------------------------------------
			std::string sval;
	};
	// --------------------------------------------------------------------------
	class T_Str8
	{
		public:
			// ------------------------------------------
			// конструкторы на разные случаи...
			T_Str8():sval(""){}
			T_Str8( const ModbusRTU::ReadInputRetMessage& ret )
			{
				char c[8];
				memcpy(c,&ret.data,sizeof(c));
				sval = std::string(c);
			}

			~T_Str8(){}
			// ------------------------------------------
			/*! размер в словах */
			static int wsize(){ return 4; }
			/*! тип значения */
			static MTRType type(){ return mtT_Str8; }
			// ------------------------------------------
			std::string sval;
	};
	// --------------------------------------------------------------------------
} // end of namespace MTR
// --------------------------------------------------------------------------
#endif // _MTR_H_
// -----------------------------------------------------------------------------
