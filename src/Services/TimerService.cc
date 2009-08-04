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
 *  \author Pavel Vainerman
 *  \date   $Date: 2007/06/17 21:30:58 $
 *  \version $Id: TimerService.cc,v 1.8 2007/06/17 21:30:58 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#include <unistd.h>
#include <sstream>
#include <algorithm>

#include "TimerService.h"
#include "Debug.h"
#include "UniXML.h"
// ------------------------------------------------------------------------------------------
using namespace UniversalIO;
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
TimerService::TimerService(const string confNodeName):
MaxCountTimers(100),
AskLifeTimeSEC(60),
terminate(false),
isSleep(true),
sleepTimer(new ThrPassiveTimer())
{
	init(confNodeName);
}

TimerService::TimerService( ObjectId id, const string confNodeName ):
UniSetObject(id),
MaxCountTimers(100),
AskLifeTimeSEC(60),
terminate(false),
isSleep(true),
sleepTimer(new ThrPassiveTimer())
{
	init(confNodeName);
}

TimerService::~TimerService()
{
	if(!terminate)
		sleepTimer->terminate();
	if( exthread )
		delete exthread;
	delete sleepTimer;
}

// ------------------------------------------------------------------------------------------
/*
 * \param ti.timerid - идентификатор заказываемого таймера
 * \param ti.timeMS - интервал (0 - означает отказ от таймера)
 * \param ci.fromId - идентификатор заказчика
 * \param ci.node - узел, на котором находится заказчик
 * \exception TimerService_i::TimerAlreadyExist  - вырабатывается если от данного заказчика
 * \b уже \b есть заказ на таймер с таким идентификатором
*/
void TimerService::askTimer( const TimerService_i::Timer& ti, const UniSetTypes::ConsumerInfo& ci )
{
	if( ti.timeMS > 0 ) // заказ
	{
			if( tlst.size() >= MaxCountTimers )
			{
				TimerService_i::LimitTimers ex;
				ex.maxTimers = MaxCountTimers;
				throw ex;
			}
				
			if( ti.timeMS < UniSetTimer::MinQuantityTime )
			{
				TimerService_i::TimeMSLowLimit ex;
				ex.lowLimitMS = UniSetTimer::MinQuantityTime;
				throw ex;
			}
			
//			unideb[Debug::INFO] << "size: "<< tlst.size() << endl;	
			{	// lock
				if( !lstMutex.isRelease() )
					unideb[Debug::INFO] << myname << ": придется подождать освобождения lstMutex-а" << endl;
 				uniset_mutex_lock lock(lstMutex, 2000);

				// поищем а может уж такой есть
				if( !tlst.empty() )
				{
					for( TimersList::iterator li=tlst.begin(); li!=tlst.end(); ++li )
					{
						if ( li->cinf.id == ci.id && li->cinf.node == ci.node && li->id == ti.timerid )
						{
							li->curTick = ti.ticks;
							li->tmr.setTiming(ti.timeMS);
							li->not_ping = false;
							li->lifetmr.reset();
							unideb[Debug::INFO] << myname << ": заказ на таймер(id="<< ti.timerid << ") "
									<< ti.timeMS << " [мс] от " << ui.getNameById(ci.id)
									<< " уже есть... " << endl;	
							throw TimerService_i::TimerAlreadyExist();
						}
					}
				}

				TimerInfo newti(ti.timerid, ti.timeMS, ci, ti.ticks, AskLifeTimeSEC, (Message::Priority)ti.msgPriority);
				try
				{
					UniSetTypes::ObjectVar op = ui.resolve(ci.id,ci.node);
					newti.ref = UniSetObject_i::_narrow(op);
				}
				catch(...){}
		
				tlst.push_back(newti);
				tlst.sort();
				newti.reset();
			}	// unlock
		
			unideb[Debug::INFO] << myname << "(askTimer): поступил заказ на таймер(id="
				<< ti.timerid << ") " << ti.timeMS 
				<< " [мс] от " << ui.getNameById(ci.id,ci.node) << endl;	
	}
	else // отказ
	{
		unideb[Debug::INFO] << myname << ": поступил отказ от "
				<< ui.getNameById(ci.id,ci.node)
				<< " по таймеру id="<< ti.timerid << endl;	

		{	// lock
			if( !lstMutex.isRelease() )
				unideb[Debug::INFO] << myname << ": придется подождать освобождения lstMutex-а" << endl;
 			uniset_mutex_lock lock(lstMutex, 2000);
			tlst.remove_if(Timer_eq(ci,ti.timerid));	// STL - способ
			tlst.sort();
		}	// unlock
	}
	

	if( tlst.empty() )
		isSleep = true;
	else
	{
		isSleep = false;	
		sleepTimer->terminate();
	}

}
// ------------------------------------------------------------------------------------------
bool TimerService::send( TimerInfo& ti )
{
	TransportMessage tm = TimerMessage(ti.id, ti.priority, ti.cinf.id).transport_msg();
	for(int i=0; i<2; i++ )	// на каждый объект по две поптыки
	{
	    try
	    {
			if( CORBA::is_nil(ti.ref) )
			{
				UniSetTypes::ObjectVar op = ui.resolve(ti.cinf.id, ti.cinf.node);
				ti.ref = UniSetObject_i::_narrow(op);
			}

			ti.ref->push(tm);
			return true;	
	    }
		catch(...){}
//		unideb[Debug::WARN] << ui.getNameById( ti.cinf.id, ti.cinf.node ) << " недоступен!!" << endl;
		ti.ref=UniSetObject_i::_nil();
	}

	return false;
}
// ------------------------------------------------------------------------------------------
// Без повторной сортировки
void TimerService::work()
{
	execute_pid = getpid();
	bool resort = false;
	terminate = false;
//	TimerInfo* ti;
	int sleepTime(UniSetTimer::MIN_QUANTITY_TIME_MS);	// мс

	while(!terminate)
	{
		sleepTime = UniSetTimer::MIN_QUANTITY_TIME_MS; // мс
		{	// lock
			uniset_mutex_lock lock(lstMutex, 5000);
			resort = false;
			sleepTime = UniSetTimer::WaitUpTime;
			for( TimersList::iterator li=tlst.begin();li!=tlst.end();++li)
			{
				if( li->tmr.checkTime() )
				{
					if( !send(*li) )
					{
						if( !li->not_ping )
						{
							unideb[Debug::WARN] << myname << ": не смогли послать сообщение "<< ui.getNameById(li->cinf.id,li->cinf.node) << endl;
							if( !AskLifeTimeSEC )
							{
								unideb[Debug::WARN] << myname << ": удаляем из списка "<< ui.getNameById(li->cinf.id,li->cinf.node) << endl;
								li = tlst.erase(li);
								if( tlst.empty() )
									isSleep = true;
								continue;
							}

							li->not_ping = true;
							li->lifetmr.reset();
							li->reset();
						}
						else if( li->lifetmr.checkTime() )
						{	
							unideb[Debug::WARN] << myname << ": удаляем из списка "<< ui.getNameById(li->cinf.id,li->cinf.node) << endl;
							li = tlst.erase(li);
							if( tlst.empty() )
								isSleep = true;
							continue;
						}
					}
					else
					{
						li->not_ping = true;
						// Проверка на количество заданных тактов
						if( !li->curTick )
						{
							li = tlst.erase(li);
							if( tlst.empty() )
								isSleep = true;
							continue;
						}
						else if(li->curTick>0 )
							li->curTick--;
					}
						
					li->reset();
					resort = true;
				}
				else
				{
					li->curTimeMS -= sleepTime; 
					if( li->curTimeMS < 0)
						li->curTimeMS = 0;

				}
				
				if( li->curTimeMS < sleepTime || sleepTime == UniSetTimer::WaitUpTime )
					sleepTime = li->curTimeMS;
			}	

			/*! \warning и не оптимально, пересортировывать каждый раз весь список
			 * \todo потом можно написать самому более оптимальное решение
			 * т.к. список и так отсортирован, нужно всего лишь перемещать элемент
			 * в нужное место (у кого больше curTimeMS). Лучше поискать что-нибудь в 
			 * stl...
			 */
			if( resort )	// пересортировываем в связи с обновлением списка
				tlst.sort();
			
			if( sleepTime < UniSetTimer::MIN_QUANTITY_TIME_MS )
				sleepTime=UniSetTimer::MIN_QUANTITY_TIME_MS;

		} // unlock

		
		if( isSleep ) 
		{
			unideb[Debug::INFO] << myname << "(execute): нет активных таймеров... спим..." << endl;
			sleepTimer->wait(UniSetTimer::WaitUpTime);
		}
		else if( sleepTime ) 
			sleepTimer->wait(sleepTime); // msleep(sleepTime);
	}
	
	unideb[Debug::INFO] << myname << "(excecute): работу закончил..." << endl << flush;
	terminate = true;
}
// ------------------------------------------------------------------------------------------
bool TimerService::activateObject()
{
	if( !UniSetObject::activateObject() )
		return false;

	exthread = new ThreadCreator<TimerService>(this, &TimerService::work);
	exthread->start();
	return true;
}
// ------------------------------------------------------------------------------------------
bool TimerService::disactivateObject()
{
	unideb[Debug::INFO] << myname << ": disactivate..." << endl;
	if( !terminate )
	{
		terminate = true;
		sleepTimer->terminate();
	}
	unideb[Debug::INFO] << myname << ": disactivate ok." << endl;
	return true;	
}
// ------------------------------------------------------------------------------------------
void TimerService::sigterm( int signo )
{
	unideb[Debug::INFO] << myname << "(sigterm): sigterm..." << endl;
	terminate = true;
	sleepTimer->terminate();
	unideb[Debug::INFO] << myname << "(sigterm): ok.." << endl;	
	msleep(100);
}
// ------------------------------------------------------------------------------------------
/*
void TimerService::insert(TimerInfo& ti)
{
	for ( TimersList::iterator li=tlst.begin();li!=tlst.end();++li)
	{
//		if( (*li)->curTimeMS > ti.curTimeMS )
		if( (*li)->tmr.getInterval() > ti.tmr.getInterval() )
		{
			// вставляем между(перед ним) и выходим
			tlst.insert(li, ti);	
			return;
		}
	}
	
	tlst.push_back(ti);
}
*/
// ------------------------------------------------------------------------------------------
void TimerService::printList()
{
	for ( TimersList::iterator li=tlst.begin();li!=tlst.end();++li)
		cout <<  li->curTimeMS << endl;
}
// ------------------------------------------------------------------------------------------
void TimerService::init(const string& confnode)
{
	// инициализация из conf-файла
	xmlNode* node = conf->getNode(confnode);
	if(!node)
	{
		unideb[Debug::WARN] << myname << "(init): Не найден конфигурационный раздел " << confnode << endl;
		unideb[Debug::WARN] << myname << "(init): инициализируемся по умолчанию "
			<< "MaxCountTimers=" << MaxCountTimers 
			<< " AskLifeTimeSEC=" << AskLifeTimeSEC << endl;
		return;
	}

	MaxCountTimers = atoi(conf->getProp(node,"MaxCountTimers").c_str());
	AskLifeTimeSEC = atoi(conf->getProp(node,"AskLifeTimeSEC").c_str());
	
	assert( TimerService::MaxCountTimers > 0 );
	assert( TimerService::AskLifeTimeSEC >= 0 );
}
// ------------------------------------------------------------------------------------------
