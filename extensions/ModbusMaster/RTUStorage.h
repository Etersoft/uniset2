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
#ifndef _RTUSTORAGE_H_
#define _RTUSTORAGE_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <memory>
#include "modbus/ModbusTypes.h"
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	class ModbusRTUMaster;
	// -----------------------------------------------------------------------------
	class RTUStorage
	{
		public:
			explicit RTUStorage( ModbusRTU::ModbusAddr addr );
			~RTUStorage();

			// throw(ModbusRTU::mbException);
			void poll( const std::shared_ptr<ModbusRTUMaster>& mb );

			inline ModbusRTU::ModbusAddr getAddress()
			{
				return addr;
			}
			inline bool ping()
			{
				return pingOK;
			}

			inline void setPollADC( bool set )
			{
				pollADC = set;
			}
			inline void setPollDI( bool set )
			{
				pollDI = set;
			}
			inline void setPollDIO( bool set )
			{
				pollDIO = set;
			}
			inline void setPollUNIO( bool set )
			{
				pollUNIO = set;
			}

			enum RTUJack
			{
				nUnknown,
				nJ1,    // UNIO48 (FPGA0)
				nJ2,    // UNIO48 (FPGA1)
				nJ5,    // DIO 16
				nX1,    // АЦП (8)
				nX2,    // АЦП (8)
				nX4,    // DI (8)
				nX5        // DI (8)
			};

			static RTUJack s2j( const std::string& jack );
			static std::string j2s( RTUJack j );

			long getInt( RTUJack jack, unsigned short channel, UniversalIO::IOType t );
			float getFloat( RTUJack jack, unsigned short channel, UniversalIO::IOType t );
			bool getState( RTUJack jack, unsigned short channel, UniversalIO::IOType t );

			static ModbusRTU::ModbusData getRegister( RTUJack jack, unsigned short channel, UniversalIO::IOType t );

			static ModbusRTU::SlaveFunctionCode getFunction( RTUJack jack, unsigned short channel, UniversalIO::IOType t );

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


			float adc[8];         // АЦП
			bool di[16];         // Порт 16DI
			bool dio_do[16];     // Порт 16DIO DO
			bool dio_di[16];     // Порт 16DIO DI
			float dio_ai[16];     // Порт 16DIO AI
			float dio_ao[16];     // Порт 16DIO AO
			bool unio_do[48];     // Порт UNIO48 DO
			bool unio_di[48];     // Порт UNIO48 DI
			float unio_ai[24];     // Порт UNIO48 AI
			float unio_ao[24];     // Порт UNIO48 AO
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif // _RTUSTORAGE_H_
// -----------------------------------------------------------------------------
