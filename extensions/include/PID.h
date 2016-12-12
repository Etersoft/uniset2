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
#ifndef PID_H_
#define PID_H_
// -----------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
// --------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	/*! ПИД
	    Формулы выведены на основе разностных уравнений
	    см. http://atm.h1.ru/root/theory/theory33.html

	    Он даёт неплохой результат и опимальнее по расчётам
	    (содержит только умножение, не переполняется
	        т.к. учитывает только два последних шага)
	*/
	class PID
	{
		public:
			PID();
			~PID();

			/*! Выполнение очередного шага расчётов
			    \param X - входное значение
			    \param Z - заданное значение
			    \param Ts - интервал расчёта данных, [сек] (интервал между шагами расчёта).
			        Ts - должно быть больше нуля
			*/
			void step( const double& X, const double& Z, const double& Ts );

			/*!    рестарт регулятора... */
			void reset();

			/*! пересчёт констант */
			void recalc();

			double Y;    /*!< расчётное выходное значение */
			double Kc;    /*!< пропорциональный коэффициент */
			double Ti;    /*!< постоянная времени интеграла, [сек] */
			double Td;    /*!< постоянная времени дифференциала, [сек] */

			double vlim;     /*!< максимальное(минимальное) разрешённое значение (для любого растущего во времени коэффициента)
                            защита от переполнения
                         */
			double d0;
			double d1;
			double d2;
			double sub1;
			double sub2;
			double sub;
			double prevTs;

			friend std::ostream& operator<<(std::ostream& os, PID& p );

			friend std::ostream& operator<<(std::ostream& os, PID* p )
			{
				return os << (*p);
			}

		protected:
		private:
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // PID_H_
// -----------------------------------------------------------------------------
