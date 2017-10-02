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
#ifndef _TCPCheck_H_
#define _TCPCheck_H_
// -----------------------------------------------------------------------------
#include <atomic>
#include "ThreadCreator.h"
#include "PassiveTimer.h" // for timeout_t
// -----------------------------------------------------------------------------
namespace uniset
{
	/*! Вспомогательный класс для проверки связи, реализованный через создание потока,
	чтобы при проверке не было "зависания" при недоступности адреса.
	Смысл: создаётся поток, в нём происходит проверка, а вызвавший поток приостанавливается
	на время timeout, по истечении которого созданный поток "принудительно"(в любом случае!) уничтожается..
	*/
	class TCPCheck
	{
		public:

			/*! Проверка связи с сервисом на определённом порту
			 * \param _ip - ip проверяемого узла
			 * \param _port - порт для проверяемого узла
			 * \param tout - таймаут на попытку
			 *
			 * Для проверки идёт попытка открыть соединение, но данные не посылаются, а соединение сразу закрывается.
			 * \note Нужно быть уверенным, что сервис не зависнет от таких попыток "соединений"
			*/
			static bool check( const std::string& _ip, int _port, timeout_t tout ) noexcept;

			/*! \param iaddr - 'ip:port' */
			static bool check( const std::string& iaddr, timeout_t tout ) noexcept;


			/*! Проверка связи с узлом командой ping
			 * \note Вызывается через system()! Это может быть опасно с точки зрения безопасности..
			 * \todo Возможно стоит написать свою реализацию ping
			 */
			static bool ping( const std::string& _ip, timeout_t tout = 1000, const std::string& ping_argc = "-c 1 -w 0.1 -q -n" ) noexcept;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -----------------------------------------------------------------------------
#endif // _TCPCheck_H_
// -----------------------------------------------------------------------------
