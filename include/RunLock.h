/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
