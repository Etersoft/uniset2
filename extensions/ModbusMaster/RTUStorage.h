// --------------------------------------------------------------------------
#ifndef _RTUSTORAGE_H_
#define _RTUSTORAGE_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include "modbus/ModbusTypes.h"
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
class ModbusRTUMaster;
// -----------------------------------------------------------------------------
class RTUStorage
{
	public:
		RTUStorage( ModbusRTU::ModbusAddr addr );
		~RTUStorage();

		void poll( ModbusRTUMaster* mb )
					throw(ModbusRTU::mbException);

		inline ModbusRTU::ModbusAddr getAddress(){ return addr; }
		inline bool ping(){ return pingOK; }

		inline void setPollADC( bool set ){ pollADC = set; }
		inline void setPollDI( bool set ){ pollDI = set; }
		inline void setPollDIO( bool set ){ pollDIO = set; }
		inline void setPollUNIO( bool set ){ pollUNIO = set; }

		enum RTUJack
		{
			nUnknown,
			nJ1,	// UNIO48 (FPGA0)
			nJ2,	// UNIO48 (FPGA1)
			nJ5,	// DIO 16
			nX1,	// АЦП (8)
			nX2,	// АЦП (8)
			nX4,	// DI (8)
			nX5		// DI (8)
		};

		static RTUJack s2j( const std::string& jack );
		static std::string j2s( RTUJack j );

		long getInt( RTUJack jack, unsigned short channel, UniversalIO::IOTypes t );
		float getFloat( RTUJack jack, unsigned short channel, UniversalIO::IOTypes t );
		bool getState( RTUJack jack, unsigned short channel, UniversalIO::IOTypes t );

		static ModbusRTU::ModbusData getRegister( RTUJack jack, unsigned short channel, UniversalIO::IOTypes t );

		static ModbusRTU::SlaveFunctionCode getFunction( RTUJack jack, unsigned short channel, UniversalIO::IOTypes t );

		// ДОДЕЛАТЬ: setState, setValue
		void print();

		friend std::ostream& operator<<(std::ostream& os, RTUStorage& m );
		friend std::ostream& operator<<(std::ostream& os, RTUStorage* m );

	protected:
		ModbusRTU::ModbusAddr addr;
		bool pingOK;

		bool pollADC;
		bool pollDI;
		bool pollDIO;
		bool pollUNIO;


		float adc[8]; 		// АЦП
		bool di[16]; 		// Порт 16DI
		bool dio_do[16]; 	// Порт 16DIO DO
		bool dio_di[16]; 	// Порт 16DIO DI
		float dio_ai[16]; 	// Порт 16DIO AI
		float dio_ao[16]; 	// Порт 16DIO AO
		bool unio_do[48]; 	// Порт UNIO48 DO
		bool unio_di[48]; 	// Порт UNIO48 DI
		float unio_ai[24]; 	// Порт UNIO48 AI
		float unio_ao[24]; 	// Порт UNIO48 AO
};
// --------------------------------------------------------------------------
#endif // _RTUSTORAGE_H_
// -----------------------------------------------------------------------------
