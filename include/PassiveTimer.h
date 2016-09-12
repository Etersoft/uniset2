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
 *  \author Vitaly Lipatov, Pavel Vainerman
*/
//----------------------------------------------------------------------------
# ifndef PASSIVETIMER_H_
# define PASSIVETIMER_H_
//----------------------------------------------------------------------------
#include <signal.h>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <limits>
#include <Poco/Timespan.h>
#include "Mutex.h"
//----------------------------------------------------------------------------------------
typedef Poco::Timespan::TimeDiff timeout_t;
//----------------------------------------------------------------------------------------
/*! \class UniSetTimer
 * \brief Базовый интерфейс пасивных таймеров
 * \author Pavel Vainerman
*/
class UniSetTimer
{
	public:
		virtual ~UniSetTimer() {};

		virtual bool checkTime() const noexcept = 0;					/*!< проверка наступления заданного времени */
		virtual timeout_t setTiming( timeout_t msec ) noexcept = 0;	/*!< установить таймер и запустить */
		virtual void reset() noexcept = 0;							/*!< перезапустить таймер */

		virtual timeout_t getCurrent() const noexcept = 0;  /*!< получить текущее значение таймера */
		virtual timeout_t getInterval() const noexcept = 0; /*!< получить интервал, на который установлен таймер, в мс */

		timeout_t getLeft( timeout_t timeout ) const noexcept;   /*!< получить время, которое остается от timeout после прошествия времени getCurrent() */

		// объявлены не чисто виртуальными т.к.
		// некоторые классы могут не иметь подобных
		// свойств.
		virtual bool wait(timeout_t timeMS);   /*!< заснуть ожидая наступления времени */
		virtual void terminate(){}            /*!< прервать работу таймера */

		/*! завершить работу таймера */
		virtual void stop() noexcept;

		/*! Время засыпания, до момента пока не будет вызвана функция прерывания
		 *  terminate() или stop()
		 */
		static const timeout_t WaitUpTime = std::numeric_limits<timeout_t>::max();

		// преобразование в Poco::Timespan с учётом WaitUpTime
		static const Poco::Timespan millisecToPoco( const timeout_t msec ) noexcept;
		static const Poco::Timespan microsecToPoco( const timeout_t usec ) noexcept;

		/*! Минимальное время срабатывания. Задается в мсек.
		 * Используется в LT_Object и CallbackTimer
		 */
		static const timeout_t MinQuantityTime = 10;
};
//----------------------------------------------------------------------------------------
/*! \class PassiveTimer
 * \brief Пассивный таймер
 * \author Vitaly Lipatov
 * \par
 * Установив таймер в конструкторе или с помощью setTiming,
 * можно с помощью checkTime проверять, не наступило ли нужное время
 * \note Если t_msec==WaitUpTime, таймер никогда не сработает
 * \note t_msec=0 - таймер сработает сразу
*/
class PassiveTimer:
	public UniSetTimer
{
	public:
		PassiveTimer() noexcept;
		PassiveTimer( timeout_t msec ) noexcept; /*!< установить таймер */
		virtual ~PassiveTimer() noexcept;

		virtual bool checkTime() const noexcept override; /*!< проверка наступления заданного времени */
		virtual timeout_t setTiming( timeout_t msec ) noexcept override;     /*!< установить таймер и запустить. timeMS = 0 вызовет немедленное срабатывание */
		virtual void reset() noexcept; /*!< перезапустить таймер */

		virtual timeout_t getCurrent() const noexcept override;  /*!< получить текущее значение таймера, в мс */

		/*! получить интервал, на который установлен таймер, в мс
		 * \return msec или 0 если интервал равен WaitUpTime
		 */
		virtual timeout_t getInterval() const noexcept override;

		virtual void terminate() noexcept; /*!< прервать работу таймера */

	protected:
		timeout_t t_msec = { 0 };  /*!< интервал таймера, в милисекундах (для "пользователей") */

		// Т.к. НЕ ВЕСЬ КОД переведён на использование std::chrono
		// везде используется timeout_t (и WaitUpTime)
		// отделяем внутреннее (теперь уже стандартное >= c++11)
		// представление для работы со временем (std::chrono)
		// и тип (t_msec) для "пользователей"
		std::chrono::high_resolution_clock::time_point t_start;	/*!< время установки таймера (сброса) */
		std::chrono::milliseconds t_inner_msec;	/*!< время установки таймера, мсек (в единицах std::chrono) */

	private:
};

//----------------------------------------------------------------------------------------
/*! \class PassiveCondTimer
 * \brief Пассивный таймер с режимом засыпания (ожидания)
 * \author Pavel Vainerman
 * \par
 * Позволяет заснуть на заданное время wait(timeout_t timeMS).
 * Механизм работает на основе std::condition_variable
 * \note Если таймер запущен в режиме ожидания (WaitUpTime), то он может быть выведен из него
 * ТОЛЬКО при помощи terminate().
*/
class PassiveCondTimer:
	public PassiveTimer
{
	public:

		PassiveCondTimer() noexcept;
		virtual ~PassiveCondTimer() noexcept;

		virtual bool wait(timeout_t t_msec) noexcept override; /*!< блокировать вызывающий поток на заданное время */
		virtual void terminate() noexcept override;  /*!< прервать работу таймера */

	protected:

	private:
		std::atomic_bool terminated;
		std::mutex    m_working;
		std::condition_variable cv_working;
};
//----------------------------------------------------------------------------------------
# endif //PASSIVETIMER_H_
