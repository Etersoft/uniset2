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
 *  \author Vitaly Lipatov, Pavel Vainerman
*/
//---------------------------------------------------------------------------- 
# ifndef PASSIVETIMER_H_
# define PASSIVETIMER_H_
//----------------------------------------------------------------------------
#include <signal.h>
#include <sys/time.h>
#include <cc++/socket.h>
#include "Mutex.h"
//----------------------------------------------------------------------------------------
/*! \class UniSetTimer
 * \brief Базовый интерфейс пасивных таймеров
 * \author Pavel Vainerman
*/ 
class UniSetTimer
{
	public:
		virtual ~UniSetTimer(){};

		virtual bool checkTime()=0;				/*!< проверка наступления заданного времени */
		virtual timeout_t setTiming( timeout_t timeMS )=0; /*!< установить таймер и запустить */
		virtual void reset()=0;					/*!< перезапустить таймер */

		virtual timeout_t getCurrent()=0; 		/*!< получить текущее значение таймера */
		virtual timeout_t getInterval()=0;		/*!< получить интервал, на который установлен таймер, в мс */
		timeout_t getLeft(timeout_t timeout)		/*< получить время, которое остается от timeout после прошествия времени getCurrent() */
		{
			timeout_t ct = getCurrent();
			if( timeout <= ct )
				return 0;
			return timeout - ct;
		}

		// объявлены не чисто виртуальными т.к.
		// некоторые классы могут не иметь подобных
		// свойств.
		virtual bool wait(timeout_t timeMS){ return 0;} 	/*!< заснуть ожидая наступления времени */
		virtual void terminate(){}					/*!< прервать работу таймера */
		virtual void stop(){ terminate(); };		/*!< завершить работу таймера */

		/*! Время засыпания, до момента пока не будет вызвана функция прерывания
		 *  terminate() или stop()
		 */
		static const timeout_t WaitUpTime = TIMEOUT_INF;

		/*! Минимальное время срабатывания. Задается в мсек. */
		static const timeout_t MinQuantityTime = 10;
		static const timeout_t MIN_QUANTITY_TIME_MS = 10; /*< устарело, не использовать! */
};
//----------------------------------------------------------------------------------------
/*! \class PassiveTimer
 * \brief Пассивный таймер
 * \author Vitaly Lipatov
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
	PassiveTimer( timeout_t timeMS ); 			/*!< установить таймер */


	virtual bool checkTime();				/*!< проверка наступления заданного времени */
	virtual timeout_t setTiming( timeout_t timeMS ); 	/*!< установить таймер и запустить. timeMS = 0 вызовет немедленное срабатывание */
	virtual void reset();					/*!< перезапустить таймер */

	virtual timeout_t getCurrent(); 				/*!< получить текущее значение таймера, в мс */
	virtual timeout_t getInterval()				/*!< получить интервал, на который установлен таймер, в мс */
	{
		// TODO: нужно убрать предположение, что нулевой интервал - выключенный таймер
		if( timeSS == WaitUpTime )
			return 0;
		return timeSS*10;
	}

	virtual void terminate();				/*!< прервать работу таймера */

protected:
	clock_t timeAct;	/*!< время срабатывания таймера, в тиках */
	timeout_t timeSS;		/*!< интервал таймера, в сантисекундах */
	clock_t timeStart;	/*!< время установки таймера (сброса) */
private:
	clock_t clock_ticks; // Количество тиков в секунду
	clock_t times(); // Возвращает текущее время в тиках
};

//----------------------------------------------------------------------------------------
class omni_mutex;
class omni_condition;

/*! \class ThrPassiveTimer
 * \brief Пассивный таймер с режимом засыпания (ожидания)
 * \author Pavel Vainerman
 * \par
 * Позволяет заснуть на заданное время wait(timeout_t timeMS).
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

		virtual bool wait(timeout_t timeMS);	/*!< блокировать вызывающий поток на заданное время */
		virtual void terminate();		/*!< прервать работу таймера */
	protected:
		  bool isTerminated();
		  void setTerminated( bool set );

	private:
		bool terminated;
		omni_mutex* tmutex;
		omni_condition* tcondx;
		UniSetTypes::uniset_mutex term_mutex;
};
//----------------------------------------------------------------------------------------

/*! \class PassiveSysTimer
 * \brief Пассивный таймер с режимом засыпания (ожидания)
 * \author Pavel Vainerman
 * \par
 * Создан на основе сигнала (SIGALRM).
*/ 
class PassiveSysTimer:
		public PassiveTimer
{ 
	public:

		PassiveSysTimer();
		~PassiveSysTimer();

		virtual bool wait(timeout_t timeMS); //throw(UniSetTypes::NotSetSignal);
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
