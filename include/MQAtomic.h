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
#ifndef MQAtomic_H_
#define MQAtomic_H_
//--------------------------------------------------------------------------
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include "MessageType.h"
//--------------------------------------------------------------------------
typedef std::shared_ptr<UniSetTypes::VoidMessage> VoidMessagePtr;
//--------------------------------------------------------------------------
/*! \class MQAtomic
 * Очередь сообщений на основе atomic переменных.
 *
 * Чтобы избежать работы с mutex, очередь построена по принципу циклического буфера,
 * c использованием atomic-переменных и попыткой реализовать LockFree работу.
 * Есть монотонно растущий индекс текущей позиции записи (wpos) и есть "догоняющий его" индекс позиции чтения (rpos).
 * Если rpos догоняет wpos - значит новых сообщений нет.
 *
 * \warning Очередь не универсальная и предназначена исключительно для использования в UniSetObject.
 * Т.к. подразумевает схему "МНОГО ПИСАТЕЛЕЙ" и "ОДИН ЧИТАТЕЛЬ".
 *
 * При этом место под очередь(буффер) резервируется сразу.
 * Счётчики сделаны (ulong) монотонно растущими.
 * Основные идеи:
 * - счётчики постоянно увеличиваются
 * - каждый пишущий поток пишет в новое место (индекс больше последнего)
 * - читающий счётчик тоже монотонно растёт
 * - реальная позиция для записи или чтения рассчитывается как (pos%size) этим и обеспечивается цикличность.
 *
 * Максимальное ограничение на размер очереди сообщений задаётся функцией setMaxSizeOfMessageQueue().
 *
 * Контроль переполения очереди осуществляется в push и в top;
 * Если очередь переполняется, то сообщения ТЕРЯЮТСЯ!
 * При помощи функции setLostStrategy() можно установить стратегию что терять
 * lostNewData - в случае переполнения теряются новые данные (т.е. не будут помещаться в очередь)
 * lostOldData - в случае переполнения очереди, старые данные затираются новыми.
 * Под переполнением подразумевается, что чтение отстаёт от писателей больше чем на размер буфера.
 *
 * --------------------------------
 * ЭТА ОЧЕРЕДЬ ПОКАЗЫВАЕТ В ТРИ РАЗА ЛУЧШУЮ СКОРОСТЬ ПО СРАВНЕНИЮ С MQMutex
 * --------------------------------
*/
class MQAtomic
{
	public:
		MQAtomic( size_t qsize = 2000 );

		/*! поместить сообщение в очередь */
		void push( const VoidMessagePtr& msg );

		/*! Извлечь сообщение из очереди
		 * \return не валидный shatred_ptr если сообщений нет
		 */
		VoidMessagePtr top();

		size_t size();
		bool empty();

		// ----- Настройки  -----
		// неявно подразумевается, что всё настривается до первого использования
		// ----------------------
		void setMaxSizeOfMessageQueue( size_t s );
		size_t getMaxSizeOfMessageQueue();

		/*! Стратегия при переполнении */
		enum LostStrategy
		{
			lostOldData, // default
			lostNewData
		};

		void setLostStrategy( LostStrategy s );

		// ---- Статистика ----
		/*! максимальное количество которое было в очереди сообщений */
		inline size_t getMaxQueueMessages() const
		{
			return stMaxQueueMessages;
		}

		/*! сколько раз очередь переполнялась */
		inline size_t getCountOfLostMessages() const
		{
			return stCountOfLostMessages;
		}

	protected:

		// заполнить всю очередь указанным сообщением
		void mqFill( const VoidMessagePtr& v );

		// для возможности тестирования переполнения
		// специально делается такая функция
		void set_wpos( unsigned long pos );
		void set_rpos( unsigned long pos );

	private:

		typedef std::vector<VoidMessagePtr> MQueue;

		MQueue mqueue;
		std::atomic_ulong wpos = { 0 }; // позиция на запись
		std::atomic_ulong rpos = { 0 }; // позиция на чтение
		std::atomic_ulong qpos = { 0 }; // текущая позиция последнего элемента (max position) (реально добавленного в очередь)

		LostStrategy lostStrategy = { lostOldData };

		/*! размер очереди сообщений (при превышении происходит очистка) */
		size_t SizeOfMessageQueue = { 2000 };

		// статистическая информация
		size_t stMaxQueueMessages = { 0 };    /*!< Максимальное число сообщений хранившихся в очереди */
		size_t stCountOfLostMessages = { 0 };    /*!< количество переполнений очереди сообщений */
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
