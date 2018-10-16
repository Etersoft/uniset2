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
#include "ORepHelpers.h"
#include "Debug.h"
// ------------------------------------------------------------------------------------------
using namespace uniset;
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
	auto conf = uniset_conf();
	if( conf )
		dbserverID = conf->getDBServer();
}

IOController::IOController(ObjectId id):
	UniSetManager(id),
	ioMutex(string(uniset_conf()->oind->getMapName(id)) + "_ioMutex"),
	isPingDBServer(true)
{
	auto conf = uniset_conf();
	if( conf )
		dbserverID = conf->getDBServer();
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
		catch( const uniset::Exception& ex )
		{
			ucrit << myname << "(sensorsUnregistration): " << ex << endl;
		}
	}
}
// ------------------------------------------------------------------------------------------
IOController::InitSignal IOController::signal_init()
{
	return sigInit;
}
// ------------------------------------------------------------------------------------------
void IOController::activateInit()
{
	for( auto && io : ioList )
	{
		try
		{
			auto s = io.second;

			// Проверка зависимостей
			if( s->d_si.id != DefaultObjectId )
			{
				auto d_it = myiofind(s->d_si.id);

				if( d_it != myioEnd() )
					s->checkDepend( d_it->second, this);
			}

			sigInit.emit(s, this);
		}
		catch( const uniset::Exception& ex )
		{
			ucrit << myname << "(activateInit): " << ex << endl << flush;
			//std::terminate();
			uterminate();
		}
	}
}
// ------------------------------------------------------------------------------------------
CORBA::Long IOController::getValue( uniset::ObjectId sid )
{
	auto li = ioList.end();
	return localGetValue(li, sid);
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( IOController::IOStateList::iterator& li, const uniset::ObjectId sid )
{
	if( li == ioList.end() )
	{
		if( sid != DefaultObjectId )
			li = ioList.find(sid);
	}

	if( li != ioList.end() )
		return localGetValue(li->second);

	// -------------
	ostringstream err;
	err << myname << "(localGetValue): Not found sensor (" << sid << ") "
		<< uniset_conf()->oind->getNameById(sid);

	uinfo << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( std::shared_ptr<USensorInfo>& usi )
{
	if( usi )
	{
		uniset_rwmutex_rlock lock(usi->val_lock);

		if( usi->undefined )
		{
			auto ex = IOController_i::Undefined();
			ex.value = usi->value;
			throw ex;
		}

		return usi->value;
	}

	// -------------
	ostringstream err;
	err << myname << "(localGetValue): Unknown sensor";
	uinfo << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
void IOController::setUndefinedState( uniset::ObjectId sid, CORBA::Boolean undefined, uniset::ObjectId sup_id )
{
	auto li = ioList.end();
	localSetUndefinedState( li, undefined, sid );
}
// -----------------------------------------------------------------------------
void IOController::localSetUndefinedState( IOStateList::iterator& li,
		bool undefined, const uniset::ObjectId sid )
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
		auto usi = li->second;
		// lock
		uniset_rwmutex_wrlock lock(usi->val_lock);
		changed = (usi->undefined != undefined);
		usi->undefined = undefined;
		if( usi->undef_value != not_specified_value )
		{
			if( undefined )
				usi->value = usi->undef_value;
			else
				usi->value = usi->real_value;
		}

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

	// потом глобальное, но конкретно для 'undefchange'
	try
	{
		if( changed )
		{
			std::lock_guard<std::mutex> l(siganyundefMutex);
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
			std::lock_guard<std::mutex> l(siganyMutex);
			sigAnyChange.emit(li->second, this);
		}
	}
	catch(...) {}
}
// ------------------------------------------------------------------------------------------
void IOController::setValue( uniset::ObjectId sid, CORBA::Long value, uniset::ObjectId sup_id )
{
	auto li = ioList.end();
	localSetValueIt( li, sid, value, sup_id );
}
// ------------------------------------------------------------------------------------------
long IOController::localSetValueIt( IOController::IOStateList::iterator& li,
									uniset::ObjectId sid,
									CORBA::Long value, uniset::ObjectId sup_id )
{
	if( sup_id == uniset::DefaultObjectId )
		sup_id = getId();

	// сохранение текущего состояния
	if( li == ioList.end() )
	{
		if( sid != DefaultObjectId )
			li = ioList.find(sid);
	}

	if( li == ioList.end() )
	{
		ostringstream err;
		err << myname << "(localSetValue): Unknown sensor (" << sid << ")"
			<< "name: " << uniset_conf()->oind->getNameById(sid);
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	return localSetValue(li->second, value, sup_id);
}
// ------------------------------------------------------------------------------------------
long IOController::localSetValue( std::shared_ptr<USensorInfo>& usi,
								  CORBA::Long value, uniset::ObjectId sup_id )
{
	// if( !usi ) - не проверяем, т.к. считаем что это внутренние функции и несуществующий указатель передать не могут

	bool changed = false;
	bool blockChanged = false;
	long retValue = value;

	{
		// lock
		uniset_rwmutex_wrlock lock(usi->val_lock);

		usi->supplier = sup_id; // запоминаем того кто изменил

		bool blocked = ( usi->blocked || usi->undefined );
		changed = ( usi->real_value != value );

		// Смотрим поменялось ли состояние блокировки.
		// т.е. смотрим записано ли у нас уже value = d_off_value и флаг блокировки
		// т.к. если blocked=true то должно быть usi->value = usi->d_off_value
		// если флаг снимется, то значит должны "восстанавливать" значение из real_value
		blockChanged = ( blocked != (usi->value == usi->d_off_value ) );

		if( changed || blockChanged )
		{
			ulog4 << myname << "(localSetValue): (" << usi->si.id << ")"
				  << uniset_conf()->oind->getNameById(usi->si.id)
				  << " newvalue=" << value
				  << " value=" << usi->value
				  << " blocked=" << usi->blocked
				  << " real_value=" << usi->real_value
				  << " supplier=" << sup_id
				  << endl;

			usi->real_value = value;
			usi->value = (blocked ? usi->d_off_value : value);
			retValue = usi->value;

			usi->nchanges++; // статистика

			// запоминаем время изменения
			try
			{
				struct timespec tm = uniset::now_to_timespec();
				usi->tv_sec  = tm.tv_sec;
				usi->tv_nsec = tm.tv_nsec;
			}
			catch( std::exception& ex )
			{
				ucrit << myname << "(localSetValue): setValue (" << usi->si.id << ") ERROR: " << ex.what() << endl;
			}
		}
	}    // unlock

	try
	{
		if( changed || blockChanged )
		{
			uniset_rwmutex_wrlock l(usi->changeMutex);
			usi->sigChange.emit(usi, this);
		}
	}
	catch(...) {}

	try
	{
		if( changed || blockChanged )
		{
			std::lock_guard<std::mutex> l(siganyMutex);
			sigAnyChange.emit(usi, this);
		}
	}
	catch(...) {}

	return retValue;
}
// ------------------------------------------------------------------------------------------
IOType IOController::getIOType( uniset::ObjectId sid )
{
	auto ali = ioList.find(sid);

	if( ali != ioList.end() )
		return ali->second->type;

	ostringstream err;
	err << myname << "(getIOType): датчик имя: " << uniset_conf()->oind->getNameById(sid) << " не найден";
	throw IOController_i::NameNotFound(err.str().c_str());
}
// ---------------------------------------------------------------------------
void IOController::ioRegistration( std::shared_ptr<USensorInfo>& usi )
{
	// проверка задан ли контроллеру идентификатор
	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err << "(IOCOntroller::ioRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
		uwarn << err.str() << endl;
		throw ResolveNameError(err.str());
	}

	try
	{
		for( size_t i = 0; i < 2; i++ )
		{
			try
			{
				ulogrep << myname
						<< "(ioRegistration): регистрирую "
						<< uniset_conf()->oind->getNameById(usi->si.id) << endl;

				ui->registered( usi->si.id, getRef(), true );
				return;
			}
			catch( const ObjectNameAlready& ex )
			{
				uwarn << myname << "(asRegistration): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
				ui->unregister(usi->si.id);
			}
		}
	}
	catch( const uniset::Exception& ex )
	{
		ucrit << myname << "(ioRegistration): " << ex << endl;
	}
}
// ---------------------------------------------------------------------------
void IOController::ioUnRegistration( const uniset::ObjectId sid )
{
	ui->unregister(sid);
}
// ---------------------------------------------------------------------------
void IOController::logging( uniset::SensorMessage& sm )
{
	std::lock_guard<std::mutex> l(loggingMutex);

	try
	{
		// значит на этом узле нет DBServer-а
		if( dbserverID == uniset::DefaultObjectId )
		{
			isPingDBServer = false;
			return;
		}

		sm.consumer = dbserverID;
		TransportMessage tm(sm.transport_msg());
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
	if( dbserverID == uniset::DefaultObjectId )
		return;

	{
		// lock
		//        uniset_mutex_lock lock(ioMutex, 100);
		for( auto && usi : ioList )
		{
			auto& s = usi.second;

			if ( !s->dbignore )
			{
				SensorMessage sm( s->makeSensorMessage() );
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
uniset::Message::Priority IOController::getPriority( const uniset::ObjectId sid )
{
	auto it = ioList.find(sid);

	if( it != ioList.end() )
		return (uniset::Message::Priority)it->second->priority;

	return uniset::Message::Medium; // ??
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorIOInfo IOController::getSensorIOInfo( const uniset::ObjectId sid )
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
CORBA::Long IOController::getRawValue( uniset::ObjectId sid )
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
			return uniset::lcalibrate(it->second->value, ci.minRaw, ci.maxRaw, ci.minCal, ci.maxCal, true);

		if( it->second->type == UniversalIO::AO )
			return uniset::lcalibrate(it->second->value, ci.minCal, ci.maxCal, ci.minRaw, ci.maxRaw, true);
	}

	return it->second->value;
}
// --------------------------------------------------------------------------------------------------------------
void IOController::calibrate( uniset::ObjectId sid,
							  const IOController_i::CalibrateInfo& ci,
							  uniset::ObjectId adminId )
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
IOController_i::CalibrateInfo IOController::getCalibrateInfo( uniset::ObjectId sid )
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
	IOController_i::SensorIOInfo(ai)
{}

IOController::USensorInfo::USensorInfo( const IOController_i::SensorIOInfo& ai ):
	IOController_i::SensorIOInfo(ai)
{}

IOController::USensorInfo::USensorInfo(IOController_i::SensorIOInfo* ai):
	IOController_i::SensorIOInfo(*ai)
{}

IOController::USensorInfo&
IOController::USensorInfo::operator=(IOController_i::SensorIOInfo& r)
{
	IOController::USensorInfo tmp(r);
	(*this) = std::move(tmp);
	return *this;
}
// ----------------------------------------------------------------------------------------
IOController::USensorInfo::USensorInfo(): d_value(1), d_off_value(0)
{
	d_si.id = uniset::DefaultObjectId;
	d_si.node = uniset::DefaultObjectId;
	default_val = 0;
	value = default_val;
	real_value = default_val;
	dbignore = false;
	undefined = false;
	blocked = false;
	supplier = uniset::DefaultObjectId;

	// стоит ли выставлять текущее время
	// Мы теряем возможность понять (по tv_sec=0),
	// что значение ещё ни разу никем не менялось
	auto tm = uniset::now_to_timespec();
	tv_sec = tm.tv_sec;
	tv_nsec = tm.tv_nsec;
}
// ----------------------------------------------------------------------------------------
IOController::USensorInfo&
IOController::USensorInfo::operator=( IOController_i::SensorIOInfo* r )
{
	IOController::USensorInfo tmp(r);
	(*this) = std::move(tmp);
	return *this;
}
// ----------------------------------------------------------------------------------------
void* IOController::USensorInfo::getUserData( size_t index )
{
	if( index >= MaxUserData )
		return nullptr;

	uniset::uniset_rwmutex_rlock ulock(userdata_lock);
	return userdata[index];
}

void IOController::USensorInfo::setUserData( size_t index, void* data )
{
	if( index >= MaxUserData )
		return;

	uniset::uniset_rwmutex_wrlock ulock(userdata_lock);
	userdata[index] = data;
}
// ----------------------------------------------------------------------------------------
const IOController::USensorInfo&
IOController::USensorInfo::operator=( const IOController_i::SensorIOInfo& r )
{
	IOController::USensorInfo tmp(r);
	(*this) = std::move(tmp);
	return *this;
}
// ----------------------------------------------------------------------------------------
void IOController::USensorInfo::init( const IOController_i::SensorIOInfo& s )
{
	IOController::USensorInfo r(s);
	(*this) = std::move(r);
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myioBegin()
{
	return ioList.begin();
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myioEnd()
{
	return ioList.end();
}
// ----------------------------------------------------------------------------------------
void IOController::initIOList( const IOController::IOStateList&& l )
{
	ioList = std::move(l);
}
// ----------------------------------------------------------------------------------------
void IOController::for_iolist( IOController::UFunction f )
{
	uniset_rwmutex_rlock lck(ioMutex);

	for( auto && s : ioList )
		f(s.second);
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myiofind( const uniset::ObjectId id )
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
	uniset::IDList badlist; // список не найденных идентификаторов

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
IOController_i::ShortIOInfo IOController::getTimeChange( uniset::ObjectId sid )
{
	auto ait = ioList.find(sid);

	if( ait != ioList.end() )
	{
		IOController_i::ShortIOInfo i;
		auto s = ait->second;
		uniset_rwmutex_rlock lock(s->val_lock);
		i.value = s->value;
		i.tv_sec = s->tv_sec;
		i.tv_nsec = s->tv_nsec;
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
IOController::ChangeSignal IOController::signal_change_value( uniset::ObjectId sid )
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

	return it->second->sigChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value()
{
	return sigAnyChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeUndefinedStateSignal IOController::signal_change_undefined_state( uniset::ObjectId sid )
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

	//	uniset_rwmutex_rlock lock(it->second->val_lock);
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
		blocked = ( d_it->value != d_value );
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
		ic->localSetValue( d_usi, real_value, sup_id );
}
// -----------------------------------------------------------------------------
uniset::SimpleInfo* IOController::getInfo( const char* userparam )
{
	uniset::SimpleInfo_var i = UniSetManager::getInfo(userparam);

	ostringstream inf;

	inf << i->info << endl;
	inf << "isPingDBServer = " << isPingDBServer << endl;
	inf << "ioListSize = " << ioList.size() << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
Poco::JSON::Object::Ptr IOController::httpHelp( const Poco::URI::QueryParameters& p )
{
	uniset::json::help::object myhelp( myname, UniSetManager::httpHelp(p) );

	{
		// 'get'
		uniset::json::help::item cmd("get", "get value for sensor");
		cmd.param("id1,name2,id3", "get value for id1,name2,id3 sensors");
		cmd.param("shortInfo", "get short information for sensors");
		myhelp.add(cmd);
	}

	{
		// 'sensors'
		uniset::json::help::item cmd("sensors", "get all sensors");
		cmd.param("nameonly", "get only name sensors");
		cmd.param("offset=N", "get from N record");
		cmd.param("limit=M", "limit of records");
		myhelp.add(cmd);
	}

	return myhelp;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
	if( req == "get" )
		return request_get(req, p);

	if( req == "sensors" )
		return request_sensors(req, p);

	return UniSetManager::httpRequest(req, p);
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_get( const string& req, const Poco::URI::QueryParameters& p )
{
	if( p.empty() )
	{
		ostringstream err;
		err << myname << "(request): 'get'. Unknown ID or Name. Use parameters: get?ID1,name2,ID3,...";
		throw uniset::SystemError(err.str());
	}

	auto conf = uniset_conf();
	auto slist = uniset::getSInfoList( p[0].first, conf );

	if( slist.empty() )
	{
		ostringstream err;
		err << myname << "(request): 'get'. Unknown ID or Name. Use parameters: get?ID1,name2,ID3,...";
		throw uniset::SystemError(err.str());
	}

	bool shortInfo = false;

	if( p.size() > 1 && p[1].first == "shortInfo" )
		shortInfo = true;

	// {
	//	 "sensors" [
	//           { name: string, value: long, error: string, ...},
	//           { name: string, value: long, error: string, ...},
	//           ...
	//	 ],
	//
	//	 "object" { mydata... }
	//	}

	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
	auto my = httpGetMyInfo(jdata);
	auto jsens = uniset::json::make_child_array(jdata, "sensors");

	for( const auto& s : slist )
	{
		try
		{
			auto sinf = ioList.find(s.si.id);

			if( sinf == ioList.end() )
			{
				Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
				jr->set("name", s.fname);
				jr->set("error", "Sensor not found");
				jsens->add(jr);
				continue;
			}

			getSensorInfo(jsens, sinf->second, shortInfo);
		}
		catch( IOController_i::NameNotFound& ex )
		{
			Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
			jr->set("name", s.fname);
			jr->set("error", string(ex.err));
			jsens->add(jr);
		}
		catch( std::exception& ex )
		{
			Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
			jr->set("name", s.fname);
			jr->set("error", string(ex.what()));
			jsens->add(jr);
		}
	}

	return jdata;
}
// -----------------------------------------------------------------------------
void IOController::getSensorInfo( Poco::JSON::Array::Ptr& jdata, std::shared_ptr<USensorInfo>& s, bool shortInfo )
{
	Poco::JSON::Object::Ptr jsens = new Poco::JSON::Object();
	jdata->add(jsens);

	{
		uniset_rwmutex_rlock lock(s->val_lock);
		jsens->set("value", s->value);
		jsens->set("real_value", s->real_value);
	}

	jsens->set("id", s->si.id);
	jsens->set("name", ORepHelpers::getShortName(uniset_conf()->oind->getMapName(s->si.id)));
	jsens->set("tv_sec", s->tv_sec);
	jsens->set("tv_nsec", s->tv_nsec);

	if( shortInfo )
		return;

	jsens->set("type", uniset::iotype2str(s->type));
	jsens->set("default_val", s->default_val);
	jsens->set("dbignore", s->dbignore);
	jsens->set("nchanges", s->nchanges);

	Poco::JSON::Object::Ptr calibr = uniset::json::make_child(jsens, "calibration");
	calibr->set("cmin", s->ci.minCal);
	calibr->set("cmax", s->ci.maxCal);
	calibr->set("rmin", s->ci.minRaw);
	calibr->set("rmax", s->ci.maxRaw);
	calibr->set("precision", s->ci.precision);

	//	::CORBA::Boolean undefined;
	//	::CORBA::Boolean blocked;
	//	::CORBA::Long priority;
	//	IOController_i::SensorInfo d_si = { uniset::DefaultObjectId, uniset::DefaultObjectId };  /*!< идентификатор датчика, от которого зависит данный */
	//	long d_value = { 1 }; /*!< разрешающее работу значение датчика от которого зависит данный */
	//	long d_off_value = { 0 }; /*!< блокирующее значение */
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_sensors( const string& req, const Poco::URI::QueryParameters& params )
{
	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
	Poco::JSON::Array::Ptr jsens = uniset::json::make_child_array(jdata, "sensors");
	auto my = httpGetMyInfo(jdata);

	size_t num = 0;
	size_t offset = 0;
	size_t limit = 0;
	size_t count = 0;

	for( const auto& p : params )
	{
		if( p.first == "offset" )
			offset = uni_atoi(p.second);
		else if( p.first == "limit" )
			limit = uni_atoi(p.second);
	}

	size_t endnum = offset + limit;

	for( auto it = myioBegin(); it != myioEnd(); ++it, num++ )
	{
		if( limit > 0 && num >= endnum )
			break;

		if( offset > 0 && num < offset )
			continue;

		getSensorInfo(jsens, it->second, false);
		count++;
	}

	jdata->set("count", count);
	jdata->set("size", ioCount());
	return jdata;
}
// -----------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
