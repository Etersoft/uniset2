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
//--------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	/*! Интерфейс для работы с в/в */
	class ComediInterface
	{
		public:
			explicit ComediInterface( const std::string& dev, const std::string& cname );
			virtual ~ComediInterface();

			virtual int getAnalogChannel( int subdev, int channel, int range = 0, int aref = AREF_GROUND ) const
			throw(uniset::Exception);

			virtual void setAnalogChannel( int subdev, int channel, int data, int range = 0, int aref = AREF_GROUND ) const
			throw(uniset::Exception);

			virtual bool getDigitalChannel( int subdev, int channel ) const
			throw(uniset::Exception);

			virtual void setDigitalChannel( int subdev, int channel, bool bit ) const
			throw(uniset::Exception);

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

			virtual void configureSubdev( int subdev, SubdevType type ) const throw(uniset::Exception);

			virtual void configureChannel( int subdev, int channel, ChannelType type, int range = 0, int aref = 0 ) const
			throw(uniset::Exception);

			inline const std::string devname() const
			{
				return dname;
			}

			inline const std::string cardname() const
			{
				return name;
			}

		protected:
			ComediInterface(): card(nullptr) {}

			comedi_t* card;    /*!< интерфейс для работы с картами в/в */
			std::string dname;
			std::string name;

		private:
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // ComediInterface_H_
// -----------------------------------------------------------------------------

