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
// -----------------------------------------------------------------------------
#ifndef ComediInterface_H_
#define ComediInterface_H_
// -----------------------------------------------------------------------------
#include <string>
#include <comedilib.h>
#include "Exceptions.h"
// -----------------------------------------------------------------------------
/*! Интерфейс для работы с в/в */
class ComediInterface
{
	public:
		explicit ComediInterface( const std::string& dev );
		~ComediInterface();

		int getAnalogChannel( int subdev, int channel, int range = 0, int aref = AREF_GROUND )
		throw(UniSetTypes::Exception);

		void setAnalogChannel( int subdev, int channel, int data, int range = 0, int aref = AREF_GROUND )
		throw(UniSetTypes::Exception);

		bool getDigitalChannel( int subdev, int channel )
		throw(UniSetTypes::Exception);

		void setDigitalChannel( int subdev, int channel, bool bit )
		throw(UniSetTypes::Exception);


		// Конфигурирование входов / выходов
		enum ChannelType
		{
			DI     = INSN_CONFIG_DIO_INPUT,
			DO     = INSN_CONFIG_DIO_OUTPUT,
			AI    = 100,     // INSN_CONFIG_AIO_INPUT,
			AO    = 101    // INSN_CONFIG_AIO_OUTPUT
		};

		enum SubdevType
		{
			Unknown = 0,
			TBI24_0 = 1,
			TBI0_24 = 2,
			TBI16_8 = 3,
			GRAYHILL = 4
		};

		static std::string type2str( SubdevType t );
		static SubdevType str2type( const std::string& s );

		void configureSubdev( int subdev, SubdevType type ) throw(UniSetTypes::Exception);

		void configureChannel( int subdev, int channel, ChannelType type, int range = 0, int aref = 0 )
		throw(UniSetTypes::Exception);

		inline const std::string devname()
		{
			return dname;
		}

	protected:

		comedi_t* card;    /*!< интерфейс для работы с картами в/в */
		std::string dname;

	private:
};
// -----------------------------------------------------------------------------
#endif // ComediInterface_H_
// -----------------------------------------------------------------------------

