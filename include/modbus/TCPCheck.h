#ifndef _TCPCheck_H_
#define _TCPCheck_H_
// -----------------------------------------------------------------------------
#include <cc++/socket.h>
#include "Mutex.h"
#include "ThreadCreator.h"
// -----------------------------------------------------------------------------
/*! Вспомогательный класс для проверки связи, реализованный через создание потока,
чтобы при проверке не было "зависания" при недоступности адреса.
Смысл: создаётся поток, в нём происходит проверка, а в вызвавший поток приостанавливается
на время timeout, по истечении которого созданный поток "принудительно"(в любом случае)
уничтожается..
*/
class TCPCheck
{
	public:
		TCPCheck();
		~TCPCheck();

		bool check( const std::string& _ip, int _port, timeout_t tout, timeout_t sleep_msec );

		/*! \param iaddr - 'ip:port' */
		bool check( const std::string& iaddr, timeout_t tout, timeout_t sleep_msec );

	protected:
		void check_thread();
		inline void setResult( bool res )
		{
			UniSetTypes::uniset_rwmutex_wrlock l(m);
			result = res;
		}

		inline bool getResult()
		{
			bool res = false;
			{
				UniSetTypes::uniset_rwmutex_rlock l(m);
				res = result;
			}
			return res;
		}

		bool result;
		std::string iaddr;
		int tout_msec;
		UniSetTypes::uniset_rwmutex m;
};
// -----------------------------------------------------------------------------
#endif // _TCPCheck_H_
// -----------------------------------------------------------------------------
