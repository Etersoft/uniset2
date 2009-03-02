// $Id: DigitalFilter.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
//--------------------------------------------------------------------------
// Цифровой фильтр с двумя (опционально) уровнями фильтрации сигнала
// Первый уровень фильтра усредняет несколько значений, переданных через массив
// Второй уровень - математическая реалезация RC фильтра
// После вызова конструктора фильтра, его необходимо еще проинициализировать
// вызвав функцию FirstValue
//--------------------------------------------------------------------------
#ifndef DigitalFilter_H_
#define DigitalFilter_H_
//--------------------------------------------------------------------------
#include <list>
#include <vector>
#include <ostream>
#include "PassiveTimer.h"
//--------------------------------------------------------------------------
class DigitalFilter
{
	public:
		DigitalFilter ( unsigned int bufsize=5, double T=0 );
		~DigitalFilter ();
		
		// T <=0 - отключить вторую ступень фильтра
		void setSettings( unsigned int bufsize, double T );

		// Усреднение с учётом СКОС
		// На вход подается новое значение
		// возвращается фильтрованное с учётом 
		// предыдущих значений...
		int filter1( int newValue );
		
		// RC-фильтр
		int filterRC( int newVal );
		
		// медианный фильтр
		int median( int newval );
		
		// получить текущее фильтрованное значение
		int current1();
		int currentRC();
		int currentMedian();

		// просто добавить очередное значение
		void add( int newValue );

		void init( int val );
		
		// void init( list<int>& data );
	
		inline int size(){ return buf.size(); }

		inline double middle(){ return M; }
		inline double sko(){ return S; }

		friend std::ostream& operator<<(std::ostream& os, const DigitalFilter& d);
		friend std::ostream& operator<<(std::ostream& os, const DigitalFilter* d);
		
	private:

		// Первая ступень фильтра
		double firstLevel();
		// Вторая ступень фильтра, математическая реализация RC фильтра
		double secondLevel( double val );

		double Ti;		// Постоянная времени для апериодического звена в милисекундах
		double val;		// Текущее значение второй ступени фильтра
		double M;		// Среднее арифметическое
		double S;		// Среднеквадратичное отклонение
		PassiveTimer tmr;
		
		typedef std::list<int> FIFOBuffer;
		FIFOBuffer buf;		
		unsigned int maxsize;
		
		typedef std::vector<int> MedianVector;
		MedianVector mvec;
};
//--------------------------------------------------------------------------
#endif // DigitalFilter_H_
//--------------------------------------------------------------------------
