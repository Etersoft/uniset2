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
	ioMutex("ioMutex"),
	isPingDBServer(true)
{
}

// ------------------------------------------------------------------------------------------

IOController::IOController(const string name, const string section):
	ObjectsManager(name, section),
	ioMutex(name+"_ioMutex"),
	isPingDBServer(true)
{
}

IOController::IOController(ObjectId id):
	ObjectsManager(id),
	ioMutex(string(conf->oind->getMapName(id))+"_ioMutex"),
	isPingDBServer(true)
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
	// Разрегистрируем аналоговые датчики
	for( IOStateList::iterator li = ioList.begin();
		 li != ioList.end(); ++li)
	{
		try
		{
			ioUnRegistration( li->second.si );
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << myname << "(sensorsUnregistration): "<< ex << endl;
		}
		catch(...){}
	}

}
// ------------------------------------------------------------------------------------------
CORBA::Long IOController::getValue( const IOController_i::SensorInfo& si )
{
	IOStateList::iterator li(ioList.end());
	return localGetValue(li,si);
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( IOController::IOStateList::iterator& li,
									const IOController_i::SensorInfo& si )
{
		if( li == ioList.end() )
			li = ioList.find( key(si.id,si.node) );

		if( li!=ioList.end() )
		{
			if( li->second.undefined )
				throw IOController_i::Undefined();

			uniset_rwmutex_rlock lock(li->second.val_lock);
			return li->second.value;
		}
	
	// -------------
	ostringstream err;
	err << myname << "(localGetValue): Not found analog sensor (" << si.id << ":" << si.node << ") "
		<< conf->oind->getNameById(si.id);

	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << err.str() << endl;

	throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
void IOController::setUndefinedState(const IOController_i::SensorInfo& si, 
									CORBA::Boolean undefined, UniSetTypes::ObjectId sup_id )
{
	IOController::IOStateList::iterator li(ioList.end());
	localSetUndefinedState( li,undefined,si );
}
// -----------------------------------------------------------------------------
void IOController::localSetUndefinedState( IOStateList::iterator& li,
											bool undefined,	const IOController_i::SensorInfo& si )
{
	// сохранение текущего состояния
	if( li == ioList.end() )
		li = ioList.find(key(si.id, si.node));

	if( li==ioList.end() )
	{
		ostringstream err;
		err << myname << "(localSetUndefined): Unknown sensor (" << si.id << ":" << si.node << ")"
			<< "name: " << conf->oind->getNameById(si.id)
			<< "node: " << conf->oind->getMapName(si.node);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	bool changed = false;
	{	// lock
		uniset_rwmutex_wrlock lock(li->second.val_lock);
		changed = (li->second.undefined != undefined);
		li->second.undefined = undefined;
	}	// unlock

	try
	{
		if( changed )
			li->second.changeSignal.emit(li->second.si, li->second.value, this);
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------
void IOController::fastSetValue( const IOController_i::SensorInfo& si, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	try
	{
		IOController::IOStateList::iterator li(ioList.end());
		localSetValue( li, si, value, sup_id );
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------
void IOController::setValue( const IOController_i::SensorInfo& si, CORBA::Long value,
								UniSetTypes::ObjectId sup_id )
{
	IOController::IOStateList::iterator li(ioList.end());
	localSetValue( li, si, value, sup_id );
}
// ------------------------------------------------------------------------------------------
void IOController::localSetValue( IOController::IOStateList::iterator& li,
									const IOController_i::SensorInfo& si, 
									CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	// сохранение текущего состояния
//	IOStateList::iterator li( ioList.end() );
	if( li == ioList.end() )
		li = ioList.find(key(si.id, si.node));
	
	if( li==ioList.end() )
	{	
		ostringstream err;
		err << myname << "(localSaveValue): Unknown sensor (" << si.id << ":" << si.node << ")"
			<< "name: " << conf->oind->getNameById(si.id)
			<< "node: " << conf->oind->getMapName(si.node);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	bool changed = false;

	{	// lock
		uniset_rwmutex_wrlock lock(li->second.val_lock);

		// фильтрам может потребоваться измениять исходное значение (например для усреднения)
		// поэтому передаём (и затем сохраняем) напрямую(ссылку) value (а не const value)
		
		bool blocked = ( li->second.blocked || li->second.undefined );
		
		if( checkIOFilters(&li->second,value,sup_id) || blocked )
		{
			if( unideb.debugging(Debug::INFO) )	
			{
				unideb[Debug::INFO] << myname << ": save sensor value (" << si.id << ":" << si.node << ")"
					<< " name: " << conf->oind->getNameById(si.id)
					<< " node: " << conf->oind->getMapName(si.node)
					<< " value="<< value << endl;
			}

			long prev = li->second.value;

			if( !blocked )
				li->second.real_value = li->second.value;
		
			if( blocked )
			{
				li->second.real_value = value;
				li->second.value = li->second.d_off_value;
			}
			else
			{
				li->second.value = value;
				li->second.real_value = value;
			}

			changed = ( prev != li->second.value );

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

	try
	{
		if( changed )
			li->second.changeSignal.emit(li->second.si, li->second.value, this);
	}
	catch(...){}
}
// ------------------------------------------------------------------------------------------
IOType IOController::getIOType( const IOController_i::SensorInfo& si )
{
	UniSetTypes::KeyType k = key(si.id,si.node);

	IOStateList::iterator ali = ioList.find(k);
	if( ali!=ioList.end() )
		return ali->second.type;
	
	ostringstream err;
	err << myname << "(getIOType): датчик имя: " << conf->oind->getNameById(si.id) << " не найден";			
//	unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ---------------------------------------------------------------------------
void IOController::ioRegistration( const USensorIOInfo& ainf, bool force )
{
	// проверка задан ли контроллеру идентификатор
	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err << "(IOCOntroller::ioRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
        if( unideb.debugging(Debug::WARN) )
            unideb[Debug::WARN] << err.str() << endl;
		throw ResolveNameError(err.str().c_str());
	}

	UniSetTypes::KeyType k = key(ainf.si.id, ainf.si.node);
	{	// lock
		uniset_rwmutex_wrlock lock(ioMutex);
		if( !force )
		{
			IOStateList::iterator li = ioList.find(k);
			if( li!=ioList.end() )
			{
				ostringstream err;
				err << "Попытка повторной регистрации датчика("<< k << "). имя: " 
					<< conf->oind->getNameById(ainf.si.id)
					<< " узел: " << conf->oind->getMapName(ainf.si.node); 
				throw ObjectNameAlready(err.str().c_str());
			}
		}

		IOStateList::mapped_type ai(ainf);
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
		ioList.insert(IOStateList::value_type(k,ai));
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
						<< "(ioRegistration): регистрирую "
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
	catch( Exception& ex )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(ioRegistration): " << ex << endl;
	}
	catch(...)
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(ioRegistration): catch ..."<< endl;
	}
}
// ---------------------------------------------------------------------------
void IOController::ioUnRegistration( const IOController_i::SensorInfo& si )
{
	ui.unregister( si.id, si.node );
}
// ---------------------------------------------------------------------------
void IOController::logging( UniSetTypes::SensorMessage& sm )
{
	uniset_rwmutex_wrlock l(loggingMutex);
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
		if( isPingDBServer )
		{
			isPingDBServer = false;
			if( unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname << "(logging): DBServer unavailable" << endl;
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
void IOController::dumpToDB()
{
	// значит на этом узле нет DBServer-а
	if( conf->getDBServer() == UniSetTypes::DefaultObjectId )
		return;

	{	// lock
//		uniset_mutex_lock lock(ioMutex, 100);
		for( IOStateList::iterator li = ioList.begin(); li!=ioList.end(); ++li )
		{
			uniset_rwmutex_rlock lock(li->second.val_lock);
			SensorMessage sm;
			sm.id 			= li->second.si.id;
			sm.node 		= li->second.si.node;
			sm.sensor_type 	= li->second.type;	
			sm.value 		= li->second.value;
			sm.undefined	= li->second.undefined;
			sm.priority 	= (Message::Priority)li->second.priority;
			sm.sm_tv_sec 	= li->second.tv_sec;
			sm.sm_tv_usec 	= li->second.tv_usec;
			sm.ci			= li->second.ci;
			if ( !li->second.db_ignore )
				logging(sm); //	alogging( li->second.si,li->second.value,li->second.type );
		}
	}	// unlock 
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorInfoSeq* IOController::getSensorsMap()
{
	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	IOController_i::SensorInfoSeq* res = new IOController_i::SensorInfoSeq();
	res->length( ioList.size());

//	{ // lock
//		uniset_mutex_lock lock(ioMutex, 500);
		int i=0;
		for( IOStateList::iterator it=ioList.begin(); it!=ioList.end(); ++it)
		{	
			uniset_rwmutex_rlock lock(it->second.val_lock);
			(*res)[i] = it->second;
			i++;
		}
//	}
	
	return res;
}
// --------------------------------------------------------------------------------------------------------------
UniSetTypes::Message::Priority IOController::getMessagePriority(UniSetTypes::KeyType k, UniversalIO::IOType type )
{
	IOStateList::iterator it = ioList.find(k);
	if( it!=ioList.end() )
		return (UniSetTypes::Message::Priority)it->second.priority;

	return UniSetTypes::Message::Medium; // ??
}
// --------------------------------------------------------------------------------------------------------------
UniSetTypes::Message::Priority IOController::getPriority(const IOController_i::SensorInfo& si, 
															UniversalIO::IOType type)
{
	return getMessagePriority(key(si.id,si.node), type);
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorIOInfo IOController::getSensorIOInfo( const IOController_i::SensorInfo& si )
{
	IOStateList::iterator it = ioList.find( key(si.id, si.node) );
	if( it!=ioList.end() )
	{
		uniset_rwmutex_rlock lock(it->second.val_lock);
		return it->second;
	}

	// -------------
	ostringstream err;
	err << myname << "(getSensorIOInfo): Unknown sensor (" << si.id << ":" << si.node << ")"
		<< conf->oind->getNameById(si.id,si.node);		

	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << err.str() << endl;

	throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
CORBA::Long IOController::getRawValue(const IOController_i::SensorInfo& si)
{
	IOStateList::iterator it = ioList.find( key(si.id, si.node) );
	if( it==ioList.end() )
	{
		ostringstream err;
		err << myname << "(getRawValue): Unknown analog sensor (" << si.id << ":" << si.node << ")"
			<< conf->oind->getNameById(si.id,si.node);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	// ??? получаем raw из калиброванного значения ???
	IOController_i::CalibrateInfo& ci(it->second.ci);

	if( ci.maxCal!=0 && ci.maxCal!=ci.minCal )
	{
		if( it->second.type == UniversalIO::AI )
			return UniSetTypes::lcalibrate(it->second.value,ci.minRaw,ci.maxRaw,ci.minCal,ci.maxCal,true);

		// п╨п╟п╩п╦п╠я─я┐п╣п╪ п╡ п╬п╠я─п╟я┌п╫я┐я▌ я│я┌п╬я─п╬п╫я┐ (п╫п╟ п╡я▀я┘п╬п╢)
		if( it->second.type == UniversalIO::AO )
			return UniSetTypes::lcalibrate(it->second.value,ci.minCal,ci.maxCal,ci.minRaw,ci.maxRaw,true);
	}
	
	return it->second.value;
}
// --------------------------------------------------------------------------------------------------------------
void IOController::calibrate(const IOController_i::SensorInfo& si, 
								const IOController_i::CalibrateInfo& ci,
								UniSetTypes::ObjectId adminId )
{
	IOStateList::iterator it = ioList.find( key(si.id, si.node) );
	if( it==ioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): Unknown analog sensor (" << si.id << ":" << si.node << ")"
			<< conf->oind->getNameById(si.id,si.node);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	if( unideb.debugging(Debug::INFO) )	
		unideb[Debug::INFO] << myname <<"(calibrate): from " << conf->oind->getNameById(adminId) << endl;
	
	it->second.ci = ci;
}									
// --------------------------------------------------------------------------------------------------------------
IOController_i::CalibrateInfo IOController::getCalibrateInfo(const IOController_i::SensorInfo& si)
{
	IOStateList::iterator it = ioList.find( key(si.id, si.node) );
	if( it==ioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): Unknown analog sensor (" << si.id << ":" << si.node << ")"
			<< conf->oind->getNameById(si.id,si.node);
		throw IOController_i::NameNotFound(err.str().c_str());
	}
	return it->second.ci;	
}
// --------------------------------------------------------------------------------------------------------------
IOController::USensorIOInfo::USensorIOInfo(IOController_i::SensorIOInfo& ai):
	IOController_i::SensorIOInfo(ai),
	any(0)
{}

IOController::USensorIOInfo::USensorIOInfo(const IOController_i::SensorIOInfo& ai):
	IOController_i::SensorIOInfo(ai),
	any(0)
{}

IOController::USensorIOInfo::USensorIOInfo(IOController_i::SensorIOInfo* ai):
	IOController_i::SensorIOInfo(*ai),
	any(0)
{}

IOController::USensorIOInfo&
			IOController::USensorIOInfo::operator=(IOController_i::SensorIOInfo& r)
{
	(*this) = r;
//	any=0;
	return *this;
}

IOController::USensorIOInfo&
				IOController::USensorIOInfo::operator=(IOController_i::SensorIOInfo* r)
{
	(*this) = (*r);
//	any=0;
	
	return *this;
}

const IOController::USensorIOInfo&
				IOController::USensorIOInfo::operator=(const IOController_i::SensorIOInfo& r)
{
	(*this) = r;
//	any=0;
	return *this;
}

// ----------------------------------------------------------------------------------------
bool IOController::checkIOFilters( const USensorIOInfo& ai, CORBA::Long& newvalue,
									UniSetTypes::ObjectId sup_id )
{
	for( IOFilterSlotList::iterator it=iofilters.begin(); it!=iofilters.end(); ++it )
	{
		if( (*it)(ai,newvalue,sup_id) == false )
			return false;
	}
	return true;
}

// ----------------------------------------------------------------------------------------
IOController::IOFilterSlotList::iterator IOController::addIOFilter( IOFilterSlot sl, bool push_front )
{
	if( push_front == false )
	{
		iofilters.push_back(sl);
		IOFilterSlotList::iterator it(iofilters.end());
		return --it;
	}

	iofilters.push_front(sl);
	return iofilters.begin();
}
// ----------------------------------------------------------------------------------------
void IOController::eraseIOFilter(IOController::IOFilterSlotList::iterator& it)
{
	iofilters.erase(it);
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myioBegin()
{ 
	return ioList.begin();
}

IOController::IOStateList::iterator IOController::myioEnd()
{ 
	return ioList.end();
}

IOController::IOStateList::iterator IOController::myiofind(UniSetTypes::KeyType k)
{ 
		return ioList.find(k);
}
// -----------------------------------------------------------------------------
IOController_i::SensorInfoSeq* IOController::getSensorSeq( const IDSeq& lst )
{
	int size = lst.length();

	IOController_i::SensorInfoSeq* res = new IOController_i::SensorInfoSeq();
	res->length(size);

    for( int i=0; i<size; i++ )
    {
		IOStateList::iterator it = ioList.find( UniSetTypes::key(lst[i],conf->getLocalNode()) );
		if( it!=ioList.end() )
		{
			uniset_rwmutex_rlock lock(it->second.val_lock);
			(*res)[i] = it->second;
			continue;
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
			IOStateList::iterator it = ioList.find( UniSetTypes::key(lst[i].si.id,lst[i].si.node) );
			if( it!=ioList.end() )
			{
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
IOController_i::ShortIOInfo IOController::getChangedTime( const IOController_i::SensorInfo& si )
{
	IOStateList::iterator ait = ioList.find( key(si.id, si.node) );
	if( ait!=ioList.end() )
	{
		IOController_i::ShortIOInfo i;
		uniset_rwmutex_rlock lock(ait->second.val_lock);
		i.value = ait->second.value;
		i.tv_sec = ait->second.tv_sec;
		i.tv_usec = ait->second.tv_usec;
		return i;
	}

	// -------------
	ostringstream err;
	err << myname << "(getChangedTime): вход(выход) с именем "
		<< conf->oind->getNameById(si.id) << " не найден";
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// -----------------------------------------------------------------------------
IOController_i::ShortMapSeq* IOController::getSensors()
{
	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	IOController_i::ShortMapSeq* res = new IOController_i::ShortMapSeq();
	res->length( ioList.size() );

	int i=0;
	for( IOStateList::iterator it=ioList.begin(); it!=ioList.end(); ++it)
	{	
		IOController_i::ShortMap m;
		{
			uniset_rwmutex_rlock lock(it->second.val_lock);
			m.id 	= it->second.si.id;
			m.value = it->second.value;
			m.type = it->second.type;
		}
		(*res)[i++] = m;
	}

	return res;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value( const IOController_i::SensorInfo& si )
{
	return signal_change_value( si.id, si.node );
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )
{
	IOStateList::iterator it = ioList.find( key(id,node) );
	if( it==ioList.end() )
	{
		ostringstream err;
		err << myname << "(signal_change_value): вход(выход) с именем "
			<< conf->oind->getNameById(id) << " не найден";

		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << err.str() << endl;

		throw IOController_i::NameNotFound(err.str().c_str());
	}

	uniset_rwmutex_rlock lock(it->second.val_lock);
	return it->second.changeSignal;
}
// -----------------------------------------------------------------------------
void IOController::USensorIOInfo::checkDepend( const IOController_i::SensorInfo& dep_si , long newvalue, IOController* ic )
{
	bool changed = false;
	{
		uniset_rwmutex_wrlock lock(val_lock);
		bool prev = blocked;
		blocked = ( newvalue == d_value ) ? false : true;
		changed = ( prev != blocked );
	}

	if( changed )
		ic->localSetValue( it, si, real_value, ic->getId() );
}
// -----------------------------------------------------------------------------
