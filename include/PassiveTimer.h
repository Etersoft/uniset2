/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 *  \author Vitaly Lipatov <lav>, Pavel Vainerman <pv>
 *  \date   $Date: 2007/08/02 22:52:27 $
 *  \version $Id: PassiveTimer.h,v 1.9 2007/08/02 22:52:27 vpashka Exp $
*/
//---------------------------------------------------------------------------- 
# ifndef PASSIVETIMER_H_
# define PASSIVETIMER_H_
//----------------------------------------------------------------------------
#include <signal.h>
#include <sys/time.h>
//#include "Exceptions.h"

//----------------------------------------------------------------------------------------
// CLK_TCK устарела по новому стандарту
#ifndef CLK_TCK
#define CLK_TCK sysconf(_SC_CLK_TCK)
#endif
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
/*! \class UniSetTimer
 * \brief Базовый интерфейс пасивных таймеров
 * \author Pavel Vainerman <pv>
 * \date  $Date: 2007/08/02 22:52:27 $
 * \version $Id: PassiveTimer.h,v 1.9 2007/08/02 22:52:27 vpashka Exp $
*/ 
class UniSetTimer
{
	public:
		virtual ~UniSetTimer(){};
	
		virtual bool checkTime()=0;				/*!< проверка наступления заданного времени */
		virtual void setTiming( int timeMS )=0; /*!< установить таймер и запустить */
		virtual void reset()=0;					/*!< перезапустить таймер */

		virtual int getCurrent()=0; 		/*!< получить текущее значение таймера */
		virtual int getInterval()=0;		/*!< получить интервал, на который установлен таймер, в мс */
		
		// объявлены не чисто виртуальными т.к.
		// некоторые классы могут не иметь подобных
		// свойств.
		virtual int wait(int timeMS){ return 0;} 	/*!< заснуть ожидая наступления времени */
		virtual void terminate(){}					/*!< прервать работу таймера */
		virtual void stop(){ terminate(); };		/*!< завершить работу таймера */

		/*! Время засыпания, до момента пока не будет вызвана функция прерывания
		 *  terminate() или stop()
		 */
		static const int WaitUpTime = -1;
		
		/*! Минимальное время срабатывания. Задается в мсек. */
		static const int MIN_QUANTITY_TIME_MS = 30;
};
//----------------------------------------------------------------------------------------
/*! \class PassiveTimer
 * \brief Пассивный таймер
 * \author Vitaly Lipatov <lav>
 * \date  $Date: 2007/08/02 22:52:27 $
 * \version $Id: PassiveTimer.h,v 1.9 2007/08/02 22:52:27 vpashka Exp $
 * \par
 * Установив таймер в конструкторе или с помощью setTiming,
 * можно с помощью checkTime проверять, не наступило ли нужное время
 * \note Если timeMS<0, таймер никогда не сработает
 * \note timeMS=0 - таймер сработает сразу
*/ 
class PassiveTimer: 
		public UniSetTimer
{
public:
	PassiveTimer();
	PassiveTimer( int timeMS ); 			/*!< установить таймер */
	

	virtual bool checkTime();				/*!< проверка наступления заданного времени */
	virtual void setTiming( int timeMS ); 	/*!< установить таймер и запустить */
	virtual void reset();					/*!< перезапустить таймер */

	virtual int getCurrent(); 				/*!< получить текущее значение таймера, в мс */
	virtual int getInterval()				/*!< получить интервал, на который установлен таймер, в мс */
	{
		return timeSS*10;
	}
	
	virtual void terminate();				/*!< прервать работу таймера */

protected:
	int timeAct;	/*!< время срабатывания таймера, в тиках */
	int timeSS;		/*!< интервал таймера, в сантисекундах */
	int timeStart;	/*!< время установки таймера (сброса) */
private:
	int clock_ticks; // CLK_TCK
};

//----------------------------------------------------------------------------------------
class omni_mutex;
class omni_condition;

/*! \class ThrPassiveTimer
 * \brief Пассивный таймер с режимом засыпания (ожидания)
 * \author Pavel Vainerman <pv>
 * \date  $Date: 2007/08/02 22:52:27 $
 * \version $Id: PassiveTimer.h,v 1.9 2007/08/02 22:52:27 vpashka Exp $
 * \par
 * Позволяет заснуть на заданное время wait(int timeMS).
 * Механизм работает на основе взаимных блокировок потоков (mutex и condition). 
 * \note Если таймер запущен в режиме ожидания (WaitUpTime), то он может быть выведен из него
 * при помощи terminate().
*/ 
class ThrPassiveTimer:
		public PassiveTimer
{ 
	public:
	
		ThrPassiveTimer();
		~ThrPassiveTimer();

		virtual int wait(int timeMS);	/*!< блокировать вызывающий поток на заданное время */
		virtual void terminate();		/*!< прервать работу таймера */
	protected:
	private:
		volatile sig_atomic_t terminated;
		omni_mutex* tmutex;
		omni_condition* tcondx;
};
//----------------------------------------------------------------------------------------

/*! \class PassiveSysTimer
 * \brief Пассивный таймер с режимом засыпания (ожидания)
 * \author Pavel Vainerman <pv>
 * \date  $Date: 2007/08/02 22:52:27 $
 * \version $Id: PassiveTimer.h,v 1.9 2007/08/02 22:52:27 vpashka Exp $
 * \par
 * Создан на основе сигнала (SIGALRM).
*/ 
class PassiveSysTimer:
		public PassiveTimer
{ 
	public:
	
		PassiveSysTimer();
		~PassiveSysTimer();

		virtual int wait(int timeMS); //throw(UniSetTypes::NotSetSignal);
		virtual void terminate();		
		
	protected:

	private:
		struct itimerval mtimer;	
		pid_t pid;	

//		bool terminated;
		volatile sig_atomic_t terminated;
		
		void init();

		static void callalrm(int signo );
		static void call(int signo, siginfo_t *evp, void *ucontext);
		
};

//----------------------------------------------------------------------------------------

# endif //PASSIVETIMER_H_
