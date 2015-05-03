#ifndef _TCPCheck_H_
#define _TCPCheck_H_
// -----------------------------------------------------------------------------
#include <atomic>
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

		/*! Проверка связи с узлом
		 * \param _ip - ip проверяемого узла
		 * \param _port - порт для проверяемого узла
		 * \param tout - таймаут на попытку
		 * \param sleep_msec - пауза между проверками результата
		*/
		bool check( const std::string& _ip, int _port, timeout_t tout, timeout_t sleep_msec = 50 );

		/*! \param iaddr - 'ip:port' */
		bool check( const std::string& iaddr, timeout_t tout, timeout_t sleep_msec = 50);

	protected:
		void check_thread();

		void setResult( bool s )
		{
			result = s;
		}

		std::atomic_bool result = {false};
		std::string ip = {""};
		int port = {0};
		int tout_msec;
};
// -----------------------------------------------------------------------------
#endif // _TCPCheck_H_
// -----------------------------------------------------------------------------
