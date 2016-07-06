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
#include <queue>
#include <vector>
#include <memory>
#include "Mutex.h"
#include "MessageType.h"
//--------------------------------------------------------------------------
typedef std::shared_ptr<UniSetTypes::VoidMessage> VoidMessagePtr;
//--------------------------------------------------------------------------
/*! \class UMessageQueue
 * Очередь сообщений.
 * \warning Очередь рассчитана на МНОГО ПИСАТЕЛЕЙ и ОДИН(!) читатель. Т.е. чтение должно быть из одного потока!
 * Сообщения извлекаются из очереди в порядке приоритета сообщения. При одинаковом приоритете - в порядке поступления в очередь.
 *
 * Максимальное ограничение на размер очереди сообщений задаётся функцией setMaxSizeOfMessageQueue().
 *
 * Контроль переполения очереди осуществляется в двух местах push и receiveMessage.
 * При переполнении очереди, происходит автоматическая очистка в два этапа.
 * Первый: производиться попытка "свёртки" сообщений.
 * Из очереди все повторяющиеся
 * - SensorMessage
 * - TimerMessage
 * - SystemMessage
 * Если это не помогло, то производиться второй этап "чистки":
 * Из очереди удаляется MaxCountRemoveOfMessage сообщений.
 * Этот парамер задаётся при помощи setMaxCountRemoveOfMessage(). По умолчанию 1/4 очереди сообщений.
 *
 * Очистка реализована в функции cleanMsgQueue();
 *
 * \warning Т.к. при фильтровании SensorMessage не смотрится значение,
 * то при удалении сообщений об изменении аналоговых датчиков очистка может привести
 * к некорректной работе фильрующих алгоритмов работающих с "выборкой" последних N значений.
 * (потому-что останется одно последнее)
 *
 *	ОПТИМИЗАЦИЯ N1:
 * Для того, чтобы функции push() и top() реже "сталкавались" на mutex-е очереди сообщений.
 * Сделано две очереди сообщений. Одна очередь сообщений наполняется в push() (с блокировкой mutex-а),
 * а вторая (без блокировки) обрабатывается в top(). Как только сообщения заканчиваются в
 * top() очереди меняются местами (при захваченном mutex).
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

		void setMaxCountRemoveOfMessage( size_t m );
		size_t getMaxCountRemoveOfMessage();

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

		// функция определения приоритетного сообщения для обработки
		struct VoidMessageCompare:
			public std::binary_function<VoidMessagePtr, VoidMessagePtr, bool>
		{
			bool operator()(const VoidMessagePtr& lhs,
							const VoidMessagePtr& rhs) const;
		};

		typedef std::priority_queue<VoidMessagePtr, std::vector<VoidMessagePtr>, VoidMessageCompare> MQueue;

	protected:

		/*! Чистка очереди сообщений */
		void cleanMsgQueue( MQueue& q );

	private:

		MQueue* wQ = { nullptr }; // указатель на текущую очередь на запись
		MQueue* rQ = { nullptr }; // указатель на текущую очередь на чтение

		MQueue mq1,mq2;

		/*! размер очереди сообщений (при превышении происходит очистка) */
		size_t SizeOfMessageQueue = { 2000 };

		/*! сколько сообщений удалять при очисте */
		size_t MaxCountRemoveOfMessage = { 500 };

		/*! замок для блокирования совместного доступа к очереди */
		UniSetTypes::uniset_rwmutex qmutex;

		// статистическая информация
		size_t stMaxQueueMessages = { 0 };    /*!< Максимальное число сообщений хранившихся в очереди */
		size_t stCountOfQueueFull = { 0 };    /*!< количество переполнений очереди сообщений */
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
