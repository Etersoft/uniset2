/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 * \brief Реализация TimerService
 * \author Pavel Vainerman <pv>
 * \date $Date: 2007/06/17 21:30:56 $
 * \version $Id: TimerService.h,v 1.7 2007/06/17 21:30:56 vpashka Exp $
 */
//---------------------------------------------------------------------------
#ifndef TimerService_H_
#define TimerService_H_
//---------------------------------------------------------------------------
#include <list>
#include <functional>
#include <omnithread.h>
#include <string>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "TimerService_i.hh"
#include "PassiveTimer.h"
//#include "OmniThreadCreator.h"
#include "ThreadCreator.h"

//-----------------------------------------------------------------------------------
/*!
 \page ServicesPage
 \section secTimerService Сервис таймеров
 \subsection subTS_common Общие сведения
  Данный сервис предоставляет возможность заказа переодических сообщений 
  UniSetTypes::TimerMessage т.е. таймеров.
  Каждый заказчик может заказывать несколько таймеров, различающихся идентификаторами. При этом 
  идентификаторы определяет сам заказчик. Они должны быть уникальны только для одного заказчика.

  \note
  Сервис не гарантирует реальное время и точное соблюдение интервала. Т.к. к указанному в заказе времени 
 добавляется время на обработку и посылку сообщения, но при достаточных системных ресурсах точность увеличивается.
	  
 \subsection subTS_idea Сценарий работы
 	На узле запускается один экземпляр сервиса. Клиенты могут получить доступ, несколькими способами:
	- через NameService
	- при помощи UniversalInterface::askTimer()

	\par 	
	Сервис является системным, поэтому его идентификатор можно получить при помощи 
	UniSetTypes::Configuration::getTimerService() объекта UniSetTypes::conf.
	

	 \subsection subTS_interface Интерфейс
	Сервис предоставляет одну функцию 
	\code
		void TimerService::askTimer( CORBA::Short timerid, CORBA::Long timeMS, const UniSetTypes::ConsumerInfo& ci,
								CORBA::Short ticks)
	\endcode
		при помощи которой осуществялется заказ таймера. 
	\param timerid - уникальный идентификатор таймера
	\par
		В случае попытки заказать таймер с уже существующим (для данного заказчика) идентификатором вырабатывается
	 исключение TimerService_i::TimerAlreadyExist. 
	\param timeMS - период между уведомлениями. Для отказа от таймера необходимо указать timeMS=0.
	\param ticks позволяет ограничить количество уведомлений.
	\par
		Если ticks<0  уведомления будут посылатся, пока заказчик сам не откажется от них. 
	 Общее количество таймеров (\b на \b всех \b заказчиков) ограничено ( TimerService::MaxCountTimers ). 
	 В случае превышения данного предела на все заказы будет вырабатываться исключение TimerService_i::LimitTimers.
	 Для преодоления этого ограничения, а так же для обеспечения оптимальной работы сервиса, можно запускать на одном
	 узле несколько TimerService-ов для распределения нагрузки(заказов) между ними. При этом за распределение заказов
	 отвечает, разработчик.

	\subsection subTS_lifetime Время жизни заказа
		Для того, чтобы оптимизировать работу сервиса и уменьшить на него нагрузку вводится понятие
	"время жизни заказа". В случае, если в течение TimerService::AskLifeTimeSEC не удаётся послать заказчику
	уведомление, в следствие его недоступности - заказ анулируется.
	
	\note Параметры можно задавать в конфигурационном файле
	Реализацию см. \ref TimerService
*/

 
/*! \class TimerService
 * Построен на основе PassiveTimer. 
*/
class TimerService: 
	public POA_TimerService_i,
	public UniSetObject
{
	public:
		
		TimerService( UniSetTypes::ObjectId id, const std::string confNodeName="LocalTimerService");
	    ~TimerService();

		//! заказ таймера
		virtual void askTimer( const TimerService_i::Timer& ti, const UniSetTypes::ConsumerInfo& ci);
		void printList();

	protected:
		TimerService(const std::string confNodeName="LocalTimerService");

		/*! Информация о таймере */
		struct TimerInfo
		{
			TimerInfo():id(0), curTimeMS(0){};
			TimerInfo(CORBA::Short id, CORBA::Long timeMS, const UniSetTypes::ConsumerInfo& cinf, 
						CORBA::Short cnt, unsigned int askLifeTime, UniSetTypes::Message::Priority p):
				cinf(cinf),
				ref(0),
				id(id), 
				curTimeMS(timeMS),
				priority(p),
				curTick(cnt-1),
				not_ping(false)
			{
				tmr.setTiming(timeMS);
				lifetmr.setTiming(askLifeTime*1000); // [сек]
			};
			
			inline void reset()
			{
				curTimeMS = tmr.getInterval();
				tmr.reset();
			}
			
			UniSetTypes::ConsumerInfo cinf;		/*!<  инфою о заказчике */
			UniSetObject_i_var ref;				/*!<  ссылка заказчика */
			UniSetTypes::TimerId id;			/*!<  идентификатор таймера */
			int curTimeMS;						/*!<  остаток времени */
			UniSetTypes::Message::Priority priority;	/*!<  приоритет посылаемого сообщения */

			/*!
			 * текущий такт
			 * \note Если задано количество -1 то сообщения будут поылатся постоянно
			*/
			CORBA::Short curTick; 
			
			// заказчик с меньшим временем ожидания имеет больший приоритет
		   	bool operator < ( const TimerInfo& ti ) const
    		{ 
				return curTimeMS > ti.curTimeMS; 
			}

			PassiveTimer tmr;
			PassiveTimer lifetmr;	/*! таймер жизни заказа в случае если объект не доступен */
			bool not_ping;			/* признак недоступности заказчика */
		};

		typedef std::list<TimerInfo> TimersList; 			
		
		//! посылка сообщения о наступлении времени
		virtual bool send(TimerInfo& ti);

		//! Дизактивизация объекта (переопределяется для необходимых действий перед деактивацией)	 
		virtual bool disactivateObject();
		virtual bool activateObject();
		virtual void sigterm( int signo );


		unsigned int MaxCountTimers; 	/*!< максимально возможное количество таймеров */	
		unsigned int AskLifeTimeSEC; 	/*!< [сек] время жизни заказа, если объект недоступен */	

		void init(const std::string& confnode);
		void work();

	private:

		bool terminate;
		bool isSleep;
		
		UniSetTimer* sleepTimer;	/*!< таймер для реализации засыпания в отсутствие заказов */
		class Timer_eq: public std::unary_function<TimerInfo, bool>
		{
			public:			
				Timer_eq(const UniSetTypes::ConsumerInfo& coi, CORBA::Short t):tid(t),ci(coi){}

			inline bool operator()(const TimerInfo& ti) const
			{
				return ( ti.cinf.id == ci.id && ti.cinf.node == ci.node && ti.id == tid );
			}

			protected:
				UniSetTypes::TimerId tid;
				UniSetTypes::ConsumerInfo ci;
		};

		TimersList tlst; 				
		/*! замок для блокирования совместного доступа к cписку таймеров */			
		UniSetTypes::uniset_mutex lstMutex; 
		int execute_pid; 
		ThreadCreator<TimerService>* exthread;		
};

#endif
