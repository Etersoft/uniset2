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
*/
// -------------------------------------------------------------------------- 

#include <sstream>
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>

#include "UniversalInterface.h"
#include "IONotifyController.h"
#include "Debug.h"
#include "NCRestorer.h"

// ------------------------------------------------------------------------------------------
using namespace UniversalIO;
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
IONotifyController::IONotifyController():
restorer(NULL),
askDMutex("askDMutex"),
askAMutex("askAMutex"),
trshMutex("trshMutex"),
maxAttemtps(conf->getPIntField("ConsumerMaxAttempts", 5))
{
	
}

IONotifyController::IONotifyController(const string name, const string section, NCRestorer* d ): 
	IOController(name, section),
	restorer(d),
	askDMutex(name+"askDMutex"),
	askAMutex(name+"askAMutex"),
	trshMutex(name+"trshMutex"),
	maxAttemtps(conf->getPIntField("ConsumerMaxAttempts", 5))
{
	// добавляем фильтры
	addAFilter( sigc::mem_fun(this,&IONotifyController::myAFilter) );
	addDFilter( sigc::mem_fun(this,&IONotifyController::myDFilter) );
	setDependsSlot( sigc::mem_fun(this,&IONotifyController::onChangeUndefined) );
}

IONotifyController::IONotifyController( ObjectId id, NCRestorer* d ):
	IOController(id),
	restorer(d),
	askDMutex(string(conf->oind->getMapName(id))+"_askDMutex"),
	askAMutex(string(conf->oind->getMapName(id))+"_askAMutex"),
	trshMutex(string(conf->oind->getMapName(id))+"_trshMutex"),
	maxAttemtps(conf->getPIntField("ConsumerMaxAttempts", 5))
{
	// добавляем фильтры
	addAFilter( sigc::mem_fun(this,&IONotifyController::myAFilter) );
	addDFilter( sigc::mem_fun(this,&IONotifyController::myDFilter) );
	setDependsSlot( sigc::mem_fun(this,&IONotifyController::onChangeUndefined) );
}

IONotifyController::~IONotifyController()
{
}

// ------------------------------------------------------------------------------------------
// функция-объект для поиска
// !!!! для ассоциативных контейнеров должна возвращать false 
// в случае равенства!!!!!!!!!!! (проверка эквивалетности. а не равенства)
/*
struct FindCons_eq: public unary_function<UniSetTypes::ConsumerInfo, bool>
{
	FindCons_eq(const UniSetTypes::ConsumerInfo& ci):ci(ci){}
	inline bool operator()(const UniSetTypes::ConsumerInfo& c) const
	{
		return !( ci.id==c.id && ci.node==c.node );
	}
	UniSetTypes::ConsumerInfo ci;
}
*/

// ------------------------------------------------------------------------------------------
/*!
 *	\param lst - указатель на список в который необходимо внести потребителя
 *	\param name - имя вносимого потребителя
 *	\note Добавление произойдет только если такого потребителя не существует в списке
*/
bool IONotifyController::addConsumer(ConsumerList& lst, const ConsumerInfo& ci )
{
//	ConsumerList::const_iterator it= find_if(lst.begin(), lst.end(), FindCons_eq(ci));
//	if(it != lst.end() )
//		return;
	for( ConsumerList::const_iterator it=lst.begin(); it!=lst.end(); ++it) 
	{
		if( it->id==ci.id && it->node==ci.node )
			return false;
	}

	ConsumerInfoExt cinf(ci,0,maxAttemtps);
	// получаем ссылку
	try
	{
		UniSetTypes::ObjectVar op = ui.resolve(ci.id,ci.node);
		cinf.ref = UniSetObject_i::_narrow(op);
	}
	catch(...){}
	
	lst.push_front(cinf);
	return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *	\param lst - указатель на список из которго происходит удаление потребителя
 *	\param name - имя удаляемого потребителя
*/
bool IONotifyController::removeConsumer(ConsumerList& lst, const ConsumerInfo& cons )
{
	for( ConsumerList::iterator li=lst.begin();li!=lst.end();++li)
	{
//		ConsumerInfo tmp(*li);
//		if( cons == tmp )
		if( li->id == cons.id && li->node == cons.node  )		
		{
			lst.erase(li);
//			unideb[Debug::INFO] << name.c_name() <<": удаляем "<< name << " из списка потребителей" << endl;
			return true;
		}
	}
	
	return false;
}

// ------------------------------------------------------------------------------------------
/*! 
 *	\param si 		- информация о датчике
 *	\param ci	 	- информация о заказчике
 *	\param cmd 		- команда см. UniversalIO::UIOCommand
*/
void IONotifyController::askSensor(const IOController_i::SensorInfo& si, const UniSetTypes::ConsumerInfo& ci, 
				UniversalIO::UIOCommand cmd)
{
	IOTypes type = IOController::getIOType(si);
	switch(type)
	{
		case UniversalIO::DigitalInput:
			askState(si,ci,cmd);
		break;;

		case UniversalIO::AnalogInput:
			askValue(si,ci,cmd);
		break;
		
		case UniversalIO::AnalogOutput:
		case UniversalIO::DigitalOutput:
			askOutput(si,ci,cmd);
		break;
		
		default:
		{
			ostringstream err;
			err << myname << "(askSensor): Неизвестен тип для " << conf->oind->getNameById(si.id);
			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << err.str() << endl;
			throw IOController_i::NameNotFound(err.str().c_str());
		}
		break;
	}
}
				
// ------------------------------------------------------------------------------------------
/*! 
 *	\param si 		- информация о датчике
 *	\param ci	 	- информация о заказчике
 *	\param cmd 		- команда см. UniversalIO::UIOCommand
*/
void IONotifyController::askState( const IOController_i::SensorInfo& si, 
									const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd )
{
	// провреки на несуществующий датчик проводить не надо т.к. заказчик принципиально
	// не может обратится к этому контроллеру по ссылке на другой датчик
	// (ведь ссылка на датчик это ссылка на контроллер который за него отвечает)
	// контроль заказа типа датчика(дискретного) здесь производится
	if( unideb.debugging(Debug::INFO) )
	{
		unideb[Debug::INFO] << "(askState): поступил " << ( cmd == UIODontNotify ? "отказ" :"заказ" ) 
			<< " от "
			<< conf->oind->getNameById(ci.id, ci.node) << " на дискретный датчик "
			<< conf->oind->getNameById(si.id,si.node) << endl;
	}
	
	// если такого дискретного датчика нет, здесь сработает исключение...
	DIOStateList::iterator li = mydioEnd();
	localGetState(li,si);
	// lock ???
	if( li==mydioEnd() )
	{
		ostringstream err;
		err << myname << "(askState): датчик имя: " << conf->oind->getNameById(si.id) << " не найден";
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	if( li->second.type != UniversalIO::DigitalInput )
	{
		ostringstream err;
		err << myname << "(askState): ВХОДНОЙ ДИСКРЕТНЫЙ ДАТЧИК с именем " << conf->oind->getNameById(si.id) << " не найден";
		if( unideb.debugging(Debug::INFO) )	
			unideb[Debug::INFO] << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	{	//lock
		uniset_mutex_lock lock(askDMutex, 200);
		// а раз есть заносим(исключаем) заказчика 
		ask( askDIOList, si, ci, cmd);		
	} // unlock			

	// посылка первый раз состояния 
	if( cmd==UniversalIO::UIONotify || (cmd==UIONotifyFirstNotNull && li->second.state) )
	{
		SensorMessage  smsg;
		smsg.id 		= si.id;
		smsg.node 		= si.node;
		smsg.consumer 	= ci.id;
		smsg.sensor_type= li->second.type;	
		smsg.priority	= (Message::Priority)li->second.priority;
		smsg.supplier 	= getId();
		{
			uniset_spin_lock lock(li->second.val_lock,getCheckLockValuePause());
			smsg.state 		= li->second.state;
			smsg.value 		= li->second.state ? 1:0;
			smsg.undefined	= li->second.undefined;
			smsg.sm_tv_sec	= li->second.tv_sec;
			smsg.sm_tv_usec	= li->second.tv_usec;
		}

		TransportMessage tm(smsg.transport_msg());
	    try
	    {
			ui.send(ci.id, tm, ci.node);
		}
		catch(Exception& ex)
		{
		   	unideb[Debug::WARN] << myname << "(askState): " 
		   		<< conf->oind->getNameById(si.id, si.node) << " "<< ex << endl;
		}
	    catch( CORBA::SystemException& ex )
	    {
    		unideb[Debug::WARN] << conf->oind->getNameById(ci.id, ci.node) << " недоступен!!(CORBA::SystemException): "
				<< ex.NP_minorString() << endl;
	    }
		catch(...){}
	}
}

// ------------------------------------------------------------------------------------------
/*! 
 *	\param si 		- информация о датчике
 *	\param ci	 	- информация о заказчике
 *	\param cmd 		- команда см. UniversalIO::UIOCommand
*/
void IONotifyController::askValue(const IOController_i::SensorInfo& si, 
									const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd )
{
	// провреки на несуществующий датчик проводить не надо т.к. заказчик ппинципиально
	// не может обратится к этому контроллеру по ссылке на другой датчик
	// (ведь ссылка на датчик это ссылка на контроллер который за него отвечает)
	// контроль заказа именно АНАЛОГОВО датчика производится

	if( unideb.debugging(Debug::INFO) )	
	{
		unideb[Debug::INFO] << "(askValue): поступил " << ( cmd == UIODontNotify ? "отказ" :"заказ" ) << " от "
			<< conf->oind->getNameById(ci.id, ci.node)
			<< " на аналоговый датчик "
			<< conf->oind->getNameById(si.id,si.node) << endl;
	}
	
	// если такого аналогового датчика нет, здесь сработает исключение...
	AIOStateList::iterator li = myaioEnd();
	localGetValue(li,si);
	if( li->second.type != UniversalIO::AnalogInput )
	{
		ostringstream err;
		err << myname << "(askState): ВХОДНОЙ АНАЛОГОВЫЙ ДАТЧИК с именем " << conf->oind->getNameById(si.id) 
			<< " не найден";
		if( unideb.debugging(Debug::INFO) )	
			unideb[Debug::INFO] << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	{	// lock
 		uniset_mutex_lock lock(askAMutex, 200);
		// а раз есть заносим(исключаем) заказчика 
		ask( askAIOList, si, ci, cmd);		
	}	// unlock

	// посылка первый раз состояния 
	if( cmd==UniversalIO::UIONotify || (cmd==UIONotifyFirstNotNull && li->second.value) )
	{
		SensorMessage  smsg;
		smsg.id 		= si.id;
		smsg.node 		= si.node;
		smsg.consumer 	= ci.id;
		smsg.supplier 	= getId();
		smsg.sensor_type= li->second.type;	
		smsg.priority	= (Message::Priority)li->second.priority;
		smsg.sm_tv_sec	= li->second.tv_sec;
		smsg.sm_tv_usec	= li->second.tv_usec;
		smsg.ci			= li->second.ci;
		{
			uniset_spin_lock lock(li->second.val_lock,getCheckLockValuePause());			
			smsg.value 		= li->second.value;
			smsg.state		= li->second.value ? true:false;
			smsg.undefined	= li->second.undefined;
			smsg.sm_tv_sec	= li->second.tv_sec;
			smsg.sm_tv_usec	= li->second.tv_usec;
		}
			
		TransportMessage tm(smsg.transport_msg());			
	    try
	    {
			ui.send(ci.id, tm, ci.node);
		}
		catch(Exception& ex)
		{
		   	unideb[Debug::WARN] << myname << "(askValue): " <<  conf->oind->getNameById(si.id, si.node) << " catch "<< ex << endl;
		}
	    catch( CORBA::SystemException& ex )
	    {
	    	unideb[Debug::WARN] << conf->oind->getNameById(ci.id, ci.node) 
	    		<< " недоступен!!(CORBA::SystemException): "
				<< ex.NP_minorString() << endl;
	    }	
		catch(...){}
	}
}

// ------------------------------------------------------------------------------------------
void IONotifyController::ask(AskMap& askLst, const IOController_i::SensorInfo& si, 
								const UniSetTypes::ConsumerInfo& cons, UniversalIO::UIOCommand cmd)
{
	// поиск датчика в списке 
	UniSetTypes::KeyType k( key(si.id,si.node) );
	AskMap::iterator askIterator = askLst.find(k);

  	switch (cmd)
	{
		case UniversalIO::UIONotify: // заказ
		case UniversalIO::UIONotifyChange:
		case UniversalIO::UIONotifyFirstNotNull:
		{
   			if( askIterator==askLst.end() ) 
			{
				ConsumerList lst; // создаем новый список
				addConsumer(lst,cons);	  
				// более оптимальный способ(при условии вставки первый раз) //	askLst[key]=lst;
				askLst.insert(AskMap::value_type(k,lst));
				
				try
				{
					dumpOrdersList(si,lst);
				}
				catch(Exception& ex)
				{
					unideb[Debug::WARN] << myname << " не смогли сделать dump: " << ex << endl;
				}
				catch(...)
				{
			    	unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
				}
		    }
			else
			{
				if( addConsumer(askIterator->second,cons) )
				{
					try
					{
						dumpOrdersList(si,askIterator->second);
					}
					catch(Exception& ex)
					{
						unideb[Debug::WARN] << myname << " не смогли сделать dump: " << ex << endl;
					}
					catch(...)
					{	
			    		unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
					}
				}
		    }
			break;
		}
		case UniversalIO::UIODontNotify: 	// отказ
		{
			if( askIterator!=askLst.end() )	// существует
			{
//				ConsumerList lst(askIterator->second);
				if( removeConsumer(askIterator->second, cons) )
				{
					if( askIterator->second.empty() )
						askLst.erase(askIterator);	
					else
					{
						try
						{
							dumpOrdersList(si,askIterator->second);
						}
						catch(Exception& ex)
						{
							unideb[Debug::WARN] << myname << " не смогли сделать dump: " << ex << endl;
						}
						catch(...)
						{		
				    		unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
						}
					}
				}
			}
			break;
		}
	
		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
bool IONotifyController::myDFilter(const UniDigitalIOInfo& di, 
									CORBA::Boolean newstate, UniSetTypes::ObjectId sup_id)
{
	return ( di.state == newstate ) ? false : true;
}
// ------------------------------------------------------------------------------------------
bool IONotifyController::myAFilter(const UniAnalogIOInfo& ai, 
									CORBA::Long newvalue, UniSetTypes::ObjectId sup_id)
{
	if( ai.value == newvalue )
		return false;
	
	if( ai.ci.sensibility <= 0 )
		return true;

	if( abs(ai.value - newvalue) < ai.ci.sensibility )
		return false;
	
	return true;
}
// ------------------------------------------------------------------------------------------
void IONotifyController::localSaveState( IOController::DIOStateList::iterator& it,
											const IOController_i::SensorInfo& si, 
											CORBA::Boolean state,
											UniSetTypes::ObjectId sup_id )
{
	// Если датчик не найден здесь сработает исключение NameNotFound
	bool prevState = IOController::localGetState( it, si );

	IOController::localSaveState( it, si, state, sup_id );

	// сравниваем именно с li->second.state
	// т.к. фактическое сохранённое значение может быть изменено
	// фильтрами или блокировками..
	SensorMessage sm(si.id, state);
	{	// lock
		uniset_spin_lock lock(it->second.val_lock,getCheckLockValuePause());
		if( prevState == it->second.state )
			return;

		// Уведомления рассылаем только в случае смены состояния...
		sm.id 			= si.id;
		sm.node 		= si.node;
		sm.state 		= it->second.state;
		sm.value 		= it->second.state ? 1:0;
		sm.undefined	= it->second.undefined;
		sm.priority		= (Message::Priority)it->second.priority;
		sm.supplier		= sup_id;
		sm.sensor_type 	= it->second.type;	
		sm.sm_tv_sec	= it->second.tv_sec;
		sm.sm_tv_usec	= it->second.tv_usec;
	}	// unlock

	try
	{	
		uniset_mutex_lock l(sig_mutex,500);
		changeSignal.emit(&sm);
	}
	catch(...){}

	try
	{	
		if( !it->second.db_ignore )
			loggingInfo(sm);
	}
	catch(...){}

	AskMap::iterator it1 = askDIOList.find( key(si.id,si.node) );
	if( it1!=askDIOList.end() )
	{	// lock
		uniset_mutex_lock lock(askDMutex, 1000);
		send(it1->second, sm);
	}	// unlock
}
// ------------------------------------------------------------------------------------------
void IONotifyController::localSaveValue( IOController::AIOStateList::iterator& li,
										const IOController_i::SensorInfo& si, 
										CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	// Если датчик не найден сдесь сработает исключение
	long prevValue = IOController::localGetValue( li, si );
	if( li == myaioEnd() ) // ???
	{
		ostringstream err;
		err << myname << "(localSaveValue): аналоговый вход(выход) с именем " 
						<< conf->oind->getNameById(si.id) << " не найден";

		if( unideb.debugging(Debug::INFO) )	
			unideb[Debug::INFO] << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	IOController::localSaveValue(li,si, value,sup_id);

	// сравниваем именно с li->second.value
	// т.к. фактическое сохранённое значение может быть изменено
	// фильтрами или блокировками..
	SensorMessage sm(si.id,li->second.value);
	{ // lock
		uniset_spin_lock lock(li->second.val_lock,getCheckLockValuePause());
		
		if( prevValue == li->second.value )
			return;

		// Рассылаем уведомления только в слуае изменения значения
		sm.id 			= si.id;
		sm.node 		= si.node;
		sm.state 		= li->second.value!=0 ? true:false;
		sm.value 		= li->second.value;
		sm.undefined	= li->second.undefined;
		sm.priority		= (Message::Priority)li->second.priority;
		sm.supplier		= sup_id;
		sm.sensor_type 	= li->second.type;
		sm.sm_tv_sec	= li->second.tv_sec;
		sm.sm_tv_usec	= li->second.tv_usec;
		sm.ci			= li->second.ci;
	} // unlock

	try
	{	
		uniset_mutex_lock l(sig_mutex,500);
		changeSignal.emit(&sm);
	}
	catch(...){}

	try
	{	
		if( !li->second.db_ignore )
			loggingInfo(sm);
	}
	catch(...){}

	AskMap::iterator it = askAIOList.find( key(si.id,si.node) );
	if( it!=askAIOList.end() )
	{	// lock
		uniset_mutex_lock lock(askAMutex, 1000);
		send(it->second, sm);
	}

	// проверка порогов
	try
	{	
		checkThreshold(li,si,true);
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------

/*!
	\note В случае зависания в функции push, будут остановлены рассылки другим объектам.
	Возможно нужно ввести своего агента на удалённой стороне, который будет заниматься
	только приёмом сообщений и локальной рассылкой. Lav
*/
void IONotifyController::send(ConsumerList& lst, UniSetTypes::SensorMessage& sm)
{
	for( ConsumerList::iterator li=lst.begin();li!=lst.end();++li )
	{
		for(int i=0; i<2; i++ )	// на каждый объект по две поптыки
		{
			try
			{
				if( CORBA::is_nil(li->ref) )
				{
					CORBA::Object_var op = ui.resolve(li->id, li->node);
					li->ref = UniSetObject_i::_narrow(op);
				}

				sm.consumer = li->id;
				li->ref->push( sm.transport_msg() );
				li->attempt = maxAttemtps; // reinit attempts
				break;
			}
			catch(Exception& ex)
			{
			   	unideb[Debug::WARN] << myname << "(IONotifyController::send): " << ex
						<< " for " << conf->oind->getNameById(li->id, li->node) << endl;
			}
		    catch( CORBA::SystemException& ex )
		    {
		    	unideb[Debug::WARN] << myname << "(IONotifyController::send): " 
					<< conf->oind->getNameById(li->id, li->node) << " (CORBA::SystemException): "
					<< ex.NP_minorString() << endl;
	    	}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(IONotifyController::send): "
					<< conf->oind->getNameById(li->id, li->node) 
					<< " catch..." << endl;
			}
			
			if( maxAttemtps>0 &&  (--li->attempt <= 0) )
			{
				li = lst.erase(li);
				break;
			}

			li->ref = UniSetObject_i::_nil();
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::loggingInfo(UniSetTypes::SensorMessage& sm)
{
	IOController::logging(sm);
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::activateObject()
{
	IOController::activateObject();
	readDump();
	buildDependsList();
	return true;
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::readDump()
{
	try
	{
		if( restorer != NULL )
			restorer->read(this);
	}
	catch(Exception& ex)
	{ 
		unideb[Debug::WARN] << myname << "(IONotifyController::readDump): " << ex << endl;
	}
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::dumpOrdersList(const IOController_i::SensorInfo& si, 
											const IONotifyController::ConsumerList& lst)
{
	if( restorer == NULL )
		return;

	try
	{
		NCRestorer::SInfo inf;
		UniversalIO::IOTypes t(getIOType(si));
		switch( t )
		{
			case UniversalIO::DigitalInput:
			case UniversalIO::DigitalOutput:
			{
				IOController_i::DigitalIOInfo dinf(getDInfo(si));
				inf=dinf;
			}
			break;
		
			case UniversalIO::AnalogOutput:
			case UniversalIO::AnalogInput:
			{
				IOController_i::AnalogIOInfo ainf(getAInfo(si));
				inf=ainf;
			}
			break;
			
			default:
				return;
		}

		restorer->dump(this,inf,lst);
	}
	catch(Exception& ex)
	{ 
		unideb[Debug::WARN] << myname << "(IONotifyController::dumpOrderList): " << ex << endl;
	}
}
// --------------------------------------------------------------------------------------------------------------

void IONotifyController::dumpThresholdList(const IOController_i::SensorInfo& si, const IONotifyController::ThresholdExtList& lst)
{
	if( restorer == NULL )
		return;

	try
	{
		NCRestorer::SInfo inf;
		UniversalIO::IOTypes t(getIOType(si));
		switch( t )
		{
			case UniversalIO::DigitalInput:
			case UniversalIO::DigitalOutput:
			{
				IOController_i::DigitalIOInfo dinf(getDInfo(si));
				inf=dinf;
			}
			break;
		
			case UniversalIO::AnalogOutput:
			case UniversalIO::AnalogInput:
			{
				IOController_i::AnalogIOInfo ainf(getAInfo(si));
				inf=ainf;
			}
			break;
			
			default:
				return;
		}
		restorer->dumpThreshold(this,inf,lst);
	}
	catch(Exception& ex)
	{ 
		unideb[Debug::WARN] << myname << "(IONotifyController::dumpThresholdList): " << ex << endl;
	}
}
// --------------------------------------------------------------------------------------------------------------

void IONotifyController::askThreshold(const IOController_i::SensorInfo& si, const UniSetTypes::ConsumerInfo& ci, 
									UniSetTypes::ThresholdId tid,
									CORBA::Long lowLimit, CORBA::Long hiLimit, CORBA::Long sb,
									UniversalIO::UIOCommand cmd )
{
	if( lowLimit > hiLimit )
		throw IONotifyController_i::BadRange();

	// если такого дискретного датчика нет сдесь сработает исключение...
	AIOStateList::iterator li = myaioEnd();
	CORBA::Long val = localGetValue(li,si);

	{	// lock
		uniset_mutex_lock lock(trshMutex, 300);

		// поиск датчика в списке 
		UniSetTypes::KeyType skey( key(si.id,si.node) );
		AskThresholdMap::iterator it = askTMap.find(skey);
		ThresholdInfoExt ti(tid,lowLimit, hiLimit,sb);
		ti.itSID = mydioEnd();

	  	switch( cmd )
		{
			case UniversalIO::UIONotify: // заказ
			case UniversalIO::UIONotifyChange:
			{
   				if( it==askTMap.end() ) 
				{
					ThresholdExtList lst;	// создаем новый список
					ThresholdsListInfo tli;
					tli.si 		= si;
					tli.list 	= lst;
					tli.type 	= li->second.type;
					tli.ait		= myaioEnd();
					addThreshold(lst,ti,ci);
					askTMap.insert(AskThresholdMap::value_type(skey,tli));
					try
					{
						dumpThresholdList(si,lst);
					}
					catch(Exception& ex)
					{
						unideb[Debug::WARN] << myname << " не смогли сделать dump: " << ex << endl;
					}
					catch(...)
					{	
			    		unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
					}
			    }
				else
				{
					if( addThreshold(it->second.list,ti,ci) )
					{
						try
						{
							dumpThresholdList(si,it->second.list);
						}
						catch(Exception& ex)
						{
							unideb[Debug::WARN] << myname << "(askThreshold): dump: " << ex << endl;
						}
						catch(...)
						{	
				    		unideb[Debug::WARN] << myname << "(askThreshold): dump catch..." << endl;
						}
					}
				}

				if( cmd == UniversalIO::UIONotifyChange )
					break;
				
				// посылка первый раз состояния 
			    try
			    {
					SensorMessage sm;
					sm.id 			= si.id;
					sm.node 		= si.node;
					sm.value 		= val;
					sm.state 		= val!=0 ? true:false;
					sm.undefined	= li->second.undefined;
					sm.sensor_type 	= li->second.type;
					sm.priority 	= (Message::Priority)li->second.priority;
					sm.consumer 	= ci.id;
					sm.tid 			= tid;
					sm.sm_tv_sec	= ti.tv_sec;
					sm.sm_tv_usec	= ti.tv_usec;
					sm.ci			= li->second.ci;
	
						// Проверка нижнего предела
					if( val <= (lowLimit-sb) )
					{
						sm.threshold = false;
						CORBA::Object_var op = ui.resolve(ci.id, ci.node);
						UniSetObject_i_var ref = UniSetObject_i::_narrow(op);
						if(!CORBA::is_nil(ref))
							ref->push(sm.transport_msg());
					}
					// Проверка верхнего предела
					else if( val >= (hiLimit+sb) )
					{
						sm.threshold = true;
						CORBA::Object_var op = ui.resolve(ci.id, ci.node);
						UniSetObject_i_var ref = UniSetObject_i::_narrow(op);
						if(!CORBA::is_nil(ref))
							ref->push(sm.transport_msg());
					}
				}
				catch(Exception& ex)
				{
				   	unideb[Debug::WARN] << myname << "(askThreshod): " << ex << endl;
				}
			    catch( CORBA::SystemException& ex )
			    {
			    	unideb[Debug::WARN] << myname << "(askThreshod): CORBA::SystemException: "
						<< ex.NP_minorString() << endl;
			    }	
				catch(...){}
		    }
			break;

			case UniversalIO::UIODontNotify: 	// отказ
			{
				if( it!=askTMap.end() )
				{
					if(	removeThreshold(it->second.list,ti,ci) )
					{
						try
						{
							dumpThresholdList(si,it->second.list);
						}
						catch(Exception& ex)
						{
							unideb[Debug::WARN] << myname << "(askThreshold): dump: " << ex << endl;
						}
						catch(...)
						{	
				    		unideb[Debug::WARN] << myname << "(askThreshold): dump catch..." << endl;
						}
					}
				}
			}
			break;

			default:
				break;
		}
	}	// unlock

}									
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::addThreshold(ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& ci)
{
	for( ThresholdExtList::iterator it=lst.begin(); it!=lst.end(); ++it) 
	{
		if( ti==(*it) )
		{
			if( addConsumer(it->clst, ci) )
			{
				ti.clst = it->clst;
				return true;
			}
			return false;
		}
	}

	addConsumer(ti.clst, ci);


	// запоминаем начальное время
	struct timeval tm;
	struct timezone tz;
	tm.tv_sec = 0; tm.tv_usec = 0;
	gettimeofday(&tm,&tz);
	ti.tv_sec	= tm.tv_sec;
	ti.tv_usec 	= tm.tv_usec;

	lst.push_front(ti);
	return true;
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::removeThreshold(ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& ci)
{
	for( ThresholdExtList::iterator it=lst.begin(); it!=lst.end(); ++it) 
	{
		if( ti == (*it) )
		{
			if( removeConsumer(it->clst, ci) )
			{
				if( it->clst.empty() )
					lst.erase(it);
				return true;
			}
		}
	}

	return false;
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::checkThreshold( AIOStateList::iterator& li, 
										const IOController_i::SensorInfo& si, 
										bool send_msg )
{
	{	// lock
		uniset_mutex_lock lock(trshMutex, 300);

		// поиск списка порогов
		UniSetTypes::KeyType skey( key(si.id,si.node) );
		AskThresholdMap::iterator lst = askTMap.find(skey);
		if( lst==askTMap.end() )
			return;

		if( lst->second.list.empty() )
			return;

		if( li == myaioEnd() )
			li = myafind(key(si.id, si.node));

		if( li==myaioEnd() )
			return; // ???

		SensorMessage sm;
		sm.id 			= si.id;
		sm.node 		= si.node;
		sm.sensor_type	= li->second.type;
		sm.priority 	= (Message::Priority)li->second.priority;
		sm.ci			= li->second.ci;
		{
			uniset_spin_lock lock(li->second.val_lock,getCheckLockValuePause());
			sm.value 		= li->second.value;
			sm.state 		= li->second.value!=0 ? true:false;
			sm.undefined	= li->second.undefined;
			sm.sm_tv_sec 	= li->second.tv_sec;
			sm.sm_tv_usec 	= li->second.tv_usec;
		}

		// текущее время
		struct timeval tm;
		struct timezone tz;
		tm.tv_sec = 0; tm.tv_usec = 0;
		gettimeofday(&tm,&tz);

		for( ThresholdExtList::iterator it=lst->second.list.begin(); it!=lst->second.list.end(); ++it) 
		{
			// Проверка нижнего предела
			// значение должно быть меньше lowLimit-чуствительность
			if( li->second.value <= (it->lowlimit-it->sensibility) )
			{
				if( it->state == IONotifyController_i::LowThreshold )
					continue;

				it->state = IONotifyController_i::LowThreshold;
				sm.threshold = false;
				sm.tid = it->id;

				// запоминаем время изменения состояния
				it->tv_sec 		= tm.tv_sec;
				it->tv_usec 	= tm.tv_usec;
				sm.sm_tv_sec 	= tm.tv_sec;
				sm.sm_tv_usec 	= tm.tv_usec;

				// порог связан с дискретным датчиком
				if( it->sid != UniSetTypes::DefaultObjectId )
				{
					try
					{
						bool state(sm.threshold);
						// проверка на инвертированную логику
						if( it->inverse )
							state^=1;
						
						localSaveState(it->itSID,SensorInfo(it->sid),state,getId());
					}
					catch( UniSetTypes::Exception& ex )
					{
						unideb[Debug::CRIT] << myname << "(checkThreshold): "
									<< ex << endl;
					}
				}

				if( send_msg )
					send(it->clst, sm);				
			}
			// Проверка верхнего предела
			// значение должно быть больше hiLimit+чуствительность
			else if( li->second.value >= (it->hilimit+it->sensibility) )
			{ 
				if( it->state == IONotifyController_i::HiThreshold )
					continue;

				it->state = IONotifyController_i::HiThreshold;
				sm.threshold = true;
				sm.tid = it->id;
				// запоминаем время изменения состояния
				it->tv_sec 		= tm.tv_sec;
				it->tv_usec 	= tm.tv_usec;
				sm.sm_tv_sec 	= tm.tv_sec;
				sm.sm_tv_usec 	= tm.tv_usec;
		
				// порог связан с дискретным датчиком
				if( it->sid != UniSetTypes::DefaultObjectId )
				{								
					try
					{
						bool state(sm.threshold);
						// проверка на инвертированную логику
						if( it->inverse ) 
							state^=1;

						localSaveState(it->itSID,SensorInfo(it->sid),state,getId());
					}
					catch( UniSetTypes::Exception& ex )
					{
						unideb[Debug::CRIT] << myname << "(checkThreshold): "
								<< ex << endl;
					}
				}
		
				if( send_msg )
					send(it->clst, sm);
			}
			else
				it->state = IONotifyController_i::NormalThreshold;
		}
	}	// unlock

}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::askOutput(const IOController_i::SensorInfo& si, 
									const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd)
{
	// провреки на несуществующий выход проводить не надо т.к. заказчик принципиально
	// не может обратится к этому контроллеру по ссылке на другой датчик
	// (ведь ссылка на датчик это ссылка на контроллер который за него отвечает)
	// контроль заказа типа выхода здесь производится

	string name = conf->oind->getNameById(ci.id, ci.node);
	if( unideb.debugging(Debug::INFO) )	
	{
		unideb[Debug::INFO] << "(askOutput): поступил " << ( cmd == UIODontNotify ? "отказ" :"заказ" ) 
			<< " от ("<< ci.id << ") " 
				<< name << " на выход "
				<< conf->oind->getNameById(si.id,si.node) << endl;
	}
	
	// если такого выхода нет, то здесь сработает исключение...
	IOTypes type = IOController::getIOType(si);
	switch(type)
	{
		case UniversalIO::DigitalOutput:
		{	//lock
			uniset_mutex_lock lock(askDOMutex, 200);
			// а раз есть заносим(исключаем) заказчика 
			ask( askDOList, si, ci, cmd );
		} // unlock			
		break;
		
		case UniversalIO::AnalogOutput:
		{	//lock
			uniset_mutex_lock lock(askAOMutex, 200);
			// а раз есть заносим(исключаем) заказчика 
			ask( askAOList, si, ci, cmd );
		} // unlock			
		break;
		
		default:
		{
			ostringstream err;
			err << myname << "(askOutput): 'выход' с именем " << conf->oind->getNameById(si.id) << " не найден";			
			if( unideb.debugging(Debug::INFO) )	
				unideb[Debug::INFO] << err.str() << endl;
			throw IOController_i::NameNotFound(err.str().c_str());
		}
		break;
	}

	// посылка первый раз состояния 
	if( cmd==UniversalIO::UIONotify )
	{
	    try
	    {
			SensorMessage  smsg;
			smsg.id = si.id;
			smsg.node = si.node;

			try
			{
				if( type == UniversalIO::AnalogOutput )
				{
					smsg.value = IOController::getValue(si);
					smsg.state = smsg.value!=0 ? true:false;
				}
				else
				{
					smsg.state = IOController::getState(si);
					smsg.value = smsg.state ? 1:0;
				}
				
				smsg.undefined	= false;
			}
			catch( IOController_i::Undefined )
			{
				smsg.undefined	= true;
			}

			smsg.consumer 		= ci.id;
			smsg.sensor_type 	= type;
			smsg.supplier 		= getId();
			
			TransportMessage tm(smsg.transport_msg());
			ui.send(ci.id, tm, ci.node);
		}
		catch(Exception& ex)
		{
		   	unideb[Debug::WARN] << myname << "(askOutput): " << name << " "<< ex << endl;
		}
	    catch( CORBA::SystemException& ex )
	    {
	    	unideb[Debug::WARN] << myname << "(askOutput): " << name 
				<< " недоступен!!(CORBA::SystemException)" 
				<< ex.NP_minorString() << endl;
	    }
		catch(...){}
	}
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::localSetState( IOController::DIOStateList::iterator& it,
										const IOController_i::SensorInfo& si, 
										CORBA::Boolean state, UniSetTypes::ObjectId sup_id )
{
	// Если датчик не найден сдесь сработает исключение NameNotFound
	bool prevState = IOController::localGetState( it, si );
	if( unideb.debugging(Debug::INFO) )	
	{
		unideb[Debug::INFO] << myname << "(IONotifyController::setState): state=" << state 
			<< " для выхода " << conf->oind->getNameById(si.id,si.node) << endl;
	}
	
	// сохраняем состояние 
	IOController::localSetState(it,si,state,sup_id);

	// Рассылаем уведомления только если значение изменилось...
	SensorMessage sm(si.id, state);
	{	// lock
		uniset_spin_lock lock(it->second.val_lock,getCheckLockValuePause());
		if( prevState == it->second.state )
			return;
		sm.id 			= si.id;		
		sm.node 		= si.node;
		sm.state 		= it->second.state;
		sm.value 		= sm.state ? 1:0;
		sm.undefined	= it->second.undefined;
		sm.priority 	= (Message::Priority)it->second.priority;
		sm.sm_tv_sec 	= it->second.tv_sec;
		sm.sm_tv_usec 	= it->second.tv_usec;
		sm.sensor_type 	= it->second.type;
		sm.supplier 	= sup_id;
	}	// unlock

	try
	{	
		uniset_mutex_lock l(sig_mutex,500);
		changeSignal.emit(&sm);
	}
	catch(...){}
	
	try
	{	
	    if( !it->second.db_ignore )
			loggingInfo(sm);
	}
	catch(...){}


	AskMap::iterator ait = askDOList.find( UniSetTypes::key(si.id,si.node) );
	if( ait!=askDOList.end() )
	{	// lock
		uniset_mutex_lock lock(askDMutex, 200);
		send(ait->second, sm);
	}	// unlock
}				
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::localSetValue( IOController::AIOStateList::iterator& li,
										const IOController_i::SensorInfo& si, 
										CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	// Если датчик не найден сдесь сработает исключение NameNotFound
	long prevValue = IOController::localGetValue( li,si );
	if( unideb.debugging(Debug::INFO) )	
	{
		unideb[Debug::INFO] << myname << "(IONotifyController::setValue): value=" << value 
							<< " для выхода " << conf->oind->getNameById(si.id,si.node) << endl;
	}

	// сохраняем новое состояние 
	IOController::localSetValue( li, si, value, sup_id );

	// Рассылаем уведомления только если значение изменилось...
	SensorMessage sm;
	{	// lock 
		uniset_spin_lock lock(li->second.val_lock,getCheckLockValuePause());
		if( prevValue == li->second.value )
			return;

		sm.id 			= si.id;		
		sm.node 		= si.node;
		sm.value 		= li->second.value;
		sm.state 		= sm.value!=0 ? true:false;
		sm.undefined	= li->second.undefined;
		sm.sm_tv_sec 	= li->second.tv_sec;
		sm.sm_tv_usec 	= li->second.tv_usec;
		sm.priority 	= (Message::Priority)li->second.priority;
		sm.sensor_type 	= li->second.type;
		sm.ci			= li->second.ci;
		sm.supplier 	= sup_id;
	}	// unlock 

	try
	{	
		uniset_mutex_lock l(sig_mutex,500);
		changeSignal.emit(&sm);
	}
	catch(...){}

	try
	{	
		if( !li->second.db_ignore )
			loggingInfo(sm);
	}
	catch(...){}

	AskMap::iterator dit = askAOList.find( UniSetTypes::key(si.id,si.node) );
	if( dit!=askAOList.end() )
	{	// lock
		uniset_mutex_lock lock(askAMutex, 200);
		send(dit->second, sm);
	}

//	// проверка порогов
//	try
//	{	
//		checkThreshold(li,si, value);
//	}
//	catch(...){}
}			

// --------------------------------------------------------------------------------------------------------------

IONotifyController::ThresholdExtList::iterator IONotifyController::findThreshold( UniSetTypes::KeyType key, UniSetTypes::ThresholdId tid  )
{
	{	// lock
		uniset_mutex_lock lock(trshMutex, 300);
		// поиск списка порогов
//		UniSetTypes::KeyType skey( key(si.id,si.node) );
		AskThresholdMap::iterator lst = askTMap.find(key);

		if( lst!=askTMap.end() )
		{
			for( ThresholdExtList::iterator it=lst->second.list.begin(); it!=lst->second.list.end(); ++it) 
			{
				if( it->id == tid )
					return it;
			}
		}
	}
	
	ThresholdExtList::iterator it;
  	return it;
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdsListSeq* IONotifyController::getThresholdsList()
{
//	unideb[Debug::INFO] << myname << "(getThresholdsList): ...\n";

	IONotifyController_i::ThresholdsListSeq* res = new IONotifyController_i::ThresholdsListSeq();

	res->length( askTMap.size() );

	if( !askTMap.empty() )
	{
		int i=0;
		for( AskThresholdMap::iterator it=askTMap.begin(); it!=askTMap.end(); ++it )
		{
			try
			{
				(*res)[i].si 	= it->second.si;
				(*res)[i].value	= IOController::localGetValue(it->second.ait,it->second.si);
				(*res)[i].type 	= it->second.type;
			}
			catch(Exception& ex)
			{
				unideb[Debug::WARN] << myname << "(getThresholdsList): для датчика " 
					<< conf->oind->getNameById(it->second.si.id, it->second.si.node)
					<< " " << ex << endl;
				continue;
			}
			
			(*res)[i].tlist.length( it->second.list.size() );

			int k=0;
			for( ThresholdExtList::const_iterator it2= it->second.list.begin(); it2!=it->second.list.end(); ++it2 )
			{
				(*res)[i].tlist[k].id 			= it2->id;
				(*res)[i].tlist[k].hilimit 		= it2->hilimit;
				(*res)[i].tlist[k].lowlimit 	= it2->lowlimit;
				(*res)[i].tlist[k].sensibility 	= it2->sensibility;
				(*res)[i].tlist[k].state 		= it2->state;
				(*res)[i].tlist[k].tv_sec 		= it2->tv_sec;
				(*res)[i].tlist[k].tv_usec 		= it2->tv_usec;
				k++;
			}
			i++;
		}
	}
	return res;
}
// -----------------------------------------------------------------------------
void IONotifyController::buildDependsList()
{
	try
	{
		if( restorer != NULL )
			restorer->buildDependsList(this);
	}
	catch(Exception& ex)
	{ 
		unideb[Debug::WARN] << myname 
				<< "(IONotifyController::buildDependsList): " << ex << endl;
	}
}
// -----------------------------------------------------------------------------
void IONotifyController::onChangeUndefined( DependsList::iterator it, bool undefined )
{
	SensorMessage sm;

	sm.id 	= it->si.id;		
	sm.node = it->si.node;
	sm.undefined = undefined;

	if( it->dit != mydioEnd() )
	{
		sm.state 		= it->dit->second.state;
		sm.value 		= sm.state ? 1:0;
		sm.sm_tv_sec 	= it->dit->second.tv_sec;
		sm.sm_tv_usec 	= it->dit->second.tv_usec;
		sm.priority 	= (Message::Priority)it->dit->second.priority;
		sm.sensor_type 	= it->dit->second.type;
		sm.supplier 	= DefaultObjectId;
	}
	else if( it->ait != myaioEnd() )
	{
		sm.value 		= it->ait->second.value;
		sm.state 		= sm.value!=0 ? true:false;
		sm.sm_tv_sec 	= it->ait->second.tv_sec;
		sm.sm_tv_usec 	= it->ait->second.tv_usec;
		sm.priority 	= (Message::Priority)it->ait->second.priority;
		sm.sensor_type 	= it->ait->second.type;
		sm.ci			= it->ait->second.ci;
		sm.supplier 	= DefaultObjectId;
	}

	try
	{	
		if( !it->ait->second.db_ignore )
			loggingInfo(sm);
	}
	catch(...){}

	AskMap::iterator it1 = askDIOList.find( key(it->si.id,it->si.node) );
	if( it1!=askDIOList.end() )
	{	// lock
		uniset_mutex_lock lock(askDMutex, 1000);	
		send(it1->second, sm);
	}	// unlock
}
// -----------------------------------------------------------------------------
IDSeq* IONotifyController::askSensorsSeq( const UniSetTypes::IDSeq& lst, 
											const UniSetTypes::ConsumerInfo& ci,
											UniversalIO::UIOCommand cmd)
{
	UniSetTypes::IDList badlist; // писок не найденных идентификаторов
	
	IOController_i::SensorInfo si;

	int size = lst.length();
	for(int i=0; i<size; i++)
	{
		si.id 	= lst[i];
		si.node = conf->getLocalNode();
		try
		{
			askSensor(si,ci,cmd);
		}
		catch(...)
		{
			badlist.add( lst[i] );
		}
	}

	return badlist.getIDSeq();
}
// -----------------------------------------------------------------------------
