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
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
//#include <stream.h>
#include <sstream>
#include <cmath>
#include "UInterface.h"
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

IOController::IOController(const string& name, const string& section):
	UniSetManager(name, section),
	ioMutex(name + "_ioMutex"),
	isPingDBServer(true)
{
}

IOController::IOController(ObjectId id):
	UniSetManager(id),
	ioMutex(string(uniset_conf()->oind->getMapName(id)) + "_ioMutex"),
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
	bool res = UniSetManager::activateObject();
	sensorsRegistration();

	// Начальная инициализация
	activateInit();

	return res;
}
// ------------------------------------------------------------------------------------------
bool IOController::deactivateObject()
{
	sensorsUnregistration();
	return UniSetManager::deactivateObject();
}
// ------------------------------------------------------------------------------------------
void IOController::sensorsUnregistration()
{
	// Разрегистрируем аналоговые датчики
	for( const auto& li : ioList )
	{
		try
		{
			ioUnRegistration( li.second->si.id );
		}
		catch( const Exception& ex )
		{
			ucrit << myname << "(sensorsUnregistration): " << ex << endl;
		}
	}
}
// ------------------------------------------------------------------------------------------
void IOController::activateInit()
{
	// Разрегистрируем аналоговые датчики
	for( auto li = ioList.begin(); li != ioList.end(); ++li )
	{
		try
		{
			auto s = li->second;

			// Проверка зависимостей
			if( s->d_si.id != DefaultObjectId )
			{
				auto d_it = myiofind(s->d_si.id);

				if( d_it != ioEnd() )
					s->checkDepend( d_it->second, this);
			}

			sigInit.emit(li, this);
		}
		catch( const Exception& ex )
		{
			ucrit << myname << "(activateInit): " << ex << endl;
		}
	}
}
// ------------------------------------------------------------------------------------------
CORBA::Long IOController::getValue( UniSetTypes::ObjectId sid )
{
	auto li = ioList.end();
	return localGetValue(li, sid);
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( IOController::IOStateList::iterator& li, const UniSetTypes::ObjectId sid )
{
	if( li == ioList.end() )
		li = ioList.find(sid);

	if( li != ioList.end() )
		return localGetValue(li->second, sid);

	// -------------
	ostringstream err;
	err << myname << "(localGetValue): Not found sensor (" << sid << ") "
		<< uniset_conf()->oind->getNameById(sid);

	uinfo << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( std::shared_ptr<USensorInfo>& li, const UniSetTypes::ObjectId sid )
{
	if( li )
	{
		uniset_rwmutex_rlock lock(li->val_lock);

		if( li->undefined )
			throw IOController_i::Undefined();

		return li->value;
	}

	// -------------
	ostringstream err;
	err << myname << "(localGetValue): Not found sensor (" << sid << ") "
		<< uniset_conf()->oind->getNameById(sid);

	uinfo << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
void IOController::setUndefinedState( UniSetTypes::ObjectId sid, CORBA::Boolean undefined, UniSetTypes::ObjectId sup_id )
{
	auto li = ioList.end();
	localSetUndefinedState( li, undefined, sid );
}
// -----------------------------------------------------------------------------
void IOController::localSetUndefinedState( IOStateList::iterator& li,
		bool undefined, const UniSetTypes::ObjectId sid )
{
	// сохранение текущего состояния
	if( li == ioList.end() )
		li = ioList.find(sid);

	if( li == ioList.end() )
	{
		ostringstream err;
		err << myname << "(localSetUndefined): Unknown sensor (" << sid << ")"
			<< "name: " << uniset_conf()->oind->getNameById(sid);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	bool changed = false;
	{
		// lock
		uniset_rwmutex_wrlock lock(li->second->val_lock);
		changed = (li->second->undefined != undefined);
		li->second->undefined = undefined;
	}    // unlock

	// сперва локальные события...
	try
	{
		if( changed )
		{
			uniset_rwmutex_wrlock l(li->second->undefMutex);
			li->second->sigUndefChange.emit( li->second, this);
		}
	}
	catch(...) {}

	// потом глобольное, но конкретно для 'undefchange'
	try
	{
		if( changed )
		{
			uniset_mutex_lock l(siganyundefMutex);
			sigAnyUndefChange.emit(li->second, this);
		}
	}
	catch(...) {}

	// теперь просто событие по изменению состояния
	try
	{
		if( changed )
		{
			uniset_rwmutex_wrlock(li->second->changeMutex);
			li->second->sigChange.emit(li->second, this);
		}
	}
	catch(...) {}

	// глобальное по всем..
	try
	{
		if( changed )
		{
			uniset_mutex_lock l(siganyMutex);
			sigAnyChange.emit(li->second, this);
		}
	}
	catch(...) {}
}
// ------------------------------------------------------------------------------------------
void IOController::fastSetValue( UniSetTypes::ObjectId sid, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	try
	{
		auto li = ioList.end();
		localSetValueIt( li, sid, value, sup_id );
	}
	catch(...) {}
}
// ------------------------------------------------------------------------------------------
void IOController::setValue( UniSetTypes::ObjectId sid, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	auto li = ioList.end();
	localSetValueIt( li, sid, value, sup_id );
}
// ------------------------------------------------------------------------------------------
void IOController::localSetValueIt( IOController::IOStateList::iterator& li,
									UniSetTypes::ObjectId sid,
									CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	if( sup_id == UniSetTypes::DefaultObjectId )
		sup_id = getId();

	// сохранение текущего состояния
	if( li == ioList.end() )
		li = ioList.find(sid);

	if( li == ioList.end() )
	{
		ostringstream err;
		err << myname << "(localSetValue): Unknown sensor (" << sid << ")"
			<< "name: " << uniset_conf()->oind->getNameById(sid);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	localSetValue(li->second, sid, value, sup_id);
}
// ------------------------------------------------------------------------------------------
void IOController::localSetValue( std::shared_ptr<USensorInfo>& usi,
								  UniSetTypes::ObjectId sid,
								  CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	bool changed = false;

	{
		// lock
		uniset_rwmutex_wrlock lock(usi->val_lock);

		usi->supplier = sup_id; // запоминаем того кто изменил

		// фильтрам может потребоваться измениять исходное значение (например для усреднения)
		// поэтому передаём (и затем сохраняем) напрямую(ссылку) value (а не const value)
		bool blocked = ( usi->blocked || usi->undefined );

		if( checkIOFilters(usi, value, sup_id) || blocked )
		{
			ulog4 << myname << ": save sensor value (" << sid << ")"
				  << " name: " << uniset_conf()->oind->getNameById(sid)
				  << " newvalue=" << value
				  << " value=" << usi->value
				  << " blocked=" << usi->blocked
				  << " real_value=" << usi->real_value
				  << endl;

			long prev = usi->value;

			if( blocked )
			{
				usi->real_value = value;
				usi->value = usi->d_off_value;
			}
			else
			{
				usi->value = value;
				usi->real_value = value;
			}

			changed = ( prev != usi->value );

			// запоминаем время изменения
			struct timeval tm;
			struct timezone tz;
			tm.tv_sec  = 0;
			tm.tv_usec = 0;
			gettimeofday(&tm, &tz);
			usi->tv_sec  = tm.tv_sec;
			usi->tv_usec = tm.tv_usec;
		}
	}    // unlock

	try
	{
		if( changed )
		{
			uniset_rwmutex_wrlock l(usi->changeMutex);
			usi->sigChange.emit(usi, this);
		}
	}
	catch(...) {}

	try
	{
		if( changed )
		{
			uniset_mutex_lock l(siganyMutex);
			sigAnyChange.emit(usi, this);
		}
	}
	catch(...) {}
}
// ------------------------------------------------------------------------------------------
IOType IOController::getIOType( UniSetTypes::ObjectId sid )
{
	auto ali = ioList.find(sid);

	if( ali != ioList.end() )
		return ali->second->type;

	ostringstream err;
	err << myname << "(getIOType): датчик имя: " << uniset_conf()->oind->getNameById(sid) << " не найден";
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ---------------------------------------------------------------------------
void IOController::ioRegistration( std::shared_ptr<USensorInfo>& ainf, bool force )
{
	// проверка задан ли контроллеру идентификатор
	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err << "(IOCOntroller::ioRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
		uwarn << err.str() << endl;
		throw ResolveNameError(err.str());
	}

	{
		// lock
		uniset_rwmutex_wrlock lock(ioMutex);

		if( !force )
		{
			auto li = ioList.find(ainf->si.id);

			if( li != ioList.end() )
			{
				ostringstream err;
				err << "Попытка повторной регистрации датчика(" << ainf->si.id << "). имя: "
					<< uniset_conf()->oind->getNameById(ainf->si.id);
				throw ObjectNameAlready(err.str());
			}
		}

		IOStateList::mapped_type ai = ainf;
		// запоминаем начальное время
		struct timeval tm;
		struct timezone tz;
		tm.tv_sec   = 0;
		tm.tv_usec  = 0;
		gettimeofday(&tm, &tz);
		ai->tv_sec   = tm.tv_sec;
		ai->tv_usec  = tm.tv_usec;
		ai->value    = ai->default_val;
		ai->supplier = getId();

		// более оптимальный способ(при условии вставки первый раз)
		ioList.emplace( IOStateList::value_type(ainf->si.id, std::move(ai) ));
	}

	try
	{
		for( unsigned int i = 0; i < 2; i++ )
		{
			try
			{
				uinfo << myname
					  << "(ioRegistration): регистрирую "
					  << uniset_conf()->oind->getNameById(ainf->si.id) << endl;

				ui->registered( ainf->si.id, getRef(), true );
				return;
			}
			catch( const ObjectNameAlready& ex )
			{
				uwarn << myname << "(asRegistration): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
				ui->unregister(ainf->si.id);
			}
		}
	}
	catch( const Exception& ex )
	{
		ucrit << myname << "(ioRegistration): " << ex << endl;
	}
}
// ---------------------------------------------------------------------------
void IOController::ioUnRegistration( const UniSetTypes::ObjectId sid )
{
	ui->unregister(sid);
}
// ---------------------------------------------------------------------------
void IOController::logging( UniSetTypes::SensorMessage& sm )
{
	uniset_rwmutex_wrlock l(loggingMutex);

	try
	{
		//        struct timezone tz;
		//        gettimeofday(&sm.tm,&tz);
		ObjectId dbID = uniset_conf()->getDBServer();

		// значит на этом узле нет DBServer-а
		if( dbID == UniSetTypes::DefaultObjectId )
			return;

		sm.consumer = dbID;
		TransportMessage tm(std::move(sm.transport_msg()));
		ui->send( sm.consumer, std::move(tm) );
		isPingDBServer = true;
	}
	catch(...)
	{
		if( isPingDBServer )
		{
			isPingDBServer = false;
			ucrit << myname << "(logging): DBServer unavailable" << endl;
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
void IOController::dumpToDB()
{
	// значит на этом узле нет DBServer-а
	if( uniset_conf()->getDBServer() == UniSetTypes::DefaultObjectId )
		return;

	{
		// lock
		//        uniset_mutex_lock lock(ioMutex, 100);
		for( auto li = ioList.begin(); li != ioList.end(); ++li )
		{
			if ( !li->second->dbignore )
			{
				SensorMessage sm(li->second->makeSensorMessage());
				logging(sm);
			}
		}
	}    // unlock
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorInfoSeq* IOController::getSensorsMap()
{
	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	IOController_i::SensorInfoSeq* res = new IOController_i::SensorInfoSeq();
	res->length( ioList.size());

	unsigned int i = 0;

	for( auto& it : ioList )
	{
		uniset_rwmutex_rlock lock(it.second->val_lock);
		(*res)[i] = *(it.second.get());
		i++;
	}

	return res;
}
// --------------------------------------------------------------------------------------------------------------
UniSetTypes::Message::Priority IOController::getPriority( const UniSetTypes::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it != ioList.end() )
		return (UniSetTypes::Message::Priority)it->second->priority;

	return UniSetTypes::Message::Medium; // ??
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorIOInfo IOController::getSensorIOInfo( const UniSetTypes::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it != ioList.end() )
	{
		uniset_rwmutex_rlock lock(it->second->val_lock);
		return *(it->second.get());
	}

	// -------------
	ostringstream err;
	err << myname << "(getSensorIOInfo): Unknown sensor (" << sid << ")"
		<< uniset_conf()->oind->getNameById(sid);

	uinfo << err.str() << endl;

	throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
CORBA::Long IOController::getRawValue( UniSetTypes::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it == ioList.end() )
	{
		ostringstream err;
		err << myname << "(getRawValue): Unknown analog sensor (" << sid << ")"
			<< uniset_conf()->oind->getNameById(sid);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	// ??? получаем raw из калиброванного значения ???
	IOController_i::CalibrateInfo& ci(it->second->ci);

	if( ci.maxCal != 0 && ci.maxCal != ci.minCal )
	{
		if( it->second->type == UniversalIO::AI )
			return UniSetTypes::lcalibrate(it->second->value, ci.minRaw, ci.maxRaw, ci.minCal, ci.maxCal, true);

		if( it->second->type == UniversalIO::AO )
			return UniSetTypes::lcalibrate(it->second->value, ci.minCal, ci.maxCal, ci.minRaw, ci.maxRaw, true);
	}

	return it->second->value;
}
// --------------------------------------------------------------------------------------------------------------
void IOController::calibrate( UniSetTypes::ObjectId sid,
							  const IOController_i::CalibrateInfo& ci,
							  UniSetTypes::ObjectId adminId )
{
	auto it = ioList.find(sid);

	if( it == ioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): Unknown analog sensor (" << sid << ")"
			<< uniset_conf()->oind->getNameById(sid);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	uinfo << myname << "(calibrate): from " << uniset_conf()->oind->getNameById(adminId) << endl;

	it->second->ci = ci;
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::CalibrateInfo IOController::getCalibrateInfo( UniSetTypes::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it == ioList.end() )
	{
		ostringstream err;
		err << myname << "(calibrate): Unknown analog sensor (" << sid << ")"
			<< uniset_conf()->oind->getNameById(sid);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	return it->second->ci;
}
// --------------------------------------------------------------------------------------------------------------
IOController::USensorInfo::USensorInfo( IOController_i::SensorIOInfo& ai ):
	IOController_i::SensorIOInfo(ai),
	any(0)
{}

IOController::USensorInfo::USensorInfo( const IOController_i::SensorIOInfo& ai ):
	IOController_i::SensorIOInfo(ai),
	any(0)
{}

IOController::USensorInfo::USensorInfo(IOController_i::SensorIOInfo* ai):
	IOController_i::SensorIOInfo(*ai),
	any(0)
{}

IOController::USensorInfo&
IOController::USensorInfo::operator=(IOController_i::SensorIOInfo& r)
{
	(*this) = r;
	//    any=0;
	return *this;
}

IOController::USensorInfo&
IOController::USensorInfo::operator=(IOController_i::SensorIOInfo* r)
{
	(*this) = (*r);
	//    any=0;

	return *this;
}

const IOController::USensorInfo&
IOController::USensorInfo::operator=(const IOController_i::SensorIOInfo& r)
{
	(*this) = r;
	//    any=0;
	return *this;
}

void IOController::USensorInfo::init( const IOController_i::SensorIOInfo& s )
{
	IOController::USensorInfo r(s);
	(*this) = std::move(r);
}
// ----------------------------------------------------------------------------------------
bool IOController::checkIOFilters( std::shared_ptr<USensorInfo>& ai, CORBA::Long& newvalue,
								   UniSetTypes::ObjectId sup_id )
{
	for( const auto& it : iofilters )
	{
		if( it(ai, newvalue, sup_id) == false )
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
		return --iofilters.end();
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

IOController::IOStateList::iterator IOController::myiofind( const UniSetTypes::ObjectId id )
{
	return ioList.find(id);
}
// -----------------------------------------------------------------------------
IOController_i::SensorInfoSeq* IOController::getSensorSeq( const IDSeq& lst )
{
	int size = lst.length();

	IOController_i::SensorInfoSeq* res = new IOController_i::SensorInfoSeq();
	res->length(size);

	for( auto i = 0; i < size; i++ )
	{
		auto it = ioList.find(lst[i]);

		if( it != ioList.end() )
		{
			(*res)[i] = it->second->makeSensorIOInfo();
			continue;
		}

		// элемент не найден...
		(*res)[i].si.id     = DefaultObjectId;
		(*res)[i].si.node   = DefaultObjectId;
		(*res)[i].undefined = true;
	}

	return res;
}
// -----------------------------------------------------------------------------
IDSeq* IOController::setOutputSeq(const IOController_i::OutSeq& lst, ObjectId sup_id )
{
	UniSetTypes::IDList badlist; // список не найденных идентификаторов

	int size = lst.length();

	for(int i = 0; i < size; i++)
	{
		ObjectId sid = lst[i].si.id;

		{
			auto it = ioList.find(sid);

			if( it != ioList.end() )
			{
				localSetValueIt(it, sid, lst[i].value, sup_id);
				continue;
			}
		}

		// не найден
		badlist.add(sid);
	}

	return badlist.getIDSeq();
}
// -----------------------------------------------------------------------------
IOController_i::ShortIOInfo IOController::getChangedTime( UniSetTypes::ObjectId sid )
{
	auto ait = ioList.find(sid);

	if( ait != ioList.end() )
	{
		IOController_i::ShortIOInfo i;
		auto s = ait->second;
		uniset_rwmutex_rlock lock(s->val_lock);
		i.value = s->value;
		i.tv_sec = s->tv_sec;
		i.tv_usec = s->tv_usec;
		i.supplier = s->supplier;
		return i;
	}

	// -------------
	ostringstream err;
	err << myname << "(getChangedTime): вход(выход) с именем "
		<< uniset_conf()->oind->getNameById(sid) << " не найден";

	uinfo << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// -----------------------------------------------------------------------------
IOController_i::ShortMapSeq* IOController::getSensors()
{
	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	IOController_i::ShortMapSeq* res = new IOController_i::ShortMapSeq();
	res->length( ioList.size() );

	int i = 0;

	for( const auto& it : ioList )
	{
		IOController_i::ShortMap m;
		{
			uniset_rwmutex_rlock lock(it.second->val_lock);
			m.id    = it.second->si.id;
			m.value = it.second->value;
			m.type  = it.second->type;
		}
		(*res)[i++] = m;
	}

	return res;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value( UniSetTypes::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it == ioList.end() )
	{
		ostringstream err;
		err << myname << "(signal_change_value): вход(выход) с именем "
			<< uniset_conf()->oind->getNameById(sid) << " не найден";

		uinfo << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	uniset_rwmutex_rlock lock(it->second->val_lock);
	return it->second->sigChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value()
{
	return sigAnyChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeUndefinedStateSignal IOController::signal_change_undefined_state( UniSetTypes::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it == ioList.end() )
	{
		ostringstream err;
		err << myname << "(signal_change_undefine): вход(выход) с именем "
			<< uniset_conf()->oind->getNameById(sid) << " не найден";

		uinfo << err.str() << endl;

		throw IOController_i::NameNotFound(err.str().c_str());
	}

	uniset_rwmutex_rlock lock(it->second->val_lock);
	return it->second->sigUndefChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeUndefinedStateSignal IOController::signal_change_undefined_state()
{
	return sigAnyUndefChange;
}
// -----------------------------------------------------------------------------
void IOController::USensorInfo::checkDepend( std::shared_ptr<USensorInfo>& d_it, IOController* ic )
{
	bool changed = false;
	ObjectId sup_id = ic->getId();
	{
		uniset_rwmutex_wrlock lock(val_lock);
		bool prev = blocked;
		uniset_rwmutex_rlock dlock(d_it->val_lock);
		blocked = ( d_it->value == d_value ) ? false : true;
		changed = ( prev != blocked );
		sup_id = d_it->supplier;
	}

	ulog4 << ic->getName() << "(checkDepend): check si.id=" << si.id
		  << " d_it->value=" << d_it->value
		  << " d_value=" << d_value
		  << " d_off_value=" << d_off_value
		  << " blocked=" << blocked
		  << " changed=" << changed
		  << " real_value=" << real_value
		  << endl;

	if( changed )
		ic->localSetValue( it, si.id, real_value, sup_id );
}
// -----------------------------------------------------------------------------
UniSetTypes::SimpleInfo* IOController::getInfo( ::CORBA::Long userparam )
{
	UniSetTypes::SimpleInfo_var i = UniSetManager::getInfo();

	ostringstream inf;

	inf << i->info << endl;
	inf << "isPingDBServer = " << isPingDBServer << endl;
	inf << "ioListSize = " << ioList.size() << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// -----------------------------------------------------------------------------
