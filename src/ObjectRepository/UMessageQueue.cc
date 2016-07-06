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
#include "UMessageQueue.h"
//--------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
//--------------------------------------------------------------------------
UMessageQueue::UMessageQueue( size_t qsize ):
	SizeOfMessageQueue(qsize)
{
	mqFill(nullptr);
}
//---------------------------------------------------------------------------
void UMessageQueue::push( const UniSetTypes::TransportMessage& tm )
{
	// проверяем переполнение, только если стратегия "терять новые данные"
	// иначе нет смысла проверять, а можно просто писать новые данные затирая старые
	if( lostStrategy == lostNewData && (wpos - rpos) >= SizeOfMessageQueue )
	{
		stCountOfQueueFull++;
		return;
	}

	// сперва надо сдвинуть счётчик (чтобы следующий поток уже писал в новое место)
	size_t w = wpos.fetch_add(1);

	// а потом уже добавлять новое сообщение в "зарезервированное" место
	mqueue[w%SizeOfMessageQueue] = make_shared<VoidMessage>(tm);
	mpos++; // теперь увеличиваем реальное количество элементов в очереди

	// ведём статистику
	size_t sz = mpos - rpos;
	if( sz > stMaxQueueMessages )
		stMaxQueueMessages = sz;
}
//---------------------------------------------------------------------------
VoidMessagePtr UMessageQueue::top()
{
	// если стратегия "потеря старых данных"
	// то надо постоянно "подтягивать" rpos к wpos
	if( lostStrategy == lostOldData && (wpos - rpos) >= SizeOfMessageQueue )
	{
		stCountOfQueueFull++;
		rpos.store( wpos - SizeOfMessageQueue - 1 );
	}

	// смотрим "фактическое" количество (mpos)
	// т.к. помещение в вектор тоже занимает время
	// а при этом wpos у нас уже будет +1
	if( rpos < mpos )
	{
		// сперва надо сдвинуть счётчик (чтобы следующий поток уже читал новое)
		size_t r = rpos.fetch_add(1);

		// т.к. между if и этим местом, может придти другой читающий поток, то
		// проверяем здесь ещё раз
		if( r < mpos )
			return mqueue[r%SizeOfMessageQueue];
	}

	return nullptr;
}
//---------------------------------------------------------------------------
size_t UMessageQueue::size()
{
	// т.к. rpos корректируется только при фактическом вызое top()
	// то тут приходиться смотреть если у нас переполнение
	// то надо просто вернуть размер очереди
	// (т.к. фактически циклический буфер никогда не переполняется)
	if( (wpos - rpos) >= SizeOfMessageQueue )
		return SizeOfMessageQueue;

	return (mpos - rpos);
}
//---------------------------------------------------------------------------
void UMessageQueue::setMaxSizeOfMessageQueue( size_t s )
{
	if( s != SizeOfMessageQueue )
	{
		SizeOfMessageQueue = s;
		mqFill(nullptr);
	}
	else
		mqFill(nullptr);
}
//---------------------------------------------------------------------------
size_t UMessageQueue::getMaxSizeOfMessageQueue()
{
	return SizeOfMessageQueue;
}
//---------------------------------------------------------------------------
void UMessageQueue::setLostStrategy( UMessageQueue::LostStrategy s )
{
	lostStrategy = s;
}
//---------------------------------------------------------------------------
void UMessageQueue::mqFill( const VoidMessagePtr& v )
{
	mqueue.reserve(SizeOfMessageQueue);
	mqueue.clear();
	for( size_t i=0; i<SizeOfMessageQueue; i++ )
		mqueue.push_back(v);
}
//---------------------------------------------------------------------------
