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
 *  \date   $Date: 2009/02/07 13:25:59 $
 *  \version $Id: IOController.cc,v 1.30 2009/02/07 13:25:59 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
//#include <stream.h>
#include <sstream>
#include <cmath>
#include "UniversalInterface.h"
#include "IOController.h"
#include "Debug.h"
// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace UniversalIO;
using namespace std;
// ------------------------------------------------------------------------------------------
IOController::IOController():
	dioMutex("dioMutex"),
	aioMutex("aioMutex")
{
}

// ------------------------------------------------------------------------------------------

IOController::IOController(const string name, const string section):   
	ObjectsManager(name, section),
	dioMutex(name+"_dioMutex"),
	aioMutex(name+"_aioMutex"),
	isPingDBServer(true),
	checkLockValuePause(0)
{
}

IOController::IOController(ObjectId id):   
	ObjectsManager(id),
	dioMutex(string(conf->oind->getMapName(id))+"_dioMutex"),
	aioMutex(string(conf->oind->getMapName(id))+"_aioMutex"),
	isPingDBServer(true),
	checkLockValuePause(0)
{
}

// ------------------------------------------------------------------------------------------
IOController::~IOController()
{
}

// ------------------------------------------------------------------------------------------
bool IOController::activateObject()
{
	bool res = ObjectsManager::activateObject();
	sensorsRegistration();
	return res;
}
// ------------------------------------------------------------------------------------------
bool IOController::disactivateObject()
{
	sensorsUnregistration();
	return ObjectsManager::disactivateObject();
}
// ------------------------------------------------------------------------------------------
void IOController::sensorsUnregistration()
{
	// Разрегистрируем дискретные датчики
	for( DIOStateList::iterator li = dioList.begin();
		 li != dioList.end(); ++li)
	{
		try
		{
			sUnRegistration( li->second.si );
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << myname << "(sensorsUnregistration): "<< ex << endl;
		}
		catch(...){}
	}

	// Разрегистрируем аналоговые датчики
	for( AIOStateList::iterator li = aioList.begin();
		 li != aioList.end(); ++li)
	{
		try
		{
			sUnRegistration( li->second.si );
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << myname << "(sensorsUnregistration): "<< ex << endl;
		}
		catch(...){}
	}

}
// ------------------------------------------------------------------------------------------
CORBA::Boolean IOController::getState( const IOController_i::SensorInfo& si )
{
	DIOStateList::iterator li(dioList.end());
	return localGetState(li,si);
}

// ------------------------------------------------------------------------------------------
CORBA::Long IOController::getValue( const IOController_i::SensorInfo& si )
{
	AIOStateList::iterator li(aioList.end());
	try
	{
		return localGetValue(li,si);
	} // getState if not found...
	catch( IOController_i::NameNotFound )
	{
		DIOStateList::iterator li(dioList.end());
		return (localGetState(li,si) ? 1 : 0);
	}
}
// ------------------------------------------------------------------------------------------
bool IOController::localGetState( IOController::DIOStateList::iterator& li, 
									const IOController_i::SensorInfo& si )
{
		if( li == dioList.end() )
			li = dioList.find( UniSetTypes::key(si.id,si.node) );
	
		if( li != dioList.end() )
		{
			if( li->second.undefined )
				throw IOController_i::Undefined();

			uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
			return li->second.state;
		}

	// -------------
	ostringstream err;
	err << myname << "(localGetState): дискретный вход(выход) с именем " 
		<< conf->oind->getNameById(si.id) << " не найден";
	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ---------------------------------------------------------------------------
long IOController::localGetValue( IOController::AIOStateList::iterator& li, 
									const IOController_i::SensorInfo& si )
{
		if( li == aioList.end() )
			li = aioList.find( key(si.id,si.node) );

		if( li!=aioList.end() )
		{
			if( li->second.undefined )
				throw IOController_i::Undefined();

			uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
			return li->second.value;
		}
	
	// -------------
	ostringstream err;
	err << myname << "(localGetValue): аналоговый вход(выход) с именем " 
		<< conf->oind->getNameById(si.id) << " не найден";
	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
void IOController::fastSaveState( const IOController_i::SensorInfo& si, CORBA::Boolean state, 
										IOTypes type, UniSetTypes::ObjectId sup_id )
{
	try
	{
		DIOStateList::iterator li(dioList.end());
		localSaveState(li, si, state, sup_id  );
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------
void IOController::saveState( const IOController_i::SensorInfo& si, CORBA::Boolean state, 
										IOTypes type, UniSetTypes::ObjectId sup_id )
{
	IOController::DIOStateList::iterator li(dioList.end());
	localSaveState( li, si, state, sup_id  );
}
// ------------------------------------------------------------------------------------------
void IOController::setUndefinedState(const IOController_i::SensorInfo& si, 
									CORBA::Boolean undefined, UniSetTypes::ObjectId sup_id )
{
	IOController::AIOStateList::iterator li(aioList.end());
	localSetUndefinedState( li,undefined,si );
}
// -----------------------------------------------------------------------------
void IOController::localSetUndefinedState( AIOStateList::iterator& li, 
											bool undefined,	const IOController_i::SensorInfo& si )
{
	// сохранение текущего состояния
	if( li == aioList.end() )
		li = aioList.find(key(si.id, si.node));

	if( li==aioList.end() )
	{
		ostringstream err;
		err << myname << "(localSetUndefined): не зарегистрирован датчик "
			<< "имя: " << conf->oind->getNameById(si.id)
			<< " узел: " << conf->oind->getMapName(si.node); 
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	{	// lock
		uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
		li->second.undefined = undefined;
		updateDepends( li->second.dlst, undefined, li->second.dlst_lock );
	}	// unlock
}															
// ------------------------------------------------------------------------------------------
void IOController::localSaveState( IOController::DIOStateList::iterator& li,
									const IOController_i::SensorInfo& si, 
									CORBA::Boolean state, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	// сохранение текущего состояния
	if( li == dioList.end() )
		li = dioList.find(key(si.id, si.node));
	
	if( li==dioList.end() )
	{
		ostringstream err;
		err << myname << "(saveState): не зарегистрирован датчик "
			<< "имя: " << conf->oind->getNameById(si.id)
			<< " узел: " << conf->oind->getMapName(si.node); 
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	if( li->second.type != UniversalIO::DigitalInput ) // && li->second.type != UniversalIO::DigitalOutput )
	{
		ostringstream err;
		err << myname << "(saveState): неверно указан тип( " << li->second.type << ") дискретного датчика имя: " 
			<< conf->oind->getNameById(si.id)
			<< " узел: " << conf->oind->getMapName(si.node); 
		throw IOController_i::IOBadParam(err.str().c_str());
	}

//	{	// lock
//		uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
		bool changed = false;
		bool blk_set = false;
		bool blocked = ( li->second.blocked || li->second.undefined );

		if( checkDFilters(&li->second,state,sup_id) || blocked )
		{
			{	// lock
				uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
				if( !blocked )
					li->second.real_state = li->second.state;

				if( li->second.real_state != state )
					changed = true;

				if( blocked )
				{
					li->second.real_state = state;
					li->second.state = li->second.block_state;
				}
				else
				{
					li->second.state = state;
					li->second.real_state = state;
				}

				blk_set = li->second.state;

			} 	// unlock

			// запоминаем время изменения
			struct timeval tm;
			struct timezone tz;
			tm.tv_sec 	= 0;
			tm.tv_usec 	= 0;
			gettimeofday(&tm,&tz);
			li->second.tv_sec 	= tm.tv_sec;
			li->second.tv_usec 	= tm.tv_usec;
		}
//	}	// unlock

		if( unideb.debugging(Debug::INFO) )	
		{
			unideb[Debug::INFO] << myname << ": сохраняем состояние дискретного датчика "
								<< conf->oind->getNameById(si.id, si.node) 
								<< " = " << state 
								<< " blocked=" << blocked 
								<< " --> state=" << li->second.state 
								<< " real_state=" << li->second.real_state
								<< " changed=" << changed
								<< endl;
		}									

		// обновляем список зависимых				
		if( changed )
			updateBlockDepends( li->second.dlst, blk_set, li->second.dlst_lock );
}
// ------------------------------------------------------------------------------------------
void IOController::fastSaveValue( const IOController_i::SensorInfo& si, CORBA::Long value, 
										IOTypes type, UniSetTypes::ObjectId sup_id )
{
	try
	{
		IOController::AIOStateList::iterator li(aioList.end());
		localSaveValue( li, si,value, sup_id );
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------
void IOController::saveValue( const IOController_i::SensorInfo& si, CORBA::Long value, 
										IOTypes type, UniSetTypes::ObjectId sup_id )
{
	IOController::AIOStateList::iterator li(aioList.end());
	localSaveValue( li, si,value, sup_id );
}
// ------------------------------------------------------------------------------------------
void IOController::localSaveValue( IOController::AIOStateList::iterator& li,
									const IOController_i::SensorInfo& si, 
									CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	// сохранение текущего состояния
//	AIOStateList::iterator li( aioList.end() );
	if( li == aioList.end() )
		li = aioList.find(key(si.id, si.node));
	
	if( li==aioList.end() )
	{	
		ostringstream err;
		err << myname << "(saveValue): не зарегистрирован датчик "
			<< "имя: " << conf->oind->getNameById(si.id)
			<< " узел: " << conf->oind->getMapName(si.node);  
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	if( li->second.type != UniversalIO::AnalogInput ) // && li->second.type != UniversalIO::AnalogOutput )
	{
		ostringstream err;
		err << myname << "(saveValue): неверно указан тип(" << li->second.type
			<< ") аналогового датчика имя: " << conf->oind->getNameById(si.id)
			<< " узел: " << conf->oind->getMapName(si.node); 
		throw IOController_i::IOBadParam(err.str().c_str());
	}

	{	// lock
		uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);

		// фильтрам может потребоваться измениять исходное значение (например для усреднения)
		// поэтому передаём (и затем сохраняем) напрямую(ссылку) value (а не const value)
		
		bool blocked = ( li->second.blocked || li->second.undefined );
		
		if( checkAFilters(&li->second,value,sup_id) || blocked )
		{
			if( unideb.debugging(Debug::INFO) )	
			{
				unideb[Debug::INFO] << myname << ": сохраняем состояние аналогового датчика "
							<< conf->oind->getNameById(si.id, si.node) 
							<< " = "<< value << endl;
			}
			if( !blocked )
				li->second.real_value = li->second.value;
		
			if( blocked )
			{
				li->second.real_value = value;
				li->second.value = li->second.block_value;
			}
			else
			{
				li->second.value = value;
				li->second.real_value = value;
			}

			// запоминаем время изменения
			struct timeval tm;
			struct timezone tz;
			tm.tv_sec 	= 0;
			tm.tv_usec 	= 0;
			gettimeofday(&tm,&tz);
			li->second.tv_sec 	= tm.tv_sec;
			li->second.tv_usec 	= tm.tv_usec;
		}
	}	// unlock
}
// ------------------------------------------------------------------------------------------
IOTypes IOController::getIOType( const IOController_i::SensorInfo& si )
{
	UniSetTypes::KeyType k = key(si.id,si.node);
	// Проверяем в списке дискретных 
	DIOStateList::iterator li = dioList.find(k);
	if( li!=dioList.end() )
		return li->second.type;

	// Проверяем в списке аналоговых
	AIOStateList::iterator ali = aioList.find(k);
	if( ali!=aioList.end() )
		return ali->second.type;
	
	ostringstream err;
	err << myname << "(getIOType): датчик имя: " << conf->oind->getNameById(si.id) << " не найден";			
//	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ---------------------------------------------------------------------------

void IOController::setState( const IOController_i::SensorInfo& si, CORBA::Boolean state, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();
	
	IOController::DIOStateList::iterator li(dioList.end());
	localSetState(li,si,state,sup_id);
}

void IOController::fastSetState( const IOController_i::SensorInfo& si, CORBA::Boolean state, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	try
	{
		IOController::DIOStateList::iterator li(dioList.end());
		localSetState(li,si,state,sup_id);
	}
	catch(...){}
}

// ---------------------------------------------------------------------------

void IOController::setValue( const IOController_i::SensorInfo& si, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	IOController::AIOStateList::iterator li(aioList.end());
	localSetValue(li,si,value,sup_id);
}

void IOController::fastSetValue( const IOController_i::SensorInfo& si, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	try
	{
		IOController::AIOStateList::iterator li(aioList.end());
		localSetValue(li,si,value,sup_id);
	}
	catch(...){};
}
// ---------------------------------------------------------------------------
void IOController::localSetState( IOController::DIOStateList::iterator& li,
									const IOController_i::SensorInfo& si, CORBA::Boolean state,
									UniSetTypes::ObjectId sup_id )
{
	if( li == dioList.end() )
		li = dioList.find( key(si.id, si.node) );

	if( li!=dioList.end() && li->second.type == UniversalIO::DigitalOutput )
	{
		bool blocked = false;
		bool changed = false;
		bool blk_set = false;

		{	// lock
			uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
			blocked = ( li->second.blocked || li->second.undefined );

			if ( !blocked )
				li->second.real_state = li->second.state;
	
			if( li->second.real_state != state )
				changed = true;

			if( blocked )
			{
				li->second.real_state 	= state;
				li->second.state 		= false;
			}
			else
			{
				li->second.state = state;
				li->second.real_state = state;
			}
			
			blk_set = li->second.state;
		} // unlock 			
			
		struct timeval tm;
		struct timezone tz;
		tm.tv_sec 	= 0;
		tm.tv_usec 	= 0;
		gettimeofday(&tm,&tz);
		li->second.tv_sec 	= tm.tv_sec;
		li->second.tv_usec 	= tm.tv_usec;

		if( unideb.debugging(Debug::INFO) )	
		{
			unideb[Debug::INFO] << myname 
					<< ": сохраняем состояние дискретного выхода "
					<< conf->oind->getNameById(si.id, si.node) << " = " << state
					<< " blocked=" << li->second.blocked 
					<<" --> state=" << li->second.state 
					<< " changed=" << changed 
					<< " blocked=" << blocked
					<< endl;
		}

		if( changed )
			updateBlockDepends( li->second.dlst, blk_set, li->second.dlst_lock );

		return;
	}	
	
	// -------------
	ostringstream err;
	err << myname << "(localSetState): выход с именем " << conf->oind->getNameById(si.id) << " не найден";
	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}										
// ---------------------------------------------------------------------------
void IOController::localSetValue( IOController::AIOStateList::iterator& li,
									const IOController_i::SensorInfo& si, CORBA::Long value,
									UniSetTypes::ObjectId sup_id )
{
	if( li == aioList.end() )
		li = aioList.find( key(si.id, si.node) );
		
	if( li!=aioList.end() && li->second.type == UniversalIO::AnalogOutput )
	{
		{	// lock
			uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);

			if( li->second.blocked )
				li->second.real_value = value;
			else
			{
				li->second.value = value;
				li->second.real_value = value;
			}

			// запоминаем время изменения
			struct timeval tm;
			struct timezone tz;
			tm.tv_sec = 0;
			tm.tv_usec = 0;
			gettimeofday(&tm,&tz);
			li->second.tv_sec = tm.tv_sec;
			li->second.tv_usec = tm.tv_usec;
			if( unideb.debugging(Debug::INFO) )	
			{
				unideb[Debug::INFO] << myname << "(localSetValue): сохраняем состояние аналогового выхода "
								<< conf->oind->getNameById(si.id, si.node) << " = " << value 
								<< " blocked=" << li->second.blocked 
								<<" --> val=" << li->second.value << endl;
			}
			return;
		} // unlock
	}
	
	// -------------
	ostringstream err;
	err << myname << "(localSetValue): выход с именем " << conf->oind->getNameById(si.id) << " не найден";
	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}												
// ---------------------------------------------------------------------------
void IOController::dsRegistration( const UniDigitalIOInfo& dinf, bool force )
{
	// проверка задан ли контроллеру идентификатор
	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err << "(IOCOntroller::dsRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
		unideb[Debug::WARN] << err.str() << endl;
		throw ResolveNameError(err.str().c_str());
	}

	UniSetTypes::KeyType k = key(dinf.si.id, dinf.si.node);
	{	// lock
		uniset_mutex_lock lock(dioMutex, 500);
		if( !force )
		{
			DIOStateList::iterator li = dioList.find(k);
			if( li!=dioList.end() )
			{
				ostringstream err;
				err << "Попытка повторной регистрации датчика("<< k << "). имя: " 
					<< conf->oind->getNameById(dinf.si.id)
					<< " узел: " << conf->oind->getMapName(dinf.si.node); 
				throw ObjectNameAlready(err.str().c_str());
			}
		}

		DIOStateList::mapped_type di(dinf);
		// запоминаем начальное время
		struct timeval tm;
		struct timezone tz;
		tm.tv_sec 	= 0;
		tm.tv_usec 	= 0;
		gettimeofday(&tm,&tz);
		di.tv_sec 	= tm.tv_sec;
		di.tv_usec 	= tm.tv_usec;
		di.state 	= di.default_val;

		// более оптимальный способ (при условии вставки первый раз)
		dioList.insert(DIOStateList::value_type(k,di));
	}

	try
	{
		for( int i=0; i<2; i++ )
		{
			try
			{
				if( unideb.debugging(Debug::INFO) )	
				{
					unideb[Debug::INFO] << myname 
						<< "(dsRegistration): регистрирую " 
						<< conf->oind->getNameById(dinf.si.id, dinf.si.node) << endl;
				}
				ui.registered( dinf.si.id, dinf.si.node, getRef(), true );
				return;
			}
			catch(ObjectNameAlready& ex )
			{	
				if( unideb.debugging(Debug::WARN) )
				{
					unideb[Debug::WARN] << myname 
						<< "(dsRegistration): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
				}
				ui.unregister(dinf.si.id,dinf.si.node);
			}
		}
	}
	catch(Exception& ex)
	{
		unideb[Debug::CRIT] << myname << "(dsRegistration): " << ex << endl;
	}
	catch(...)
	{
		unideb[Debug::CRIT] << myname << "(dsRegistration): catch ..."<< endl;		
	}
}
// ---------------------------------------------------------------------------
void IOController::asRegistration( const UniAnalogIOInfo& ainf, bool force )
{
	// проверка задан ли контроллеру идентификатор
	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err << "(IOCOntroller::dsRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
		unideb[Debug::WARN] << err.str() << endl;
		throw ResolveNameError(err.str().c_str());
	}

	UniSetTypes::KeyType k = key(ainf.si.id, ainf.si.node);
	{	// lock
		uniset_mutex_lock lock(aioMutex, 500);
		if( !force )
		{
			AIOStateList::iterator li = aioList.find(k);
			if( li!=aioList.end() )
			{
				ostringstream err;
				err << "Попытка повторной регистрации датчика("<< k << "). имя: " 
					<< conf->oind->getNameById(ainf.si.id)
					<< " узел: " << conf->oind->getMapName(ainf.si.node); 
				throw ObjectNameAlready(err.str().c_str());
			}
		}

		AIOStateList::mapped_type ai(ainf);
		// запоминаем начальное время
		struct timeval tm;
		struct timezone tz;
		tm.tv_sec 	= 0;
		tm.tv_usec 	= 0;
		gettimeofday(&tm,&tz);
		ai.tv_sec 	= tm.tv_sec;
		ai.tv_usec 	= tm.tv_usec;
		ai.value 	= ai.default_val;

		// более оптимальный способ(при условии вставки первый раз)
		aioList.insert(AIOStateList::value_type(k,ai));
	}

	try
	{
		for( int i=0; i<2; i++ )
		{
			try
			{
				if( unideb.debugging(Debug::INFO) )	
				{
					unideb[Debug::INFO] << myname 
						<< "(asRegistration): регистрирую " 
						<< conf->oind->getNameById(ainf.si.id, ainf.si.node) << endl;
				}
				ui.registered( ainf.si.id, ainf.si.node, getRef(), true );
				return;
			}
			catch(ObjectNameAlready& ex )
			{
				if( unideb.debugging(Debug::WARN) )
				{
					unideb[Debug::WARN] << myname 
					<< "(asRegistration): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
				}
				ui.unregister(ainf.si.id,ainf.si.node);
			}
		}
	}
	catch(Exception& ex)
	{
		unideb[Debug::CRIT] << myname << "(asRegistration): " << ex << endl;
	}
	catch(...)
	{
		unideb[Debug::CRIT] << myname << "(asRegistration): catch ..."<< endl;		
	}
}
// ---------------------------------------------------------------------------
void IOController::sUnRegistration( const IOController_i::SensorInfo& si )
{
	ui.unregister( si.id, si.node );
}
// ---------------------------------------------------------------------------

void IOController::logging( UniSetTypes::SensorMessage& sm )
{
	try
	{
//		struct timezone tz;
//		gettimeofday(&sm.tm,&tz);
		ObjectId dbID = conf->getDBServer();				
		// значит на этом узле нет DBServer-а
		if( dbID == UniSetTypes::DefaultObjectId )
			return;
 		
		sm.consumer = dbID;
		TransportMessage tm(sm.transport_msg());
		ui.send(sm.consumer, tm);
		isPingDBServer = true;
	}
	catch(...)
	{
		if(isPingDBServer)
		{
			unideb[Debug::CRIT] << myname << "(logging): DBServer недоступен" << endl;
			isPingDBServer = false;
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
void IOController::dumpToDB()
{
	// значит на этом узле нет DBServer-а
	if( conf->getDBServer() == UniSetTypes::DefaultObjectId )
		return;

	// Проходим по списку дискретных 
	{	// lock
//		uniset_mutex_lock lock(dioMutex, 100);
		for( DIOStateList::iterator li = dioList.begin(); li!=dioList.end(); ++li ) 
		{
			uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
			SensorMessage sm;
			sm.id 			= li->second.si.id;
			sm.node 		= li->second.si.node;
			sm.sensor_type 	= li->second.type;	
			sm.state 		= li->second.state;
			sm.value 		= sm.state ? 1:0;
			sm.undefined	= li->second.undefined;
			sm.priority 	= (Message::Priority)li->second.priority;
			sm.sm_tv_sec 	= li->second.tv_sec;
			sm.sm_tv_usec 	= li->second.tv_usec;
			logging(sm);
		}
	}	// unlock 

	// Проходим по списку аналоговых
	{	// lock
//		uniset_mutex_lock lock(aioMutex, 100);
		for( AIOStateList::iterator li = aioList.begin(); li!=aioList.end(); ++li ) 
		{
			uniset_spin_lock lock(li->second.val_lock,checkLockValuePause);
			SensorMessage sm;
			sm.id 			= li->second.si.id;
			sm.node 		= li->second.si.node;
			sm.sensor_type 	= li->second.type;	
			sm.value 		= li->second.value;
			sm.state 		= sm.value!=0 ? true:false;
			sm.undefined	= li->second.undefined;
			sm.priority 	= (Message::Priority)li->second.priority;
			sm.sm_tv_sec 	= li->second.tv_sec;
			sm.sm_tv_usec 	= li->second.tv_usec;
			sm.ci			= li->second.ci;
			logging(sm); //	alogging( li->second.si,li->second.value,li->second.type );
		}
	}	// unlock 
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::ASensorInfoSeq* IOController::getAnalogSensorsMap()
{
	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	IOController_i::ASensorInfoSeq* res = new IOController_i::ASensorInfoSeq();	
	res->length( aioList.size());

//	{ // lock
//		uniset_mutex_lock lock(aioMutex, 500);		
		int i=0;
		for( AIOStateList::iterator it=aioList.begin(); it!=aioList.end(); ++it)
		{	
			uniset_spin_lock lock(it->second.val_lock,checkLockValuePause);
			(*res)[i] = it->second;
			i++;
		}
//	}
	
	return res;
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::DSensorInfoSeq* IOController::getDigitalSensorsMap()
{
	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	IOController_i::DSensorInfoSeq* res = new IOController_i::DSensorInfoSeq();	
	res->length(dioList.size());

//	{ // lock
//		uniset_mutex_lock lock(dioMutex, 500);		
		int i=0;
		for( DIOStateList::iterator it= dioList.begin(); it!=dioList.end(); ++it)
		{	
			uniset_spin_lock lock(it->second.val_lock,checkLockValuePause);
			(*res)[i].si		= it->second.si;
			(*res)[i].type		= it->second.type;
			(*res)[i].state		= it->second.state;
			(*res)[i].real_state= it->second.real_state;
			(*res)[i].priority 	= it->second.priority;
			(*res)[i].tv_sec 	= it->second.tv_sec;
			(*res)[i].tv_usec 	= it->second.tv_usec;
			(*res)[i].undefined = it->second.undefined;
			(*res)[i].blocked 	= it->second.blocked;
			(*res)[i].default_val = it->second.default_val;
			i++;
		}
//	}
	
	return res;
}
// --------------------------------------------------------------------------------------------------------------
UniSetTypes::Message::Priority IOController::getMessagePriority(UniSetTypes::KeyType k, UniversalIO::IOTypes type )
{
	switch(type)
	{
		case UniversalIO::DigitalInput:
		case UniversalIO::DigitalOutput:
		{
			DIOStateList::iterator it = dioList.find(k);
			if( it!=dioList.end() )
				return (UniSetTypes::Message::Priority)it->second.priority;
		}
		break;
		
		case UniversalIO::AnalogInput:
		case UniversalIO::AnalogOutput:
		{
			AIOStateList::iterator it = aioList.find(k);
			if( it!=aioList.end() )
				return (UniSetTypes::Message::Priority)it->second.priority;
		}
		break;
	}
	
	return UniSetTypes::Message::Medium; // ??
}
// --------------------------------------------------------------------------------------------------------------
UniSetTypes::Message::Priority IOController::getPriority(const IOController_i::SensorInfo& si, 
															UniversalIO::IOTypes type)
{
	return getMessagePriority(key(si.id,si.node), type);
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::DigitalIOInfo IOController::getDInfo(const IOController_i::SensorInfo& si)
{
	DIOStateList::iterator it = dioList.find( key(si.id, si.node) );
	if( it!=dioList.end() )
		return it->second;

	// -------------
	ostringstream err;
	err << myname << "(getDInfo): дискретный вход(выход) с именем " << conf->oind->getNameById(si.id) << " не найден";
	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::AnalogIOInfo IOController::getAInfo(const IOController_i::SensorInfo& si)
{
	AIOStateList::iterator it = aioList.find( key(si.id, si.node) );
	if( it!=aioList.end() )
		return it->second;

	// -------------
	ostringstream err;
	err << myname << "(getAInfo): аналоговый вход(выход) с именем " << conf->oind->getNameById(si.id) << " не найден";
	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
CORBA::Long IOController::getRawValue(const IOController_i::SensorInfo& si)
{
	AIOStateList::iterator it = aioList.find( key(si.id, si.node) );
	if( it==aioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): аналоговый вход(выход) с именем " 
			<< conf->oind->getNameById(si.id,si.node) << " не найден";
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	// ??? получаем raw из калиброванного значения ???
	IOController_i::CalibrateInfo& ci(it->second.ci);
	return UniSetTypes::lcalibrate(it->second.value,ci.minRaw,ci.maxRaw,ci.minCal,ci.maxCal,true);
}
// --------------------------------------------------------------------------------------------------------------
void IOController::calibrate(const IOController_i::SensorInfo& si, 
								const IOController_i::CalibrateInfo& ci,
								UniSetTypes::ObjectId adminId )
{
	AIOStateList::iterator it = aioList.find( key(si.id, si.node) );
	if( it==aioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): аналоговый вход(выход) с именем " << conf->oind->getNameById(si.id,si.node) << " не найден";
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	if( unideb.debugging(Debug::INFO) )	
		unideb[Debug::INFO] << myname <<"(calibrate): from " << conf->oind->getNameById(adminId) << endl;
	
	it->second.ci = ci;
}									
// --------------------------------------------------------------------------------------------------------------
IOController_i::CalibrateInfo IOController::getCalibrateInfo(const IOController_i::SensorInfo& si)
{
	AIOStateList::iterator it = aioList.find( key(si.id, si.node) );
	if( it==aioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): аналоговый вход(выход) с именем " 
			<< conf->oind->getNameById(si.id,si.node) << " не найден";
		throw IOController_i::NameNotFound(err.str().c_str());
	}
	return it->second.ci;	
}
// --------------------------------------------------------------------------------------------------------------

IOController::UniDigitalIOInfo::UniDigitalIOInfo& 
			IOController::UniDigitalIOInfo::operator=(IOController_i::DigitalIOInfo& r)
{
	(*this) = r;
//	any=0;
	return *this;
}

IOController::UniDigitalIOInfo::UniDigitalIOInfo& 
				IOController::UniDigitalIOInfo::operator=(IOController_i::DigitalIOInfo* r)
{
	(*this) = (*r);
//	any=0;
	return *this;
}

const IOController::UniDigitalIOInfo::UniDigitalIOInfo& 
				IOController::UniDigitalIOInfo::operator=(const IOController_i::DigitalIOInfo& r)
{
	(*this) = r;
//	any=0;
	return *this;
}

IOController::UniDigitalIOInfo::UniDigitalIOInfo(IOController_i::DigitalIOInfo& di):
	IOController_i::DigitalIOInfo(di),
	any(0),
	dlst_lock(false)
{}
IOController::UniDigitalIOInfo::UniDigitalIOInfo(IOController_i::DigitalIOInfo* di):
	IOController_i::DigitalIOInfo(*di),
	any(0),
	dlst_lock(false)
{}

IOController::UniDigitalIOInfo::UniDigitalIOInfo(const IOController_i::DigitalIOInfo& di):
	IOController_i::DigitalIOInfo(di),
	any(0),
	dlst_lock(false)
{}

IOController::UniAnalogIOInfo::UniAnalogIOInfo(IOController_i::AnalogIOInfo& ai):
	IOController_i::AnalogIOInfo(ai),
	any(0),
	dlst_lock(false)
{}

IOController::UniAnalogIOInfo::UniAnalogIOInfo(const IOController_i::AnalogIOInfo& ai):
	IOController_i::AnalogIOInfo(ai),
	any(0),
	dlst_lock(false)
{}

IOController::UniAnalogIOInfo::UniAnalogIOInfo(IOController_i::AnalogIOInfo* ai):
	IOController_i::AnalogIOInfo(*ai),
	any(0),
	dlst_lock(false)
{}

IOController::UniAnalogIOInfo::UniAnalogIOInfo& 
			IOController::UniAnalogIOInfo::operator=(IOController_i::AnalogIOInfo& r)
{
	(*this) = r;
//	any=0;
	return *this;
}

IOController::UniAnalogIOInfo::UniAnalogIOInfo& 
				IOController::UniAnalogIOInfo::operator=(IOController_i::AnalogIOInfo* r)
{
	(*this) = (*r);
//	any=0;
	
	return *this;
}

const IOController::UniAnalogIOInfo::UniAnalogIOInfo& 
				IOController::UniAnalogIOInfo::operator=(const IOController_i::AnalogIOInfo& r)
{
	(*this) = r;
//	any=0;
	return *this;
}

// ----------------------------------------------------------------------------------------
bool IOController::checkDFilters( const UniDigitalIOInfo& di, CORBA::Boolean newstate, 
									UniSetTypes::ObjectId sup_id )
{
	for( DFilterSlotList::iterator it=dfilters.begin(); it!=dfilters.end(); ++it )
	{
		if( it->operator()(di,newstate,sup_id) == false )
			return false;
	}
	return true;
}
// ----------------------------------------------------------------------------------------
bool IOController::checkAFilters( const UniAnalogIOInfo& ai, CORBA::Long& newvalue, 
									UniSetTypes::ObjectId sup_id )
{
	for( AFilterSlotList::iterator it=afilters.begin(); it!=afilters.end(); ++it )
	{
		if( (*it)(ai,newvalue,sup_id) == false )
			return false;
	}
	return true;
}

// ----------------------------------------------------------------------------------------
IOController::AFilterSlotList::iterator IOController::addAFilter( AFilterSlot sl, bool push_front )
{
	if( push_front == false )
	{
		afilters.push_back(sl);
		AFilterSlotList::iterator it(afilters.end());
		return --it;
	}

	afilters.push_front(sl);
	return afilters.begin();
}
// ----------------------------------------------------------------------------------------
IOController::DFilterSlotList::iterator IOController::addDFilter( DFilterSlot sl, bool push_front )
{
	if( push_front == false )
	{
		dfilters.push_back(sl);
		DFilterSlotList::iterator it(dfilters.end());
		return --it;
	}

	dfilters.push_front(sl);
	return dfilters.begin();
}
// ----------------------------------------------------------------------------------------
void IOController::eraseAFilter(IOController::AFilterSlotList::iterator& it)
{
	afilters.erase(it);
}
// ----------------------------------------------------------------------------------------
void IOController::eraseDFilter(IOController::DFilterSlotList::iterator& it)
{
	dfilters.erase(it);
}
// ----------------------------------------------------------------------------------------
IOController::DIOStateList::iterator IOController::mydioBegin()
{
//	{	// lock
//		uniset_mutex_lock lock(dioMutex, 200);
		return dioList.begin(); 
//	}
}

IOController::DIOStateList::iterator IOController::mydioEnd()
{
//	{	// lock
//		uniset_mutex_lock lock(dioMutex, 200);
		return dioList.end(); 
//	}
}

IOController::AIOStateList::iterator IOController::myaioBegin()
{ 
//	{	// lock
//		uniset_mutex_lock lock(aioMutex, 200);
		return aioList.begin();
//	}
}

IOController::AIOStateList::iterator IOController::myaioEnd()
{ 
//	{	// lock
//		uniset_mutex_lock lock(aioMutex, 200);
		return aioList.end();
//	}
}

IOController::AIOStateList::iterator IOController::myafind(UniSetTypes::KeyType k)
{ 
//	{	// lock
//		uniset_mutex_lock lock(aioMutex, 200);
		return aioList.find(k);
//	}
}

IOController::DIOStateList::iterator IOController::mydfind(UniSetTypes::KeyType k)
{ 
//	{	// lock
//		uniset_mutex_lock lock(dioMutex, 200);
		return dioList.find(k);
//	}
}
// -----------------------------------------------------------------------------
IOController::DependsInfo::DependsInfo( bool init ):
	block_invert(false),
	init(init)
{
}
// -----------------------------------------------------------------------------
IOController::DependsInfo::DependsInfo( IOController_i::SensorInfo& si,
				DIOStateList::iterator& dit, AIOStateList::iterator& ait ):
	si(si),
	dit(dit),
	ait(ait),
	block_invert(false),
	init(true)
{
}
// -----------------------------------------------------------------------------
void IOController::updateDepends( IOController::DependsList& lst, bool undefined, bool& lst_lock )
{
	// защита от "зацикливания" рекурсивного вызова функции
	if( lst_lock || lst.empty() )	
		return;

	lst_lock = true;
	for( DependsList::iterator it=lst.begin(); it!=lst.end(); ++it )
	{
		if( it->dit != mydioEnd() )
		{
			if( it->dit->second.undefined != undefined )
			{
				it->dit->second.undefined = undefined;
				dslot(it,undefined);
				updateDepends( it->dit->second.dlst,undefined,it->dit->second.dlst_lock );
			}
		}
		else if( it->ait != myaioEnd() )
		{
			if( it->ait->second.undefined != undefined )
			{
				it->ait->second.undefined = undefined;
				dslot(it,undefined);
				updateDepends( it->ait->second.dlst,undefined,it->ait->second.dlst_lock );
			}
		}
	}

	lst_lock = false;
}
// -----------------------------------------------------------------------------
void IOController::updateBlockDepends( IOController::DependsList& lst, bool blk_state, bool& lst_lock )
{
	// защита от "зацикливания" рекурсивного вызова функции
	if( lst_lock || lst.empty() )	
		return;

	lst_lock = true;
	for( DependsList::iterator it=lst.begin(); it!=lst.end(); ++it )
	{
		bool set_blk = it->block_invert ? blk_state : !blk_state;

		if( it->dit != mydioEnd() )
		{
			if( it->dit->second.blocked != set_blk )
			{
				bool set = set_blk ? it->dit->second.state : it->dit->second.real_state;
				it->dit->second.blocked = set_blk;
				localSaveState( it->dit, it->si,set, getId() );
				bslot(it,set_blk);
			}
		}
		else if( it->ait != myaioEnd() )
		{
			if( it->ait->second.blocked != set_blk )
			{
				long val = set_blk ? it->ait->second.value : it->ait->second.real_value;
				it->ait->second.blocked = set_blk;
				localSaveValue( it->ait, it->si, val, getId() );
				bslot(it,set_blk);
			}
		}
	}

	lst_lock = false;
}
// -----------------------------------------------------------------------------
void IOController::setDependsSlot( DependsSlot sl )
{
	dslot = sl;
}
// -----------------------------------------------------------------------------
void IOController::setBlockDependsSlot( DependsSlot sl )
{
	bslot = sl;
}
// -----------------------------------------------------------------------------
void IOController::setCheckLockValuePause( int msec )
{
	checkLockValuePause = msec;
}
// -----------------------------------------------------------------------------
IOController_i::ASensorInfoSeq* IOController::getSensorSeq( const IDSeq& lst )
{
	int size = lst.length();

	IOController_i::ASensorInfoSeq* res = new IOController_i::ASensorInfoSeq();	
	res->length(size);

	for(int i=0; i<size; i++)
	{
		{
			DIOStateList::iterator it = dioList.find( UniSetTypes::key(lst[i],conf->getLocalNode()) );
			if( it!=dioList.end() )
			{
				uniset_spin_lock lock(it->second.val_lock,checkLockValuePause);
				(*res)[i].si		= it->second.si;
				(*res)[i].type		= it->second.type;
				(*res)[i].real_value= it->second.real_state ? 1 : 0;
				(*res)[i].value		= it->second.state ? 1 : 0;
				(*res)[i].undefined = it->second.undefined;
				(*res)[i].blocked 	= it->second.blocked;
				(*res)[i].priority 	= it->second.priority;
//				(*res)[i].ci 		= it->second.ci;
				(*res)[i].tv_sec 	= it->second.tv_sec;
				(*res)[i].tv_usec 	= it->second.tv_usec;
				(*res)[i].default_val = it->second.default_val;
				continue;
			}
		}

		{
			AIOStateList::iterator it = aioList.find( UniSetTypes::key(lst[i],conf->getLocalNode()) );
			if( it!=aioList.end() )
			{
				uniset_spin_lock lock(it->second.val_lock,checkLockValuePause);
				(*res)[i] = it->second;
				continue;
			}
		}

		// элемент не найден...
		(*res)[i].si.id 	= DefaultObjectId;
		(*res)[i].si.node 	= DefaultObjectId;
		(*res)[i].undefined = true;
	}

	return res;
}
// -----------------------------------------------------------------------------
IDSeq* IOController::setOutputSeq(const IOController_i::OutSeq& lst, ObjectId sup_id )
{
	UniSetTypes::IDList badlist; // писок не найденных идентификаторов

	int size = lst.length();

	for(int i=0; i<size; i++)
	{
		{
			DIOStateList::iterator it = dioList.find( UniSetTypes::key(lst[i].si.id,lst[i].si.node) );
			if( it!=dioList.end() )
			{
				if( it->second.type == UniversalIO::DigitalInput )
					localSaveState(it,lst[i].si,(bool)lst[i].value, sup_id);
				else // if( it.second.iotype == UniversalIO::DigitalOutput )
					localSetState(it,lst[i].si,(bool)lst[i].value, sup_id);
				continue;
			}
		}

		{
			AIOStateList::iterator it = aioList.find( UniSetTypes::key(lst[i].si.id,lst[i].si.node) );
			if( it!=aioList.end() )
			{
				if( it->second.type == UniversalIO::AnalogInput )
					localSaveValue(it,lst[i].si,lst[i].value, sup_id);
				else // if( it.second.iotype == UniversalIO::AnalogOutput )
					localSetValue(it,lst[i].si,lst[i].value, sup_id);
				
				continue;
			}
		}

		// не найден
		badlist.add( lst[i].si.id );
	}

	return badlist.getIDSeq();
}
// -----------------------------------------------------------------------------
