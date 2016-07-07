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
#ifndef UMessageQueue_H_
#define UMessageQueue_H_
//--------------------------------------------------------------------------
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include "MessageType.h"
//--------------------------------------------------------------------------
typedef std::shared_ptr<UniSetTypes::VoidMessage> VoidMessagePtr;
//--------------------------------------------------------------------------
/*! \class UMessageQueue
 * Очередь сообщений.
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
 * Счётчики сделаны (uint) монотонно растущими.
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
 *
 * Защита от переполнения индекса (wpos и rpos).
 * =============================================
 * Т.к. для обеспечения lockfree записи индексы (wpos) постоянно растут
 * (т.е. каждый пишущий поток пишет в новое место), в качестве atomic индекса выбран unsigned long.
 * Что для x86_32 системы работающей без перезагруки длительное время переполнение индекса может стать проблеммой.
 * Фактически же размер циклического буфера ограничен и запись ведётся в позицию [wpos%size], т.е.
 * в общем случае достаточно чтобы индекс был не больше размера буфера (size).
 * \error ПОКА ПРОБЛЕММА ПЕРЕПОЛНЕНИЯ НЕ РЕШЕНА..
 * чтобы "сбрасывать" индекс, нужно приостанавливать все пишущие и читающие потоки и одновременно
 * (одной транзакцией) менять wpos и rpos.
 *
*/
class UMessageQueue
{
	public:
		UMessageQueue( size_t qsize = 2000 );

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
		inline size_t getCountOfQueueFull() const
		{
			return stCountOfQueueFull;
		}

	protected:

		// заполнить всю очередь указанным сообщением
		void mqFill( const VoidMessagePtr& v );

	private:

		void resetIndexes();
		std::mutex resetMutex;

		typedef std::vector<VoidMessagePtr> MQueue;

		MQueue mqueue;
		std::atomic_ulong wpos = { 0 }; // позиция на запись
		std::atomic_ulong rpos = { 0 }; // позиция на чтение
		std::atomic_ulong mpos = { 0 }; // текущая позиция последнего элемента (max position) (реально добавленного в очередь)

		LostStrategy lostStrategy = { lostOldData };

		/*! размер очереди сообщений (при превышении происходит очистка) */
		size_t SizeOfMessageQueue = { 2000 };

		// статистическая информация
		size_t stMaxQueueMessages = { 0 };    /*!< Максимальное число сообщений хранившихся в очереди */
		size_t stCountOfQueueFull = { 0 };    /*!< количество переполнений очереди сообщений */
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
