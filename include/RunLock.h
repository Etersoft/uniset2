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
/*! \file
 *  \brief Блокировка повторного запуска программы
 *  \author Ahton Korbin, Pavel Vainerman
*/
// ---------------------------------------------------------------------------
#ifndef RunLock_H_
#define RunLock_H_
// ---------------------------------------------------------------------------
#include <string>
// ---------------------------------------------------------------------------
/*! Защита от поторного запуска программы(процесса).
    При вызове lock(lockFile) в файл lockFile записывается pid текущего процесса.
    При вызове isLocked() проверяется состояние процесса по его pid (записанному в файл).
    unlock() - удаляет файл.

\warning Код не переносимый, т.к. рассчитан на наличие каталога /proc,
по которому проверяется статус процесса (по pid).
*/
class RunLock
{
	public:
		RunLock();
		~RunLock();

		static bool isLocked(const std::string& lockFile); //, char* **argv );
		static bool lock(const std::string& lockFile);
		static bool unlock(const std::string& lockFile);

	protected:

};
// ----------------------------------------------------------------------------
#endif
