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
// -------------------------------------------------------------------------
#include <unordered_map>
#include <map>
#include "MessageType.h"
#include "MQAtomic.h"
//--------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
//--------------------------------------------------------------------------
MQAtomic::MQAtomic( size_t qsize ):
	SizeOfMessageQueue(qsize)
{
	mqFill(nullptr);
}
//---------------------------------------------------------------------------
void MQAtomic::push( const VoidMessagePtr& vm )
{
	// проверяем переполнение, только если стратегия "терять новые данные"
	// иначе нет смысла проверять, а можно просто писать новые данные затирая старые
	if( lostStrategy == lostNewData && (wpos - rpos) >= SizeOfMessageQueue )
	{
		stCountOfLostMessages++;
		return;
	}

	// -----------------------------------------------
	// Если у нас wpos уже перешёл через максимум
	// то смотрим где rpos
	if( wpos < rpos )
	{
		// только надо привести к одному масштабу
		unsigned long w = wpos%SizeOfMessageQueue;
		unsigned long r = rpos%SizeOfMessageQueue;

		if( lostStrategy == lostNewData && (r-w) >= SizeOfMessageQueue )
		{
			stCountOfLostMessages++;
			return;
		}
	}
	// -----------------------------------------------

	// сперва надо сдвинуть счётчик (чтобы следующий поток уже писал в новое место)
	unsigned long w = wpos.fetch_add(1);

	// а потом уже добавлять новое сообщение в "зарезервированное" место
	mqueue[w%SizeOfMessageQueue] = vm;
	qpos.fetch_add(1); // теперь увеличиваем реальное количество элементов в очереди

	// ведём статистику
	size_t sz = qpos - rpos;
	if( sz > stMaxQueueMessages )
		stMaxQueueMessages = sz;
}
//---------------------------------------------------------------------------
VoidMessagePtr MQAtomic::top()
{
	// если стратегия "потеря старых данных"
	// то надо постоянно "подтягивать" rpos к wpos
	if( lostStrategy == lostOldData && (wpos - rpos) >= SizeOfMessageQueue )
	{
		stCountOfLostMessages++;
		rpos.store( wpos - SizeOfMessageQueue );
	}

	if( rpos == qpos )
		return nullptr;

	// смотрим именно qpos, а не wpos.
	// Т.к. qpos увеличивается только после помещения элемента в очередь
	// (помещение в вектор тоже занимает время)
	// иначе может случиться что wpos уже увеличился, но элемент ещё не поместили в очередь
	// а мы уже пытаемся читать.
	if( rpos < qpos )
	{
		// сперва надо сдвинуть счётчик (чтобы следующий поток уже работал с следующим значением)
		unsigned long r = rpos.fetch_add(1);
		return mqueue[r%SizeOfMessageQueue];
	}

	// Если rpos > qpos, значит qpos уже перешёл через максимум
	// И это особый случай обработки (пока rpos тоже не "перескочит" через максимум)
	if( rpos > qpos ) // делаем if каждый раз, т.к. qpos может уже поменяться в параллельном потоке
	{
		// приводим к одному масштабу
		unsigned long w = qpos%SizeOfMessageQueue;
		unsigned long r = rpos%SizeOfMessageQueue;

		if( lostStrategy == lostOldData && (r - w) >= SizeOfMessageQueue )
		{
			stCountOfLostMessages++;
			rpos.store(qpos - SizeOfMessageQueue); // "подтягиваем" rpos к qpos
		}

		// продолжаем читать как обычно
		r = rpos.fetch_add(1);
		return mqueue[r%SizeOfMessageQueue];
	}

	return nullptr;
}
//---------------------------------------------------------------------------
size_t MQAtomic::size() const
{
	// т.к. rpos корректируется только при фактическом вызое top()
	// то тут приходиться смотреть если у нас переполнение
	// то надо просто вернуть размер очереди
	// (т.к. фактически циклический буфер никогда не переполняется)
	if( (wpos - rpos) >= SizeOfMessageQueue )
		return SizeOfMessageQueue;

	return (qpos - rpos);
}
//---------------------------------------------------------------------------
bool MQAtomic::empty() const
{
	return (qpos == rpos);
}
//---------------------------------------------------------------------------
void MQAtomic::setMaxSizeOfMessageQueue( size_t s )
{
	if( s != SizeOfMessageQueue )
	{
		SizeOfMessageQueue = s;
		mqFill(nullptr);
	}
}
//---------------------------------------------------------------------------
size_t MQAtomic::getMaxSizeOfMessageQueue() const
{
	return SizeOfMessageQueue;
}
//---------------------------------------------------------------------------
void MQAtomic::setLostStrategy( MQAtomic::LostStrategy s )
{
	lostStrategy = s;
}
//---------------------------------------------------------------------------
void MQAtomic::mqFill( const VoidMessagePtr& v )
{
	mqueue.reserve(SizeOfMessageQueue);
	mqueue.clear();
	for( size_t i=0; i<SizeOfMessageQueue; i++ )
		mqueue.push_back(v);
}
//---------------------------------------------------------------------------
void MQAtomic::set_wpos( unsigned long pos )
{
	wpos = pos;
	qpos = pos;
}
//---------------------------------------------------------------------------
void MQAtomic::set_rpos( unsigned long pos )
{
	rpos = pos;
}
//---------------------------------------------------------------------------
