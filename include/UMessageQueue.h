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
#include "MessageType.h"
//--------------------------------------------------------------------------
typedef std::shared_ptr<UniSetTypes::VoidMessage> VoidMessagePtr;
//--------------------------------------------------------------------------
/*! \class UMessageQueue
 * Очередь сообщений.
 *
 * Чтобы избежать работы с mutex, очередь построена по принципу циклического буфера,
 * c использованием atomic-переменных и попыткой реализовать LockFree работу.
 * Есть указатель на текущую позицию записи (wp) и есть "догоняющий его" указатель на позицию чтения (rp).
 * Если rp догоняет wp - значит новых сообщений нет.
 *
 * При этом место под очередь(буффер) резервируется сразу.
 * Счётчики сделаны (uint) монотонно растущими.
 * Основные идеи:
 * - счётчики постоянно увеличиваются
 * - каждый пишущий поток пишет в новое место
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
*/
class UMessageQueue
{
	public:
		UMessageQueue( size_t qsize = 2000 );

		void push( const UniSetTypes::TransportMessage& msg );

		VoidMessagePtr top();

		size_t size();

		// ----- Настройки  -----
		void setMaxSizeOfMessageQueue( size_t s );
		size_t getMaxSizeOfMessageQueue();

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

		typedef std::vector<VoidMessagePtr> MQueue;

	protected:

		// заполнить всю очередь указанным сообщением
		void mqFill( const VoidMessagePtr& v );

	private:

		MQueue mqueue;
		std::atomic_uint wpos = { 0 }; // позиция на запись
		std::atomic_uint rpos = { 0 }; // позиция на чтение
		std::atomic_uint mpos = { 0 }; // текущая позиция последнего элемента (max position) (реально добавленного в очередь)
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
