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
#ifndef SingleProcess_H_
#define SingleProcess_H_
// --------------------------------------------------------------------------
#include <string>
// --------------------------------------------------------------------------
/*! Базовый класс для одиночных процессов.
    Обеспечивает корректное завершение процесса,
    даже по сигналам...
*/
class SingleProcess
{
	public:
		SingleProcess();
		virtual ~SingleProcess();

	protected:
		virtual void term( int signo ) {}

		static void set_signals( bool ask );

	private:

		static void terminated( int signo );
		static void finishterm( int signo );

};
// --------------------------------------------------------------------------
#endif // SingleProcess_H_
// --------------------------------------------------------------------------
