#ifndef _TCPCheck_H_
#define _TCPCheck_H_
// -----------------------------------------------------------------------------
#include <atomic>
#include <cc++/socket.h>
#include "ThreadCreator.h"
// -----------------------------------------------------------------------------
/*! Вспомогательный класс для проверки связи, реализованный через создание потока,
чтобы при проверке не было "зависания" при недоступности адреса.
Смысл: создаётся поток, в нём происходит проверка, а в вызвавший поток приостанавливается
на время timeout, по истечении которого созданный поток "принудительно"(в любом случае!) уничтожается..
*/
class TCPCheck
{
	public:
		TCPCheck();
		~TCPCheck();

		/*! Проверка связи с сервисом на определённом порту
		 * \param _ip - ip проверяемого узла
		 * \param _port - порт для проверяемого узла
		 * \param tout - таймаут на попытку
		 * \param sleep_msec - пауза между проверками результата
		 *
		 * Для проверки идёт попытка открыть соединение, но данные не посылаются, а соединение сразу закрывается.
		 * \note Нужно быть уверенным, что сервис не зависнет от таких попыток "соединений"
		*/
		bool check( const std::string& _ip, int _port, timeout_t tout, timeout_t sleep_msec = 50 );

		/*! \param iaddr - 'ip:port' */
		bool check( const std::string& iaddr, timeout_t tout, timeout_t sleep_msec = 50);


		/*! Проверка связи с узлом командой ping
		 * \note Вызывается через system()! Это может быть опасно с точки зрения безопасности..
		 * \todo Возможно стоит написать свою реализацию ping
		 */
		bool ping( const std::string& _ip, timeout_t tout = 1000, timeout_t sleep_msec = 200, const std::string& ping_argc="-c 1 -w 0.1 -q -n" );

	protected:

		void check_thread();
		void ping_thread();

		void setResult( bool s )
		{
			result = s;
		}

		std::atomic_bool result = {false};
		std::string ip = {""};
		int port = {0};
		int tout_msec;

		std::string ping_args={"-c 1 -w 0.1 -q -n"};
};
// -----------------------------------------------------------------------------
#endif // _TCPCheck_H_
// -----------------------------------------------------------------------------
