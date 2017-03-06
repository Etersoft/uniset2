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
//--------------------------------------------------------------------------
#ifndef WDTInterface_H_
#define WDTInterface_H_
/*
Для всех станций кроме GUI используется драйвер wdt686.o и девайс /dev/wdt с мажером 208 и минором 0
для GUI используется i810-tco, а девайс /dev/watchdog создаваемый через мискдевайс(?)
*/
//--------------------------------------------------------------------------
#include <string>
namespace uniset
{

	class WDTInterface
	{
		public:
			WDTInterface( const std::string& dev );
			~WDTInterface();

			bool ping();
			bool stop();

		protected:
			const std::string dev;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
//--------------------------------------------------------------------------
#endif
