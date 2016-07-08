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
#include "MQMutex.h"
//--------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
//--------------------------------------------------------------------------
MQMutex::MQMutex( size_t qsize ):
	SizeOfMessageQueue(qsize)
{
}
//---------------------------------------------------------------------------
void MQMutex::push( const VoidMessagePtr& vm )
{
	std::lock_guard<std::mutex> lk(qmutex);

	size_t sz = mqueue.size();

	// проверяем переполнение, только если стратегия "терять новые данные"
	// иначе нет смысла проверять, а можно просто писать новые данные затирая старые
	// (sz+1) - т.к мы смотрим есть ли место для новых данных
	if( (sz+1) > SizeOfMessageQueue )
	{
		stCountOfLostMessages++;
		if( lostStrategy == lostNewData )
			return;

		// if( lostStrategy == lostOldData )
		mqueue.pop_front(); // удаляем одно старое, добавляем одно новое
		sz--;
	}

	mqueue.push_back(vm);
	sz++;
	if( sz > stMaxQueueMessages )
		stMaxQueueMessages = sz;
}
//---------------------------------------------------------------------------
VoidMessagePtr MQMutex::top()
{
	std::lock_guard<std::mutex> lk(qmutex);

	if( mqueue.empty() )
		return nullptr;

	auto m = mqueue.front();
	mqueue.pop_front();
	return m;
}
//---------------------------------------------------------------------------
size_t MQMutex::size()
{
	std::lock_guard<std::mutex> lk(qmutex);
	return mqueue.size();
}
//---------------------------------------------------------------------------
bool MQMutex::empty()
{
	std::lock_guard<std::mutex> lk(qmutex);
	return mqueue.empty();
}
//---------------------------------------------------------------------------
void MQMutex::setMaxSizeOfMessageQueue( size_t s )
{
	SizeOfMessageQueue = s;
}
//---------------------------------------------------------------------------
size_t MQMutex::getMaxSizeOfMessageQueue()
{
	return SizeOfMessageQueue;
}
//---------------------------------------------------------------------------
void MQMutex::setLostStrategy( MQMutex::LostStrategy s )
{
	lostStrategy = s;
}
//---------------------------------------------------------------------------
