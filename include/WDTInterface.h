/* $Id: WDTInterface.h,v 1.1 2008/12/14 21:57:51 vpashka Exp $ */
//--------------------------------------------------------------------------
#ifndef WDTInterface_H_
#define WDTInterface_H_
/*
Для всех станций кроме GUI используется драйвер wdt686.o и девайс /dev/wdt с мажером 208 и минором 0
для GUI используется i810-tco, а девайс /dev/watchdog создаваемый через мискдевайс(?)
*/
//--------------------------------------------------------------------------
#include <string>

class WDTInterface
{
	public:
		WDTInterface(const std::string dev);
		~WDTInterface();

		bool	ping();
		bool	stop();
	
	protected:
		const std::string dev;
};
//--------------------------------------------------------------------------
#endif 
