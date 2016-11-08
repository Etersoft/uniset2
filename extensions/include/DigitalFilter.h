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
// Реализация различных цифровых фильтров
//--------------------------------------------------------------------------
#ifndef DigitalFilter_H_
#define DigitalFilter_H_
//--------------------------------------------------------------------------
#include <deque>
#include <vector>
#include <ostream>
#include "PassiveTimer.h"
//--------------------------------------------------------------------------
namespace uniset
{
//--------------------------------------------------------------------------
class DigitalFilter
{
	public:
		DigitalFilter ( unsigned int bufsize = 5, double T = 0, double lsq = 0.2,
						int iir_thr = 10000, double iir_coeff_prev = 0.5,
						double iir_coeff_new = 0.5 );
		~DigitalFilter ();

		// T <=0 - отключить вторую ступень фильтра
		void setSettings( unsigned int bufsize, double T, double lsq,
						  int iir_thr, double iir_coeff_prev, double iir_coeff_new );

		// Усреднение с учётом СКОС
		// На вход подается новое значение
		// возвращается фильтрованное с учётом
		// предыдущих значений...
		int filter1( int newValue );

		// RC-фильтр
		int filterRC( int newVal );

		// медианный фильтр
		int median( int newval );

		// адаптивный фильтр по схеме наименьших квадратов
		int leastsqr( int newval );

		// рекурсивный фильтр
		int filterIIR( int newval );

		// получить текущее фильтрованное значение
		int current1();
		int currentRC();
		int currentMedian();
		int currentLS();
		int currentIIR();

		// просто добавить очередное значение
		void add( int newValue );

		void init( int val );

		// void init( list<int>& data );

		inline int size()
		{
			return buf.size();
		}

		inline double middle()
		{
			return M;
		}
		inline double sko()
		{
			return S;
		}

		friend std::ostream& operator<<(std::ostream& os, const DigitalFilter& d);
		friend std::ostream& operator<<(std::ostream& os, const DigitalFilter* d);

	private:

		// Первая ступень фильтра
		double firstLevel();
		// Вторая ступень фильтра, математическая реализация RC фильтра
		double secondLevel( double val );

		double Ti;       // Постоянная времени для апериодического звена в милисекундах
		double val;      // Текущее значение второй ступени фильтра
		double M;        // Среднее арифметическое
		double S;        // Среднеквадратичное отклонение
		PassiveTimer tmr;

		typedef std::deque<int> FIFOBuffer;
		FIFOBuffer buf;
		unsigned int maxsize;

		typedef std::vector<int> MedianVector;
		MedianVector mvec;
		bool mvec_sorted; // флаг, что mvec остортирован (заполнен)

		typedef std::vector<double> Coeff;
		Coeff w;        // Вектор коэффициентов для filterIIR

		double lsparam; // Параметр для filterIIR
		double ls;      // Последнее значение, возвращённое filterIIR

		int thr;        // Порог для изменений, обрабатываемых рекурсивным фильтром
		int prev;       // Последнее значение, возвращённое рекурсивным фильтром

		// Коэффициенты для рекурсивного фильтра
		double coeff_prev;
		double coeff_new;
};
// -------------------------------------------------------------------------
} // end of namespace uniset
//--------------------------------------------------------------------------
#endif // DigitalFilter_H_
//--------------------------------------------------------------------------
