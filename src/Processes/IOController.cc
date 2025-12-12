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
#include <unordered_set>
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
    for( auto&& io : ioList )
    {
        try
        {
            auto s = io.second;

            if( s->acl && s->acl->defaultPermissions == AccessNone )
                s->acl->defaultPermissions = defaultAccessMask;

            // Проверка зависимостей
            if( s->depend_sid != DefaultObjectId )
            {
                auto d_it = myiofind(s->depend_sid);

                if( d_it != myioEnd() )
                    s->checkDepend( d_it->second, this);
            }

            sigInit.emit(s, this);
        }
        catch( const uniset::Exception& ex )
        {
            ucrit << myname << "(activateInit): " << ex << endl << flush;
            uterminate();
        }
    }
}
// ------------------------------------------------------------------------------------------
CORBA::Long IOController::getValue( uniset::ObjectId sid, uniset::ObjectId sup_id )
{
    auto li = ioList.end();
    return localGetValue(li, sid, sup_id);
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( IOController::IOStateList::iterator& li, const uniset::ObjectId sid, const uniset::ObjectId consumer_id )
{
    if( li == ioList.end() )
    {
        if( sid != DefaultObjectId )
            li = ioList.find(sid);
    }

    if( li != ioList.end() )
        return localGetValue(li->second, consumer_id);

    // -------------
    ostringstream err;
    err << myname << "(localGetValue): Not found sensor (" << sid << ") "
        << uniset_conf()->oind->getNameById(sid);

    uinfo << err.str() << endl;
    throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( std::shared_ptr<USensorInfo>& usi, const uniset::ObjectId consumer_id )
{
    if( usi )
    {
        if( consumer_id != getId() && !usi->checkMask(consumer_id, defaultAccessMask).canRead() )
        {
            ostringstream err;
            err << myname << "(localGetValue): Access denied";
            uinfo << err.str() << endl;
            throw IOController_i::AccessDenied(err.str().c_str());
        }

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
    // TODO: check access control
    //    if( !usi->checkMask(consumer_id).canWrite() )
    //    {
    //        ostringstream err;
    //        err << myname << "(localSetUndefinedState): Access denied";
    //        uinfo << err.str() << endl;
    //        throw IOController_i::AccessDenied(err.str().c_str());
    //    }

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

        if( usi->readonly )
        {
            ostringstream err;
            err << myname << "(localSetUndefined): Readonly sensor (" << sid << ")"
                << "name: " << uniset_conf()->oind->getNameById(sid);
            throw IOController_i::IOBadParam(err.str().c_str());
        }

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
void IOController::freezeValue( uniset::ObjectId sid,
                                CORBA::Boolean set,
                                CORBA::Long value,
                                uniset::ObjectId sup_id )
{
    auto li = ioList.end();
    localFreezeValueIt( li, sid, set, value, sup_id );
}
// ------------------------------------------------------------------------------------------
void IOController::localFreezeValueIt( IOController::IOStateList::iterator& li,
                                       uniset::ObjectId sid,
                                       CORBA::Boolean set,
                                       CORBA::Long value,
                                       uniset::ObjectId sup_id )
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
        err << myname << "(localFreezeValue): Unknown sensor (" << sid << ")"
            << "name: " << uniset_conf()->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    localFreezeValue(li->second, set, value, sup_id);
}
// ------------------------------------------------------------------------------------------
void IOController::localFreezeValue( std::shared_ptr<USensorInfo>& usi,
                                     CORBA::Boolean set,
                                     CORBA::Long value,
                                     uniset::ObjectId sup_id )
{
    if( sup_id != getId() && !usi->checkMask(sup_id, defaultAccessMask).canWrite() )
    {
        ostringstream err;
        err << myname << "(localFreezeValue): Access denied";
        uinfo << err.str() << endl;
        throw IOController_i::AccessDenied(err.str().c_str());
    }

    if( usi->readonly )
    {
        ostringstream err;
        err << myname << "(localFreezeValue): Readonly sensor (" << usi->si.id << ")"
            << "name: " << uniset_conf()->oind->getNameById(usi->si.id);
        throw IOController_i::IOBadParam(err.str().c_str());
    }

    ulog4 << myname << "(localFreezeValue): (" << usi->si.id << ")"
          << uniset_conf()->oind->getNameById(usi->si.id)
          << " value=" << value
          << " set=" << set
          << " supplier=" << sup_id
          << endl;

    {
        // выставляем флаг заморозки
        uniset_rwmutex_wrlock lock(usi->val_lock);
        usi->frozen = set;
        usi->frozen_value = set ? value : usi->value;
        value = usi->real_value;
    }

    localSetValue(usi, value, sup_id);
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
    if( sup_id != getId() && !usi->checkMask(sup_id, defaultAccessMask).canWrite() )
    {
        ostringstream err;
        err << myname << "(localSetValue): Access denied";
        uinfo << err.str() << endl;
        throw IOController_i::AccessDenied(err.str().c_str());
    }

    if( usi->readonly )
    {
        ostringstream err;
        err << myname << "(localSetValue): Readonly sensor (" << usi->si.id << ")"
            << "name: " << uniset_conf()->oind->getNameById(usi->si.id);
        throw IOController_i::IOBadParam(err.str().c_str());
    }

    bool changed = false;
    bool blockChanged = false;
    bool freezeChanged = false;
    long retValue = value;

    {
        // lock
        uniset_rwmutex_wrlock lock(usi->val_lock);

        usi->supplier = sup_id; // запоминаем того кто изменил

        bool blocked = ( usi->blocked || usi->undefined );
        changed = ( usi->real_value != value );

        // Выставление запоненного значения (real_value)
        // если снялась блокировка или заморозка
        blockChanged = ( blocked != (usi->value == usi->d_off_value) );
        freezeChanged = ( usi->frozen != (usi->value == usi->frozen_value) );

        if( changed || blockChanged || freezeChanged )
        {
            ulog4 << myname << "(localSetValue): (" << usi->si.id << ")"
                  << uniset_conf()->oind->getNameById(usi->si.id)
                  << " newvalue=" << value
                  << " value=" << usi->value
                  << " blocked=" << usi->blocked
                  << " frozen=" << usi->frozen
                  << " real_value=" << usi->real_value
                  << " supplier=" << sup_id
                  << endl;

            usi->real_value = value;

            if( usi->frozen )
                usi->value = usi->frozen_value;
            else
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
        if( changed || blockChanged || freezeChanged )
        {
            uniset_rwmutex_wrlock l(usi->changeMutex);
            usi->sigChange.emit(usi, this);
        }
    }
    catch(...) {}

    try
    {
        if( changed || blockChanged || freezeChanged )
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
        throw;
    }
}
// ---------------------------------------------------------------------------
void IOController::ioUnRegistration( const uniset::ObjectId sid )
{
    ui->unregister(sid);
}
// ---------------------------------------------------------------------------
void IOController::setDBServer( const std::shared_ptr<DBServer>& db )
{
    dbserver = db;
}
// ---------------------------------------------------------------------------
void IOController::logging( uniset::SensorMessage& sm )
{
    std::lock_guard<std::mutex> l(loggingMutex);

    try
    {
        if( dbserver )
        {
            sm.consumer = dbserverID;
            dbserver->push(sm.transport_msg());
            isPingDBServer = true;
            return;
        }

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
        for( auto&& usi : ioList )
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
IOController_i::SensorInfoSeq* IOController::getSensorsMap( const uniset::ObjectId consumer_id )
{
    // TODO: check access control

    // ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
    // поэтому ему лучше пользоваться при получении _var-классом
    IOController_i::SensorInfoSeq* res = new IOController_i::SensorInfoSeq();
    res->length( ioList.size());

    unsigned int i = 0;

    for( const auto& it : ioList )
    {
        uniset_rwmutex_rlock lock(it.second->val_lock);
        (*res)[i] = *(it.second.get());
        i++;
    }

    return res;
}
// --------------------------------------------------------------------------------------------------------------
uniset::Message::Priority IOController::getPriority( const uniset::ObjectId sid, const uniset::ObjectId consumer_id )
{
    auto it = ioList.find(sid);

    if( it != ioList.end() )
    {
        if( consumer_id != getId() && !it->second->checkMask(consumer_id, defaultAccessMask).canRead() )
        {
            ostringstream err;
            err << myname << "(getPriority): Access denied";
            uinfo << err.str() << endl;
            throw IOController_i::AccessDenied(err.str().c_str());
        }

        return (uniset::Message::Priority) it->second->priority;
    }

    return uniset::Message::Medium; // ??
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorIOInfo IOController::getSensorIOInfo( const uniset::ObjectId sid, const uniset::ObjectId consumer_id )
{
    auto it = ioList.find(sid);

    if( it != ioList.end() )
    {
        if( consumer_id != getId() && !it->second->checkMask(consumer_id, defaultAccessMask).canRead() )
        {
            ostringstream err;
            err << myname << "(getSensorIOInfo): Access denied";
            uinfo << err.str() << endl;
            throw IOController_i::AccessDenied(err.str().c_str());
        }

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
CORBA::Long IOController::getRawValue( uniset::ObjectId sid, const uniset::ObjectId consumer_id )
{
    auto it = ioList.find(sid);

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(getRawValue): Unknown analog sensor (" << sid << ")"
            << uniset_conf()->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    if( consumer_id != getId() && !it->second->checkMask(consumer_id, defaultAccessMask).canRead() )
    {
        ostringstream err;
        err << myname << "(getRawValue): Access denied";
        uinfo << err.str() << endl;
        throw IOController_i::AccessDenied(err.str().c_str());
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
                              uniset::ObjectId sup_id )
{
    auto it = ioList.find(sid);

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(calibrate): Unknown analog sensor (" << sid << ")"
            << uniset_conf()->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    if( sup_id != getId() && !it->second->checkMask(sup_id, defaultAccessMask).canWrite() )
    {
        ostringstream err;
        err << myname << "(calibrate): Access denied";
        uinfo << err.str() << endl;
        throw IOController_i::AccessDenied(err.str().c_str());
    }

    uinfo << myname << "(calibrate): from " << uniset_conf()->oind->getNameById(sup_id) << endl;

    it->second->ci = ci;
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::CalibrateInfo IOController::getCalibrateInfo( uniset::ObjectId sid, const uniset::ObjectId consumer_id )
{
    auto it = ioList.find(sid);

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(calibrate): Unknown analog sensor (" << sid << ")"
            << uniset_conf()->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    if( consumer_id != getId() && !it->second->checkMask(consumer_id, defaultAccessMask).canRead() )
    {
        ostringstream err;
        err << myname << "(getCalibrateInfo): Access denied";
        uinfo << err.str() << endl;
        throw IOController_i::AccessDenied(err.str().c_str());
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
    depend_sid = uniset::DefaultObjectId;
    default_val = 0;
    value = default_val;
    real_value = default_val;
    dbignore = false;
    undefined = false;
    blocked = false;
    frozen = false;
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

    for( auto&& s : ioList )
        f(s.second);
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myiofind( const uniset::ObjectId id )
{
    return ioList.find(id);
}
// -----------------------------------------------------------------------------
void IOController::setDefaultAccessMask( uniset::AccessMask m )
{
    defaultAccessMask = m;
}
// -----------------------------------------------------------------------------
IOController_i::SensorInfoSeq* IOController::getSensorSeq( const IDSeq& lst, const uniset::ObjectId consumer_id )
{
    // TODO: check access control
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
IDSeq* IOController::setOutputSeq(const IOController_i::OutSeq& lst, const ObjectId sup_id )
{
    // TODO: check access control

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
IOController_i::ShortIOInfo IOController::getTimeChange( uniset::ObjectId sid, const uniset::ObjectId consumer_id )
{
    // TODO: check access control
    auto ait = ioList.find(sid);

    if( ait != ioList.end() )
    {
        if( consumer_id != getId() && !ait->second->checkMask(consumer_id, defaultAccessMask).canRead() )
        {
            ostringstream err;
            err << myname << "(getTimeChange): Access denied";
            uinfo << err.str() << endl;
            throw IOController_i::AccessDenied(err.str().c_str());
        }

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
IOController_i::ShortMapSeq* IOController::getSensors( const uniset::ObjectId consumer_id )
{
    // TODO: check access control

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

    //  uniset_rwmutex_rlock lock(it->second->val_lock);
    return it->second->sigUndefChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeUndefinedStateSignal IOController::signal_change_undefined_state()
{
    return sigAnyUndefChange;
}
// -----------------------------------------------------------------------------
void IOController::reloadACLConfig( uniset::ACLMap& amap, uniset::ACLInfoMap& iomap )
{
    for( const auto& io: iomap )
    {
        auto s = ioList.find(io.second.sid);
        if( s == ioList.end() )
            continue;

        auto acl = amap.find(io.second.name);
        if( acl == amap.end() )
        {
            s->second->acl = nullptr;
            continue;
        }

        // update sensor ACL
        s->second->acl = acl->second;
    }
}
// -----------------------------------------------------------------------------
uniset::AccessMask IOController::USensorInfo::checkMask( uniset::ObjectId sup, const uniset::AccessMask& defaultMask ) const
{
    auto result = AccessUnknown;
    // копируем т.к. параллельно может произойти reload() и подмена acl
    auto a = acl;
    if( a )
    {
        auto it = a->permissions.find(sup);
        if( it != a->permissions.end() )
            result = it->second;

        if( result == AccessUnknown && a->defaultPermissions != AccessUnknown )
            result = a->defaultPermissions;
    }

    if( result == AccessUnknown )
        result = defaultMask;

    return result;
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
        cmd.param("filter=id1,name2,id3", "get value for id1,name2,id3 sensors (mixed ID/name format)");
        cmd.param("shortInfo", "[optional] get short information for sensors");
//        cmd.param("supplier", "[optional] But required for access control");
        myhelp.add(cmd);
    }

    {
        // 'set'
        uniset::json::help::item cmd("set", "set value for sensor");
        cmd.param("id1=val1&name2=val2&id3=val3", "set value for sensors id1,name2,id3");
        cmd.param("supplier", "[optional] name of the process that changes sensors (must be first in request)");
        myhelp.add(cmd);
    }

    {
        // 'freeze'
        uniset::json::help::item cmd("freeze", "freeze value for sensor");
        cmd.param("id1=val1&name2=val2&id3=val3", "freeze value for sensors id1,name2,id3");
        cmd.param("supplier", "[optional] name of the process that changes sensors (must be first in request)");
        myhelp.add(cmd);
    }

    {
        // 'unfreeze'
        uniset::json::help::item cmd("unfreeze", "unfreeze value for sensor");
        cmd.param("id1&name2&id3", "unfreeze value for sensors id1,name2,id3");
        cmd.param("supplier", "[optional] name of the process that changes sensors (must be first in request)");
        myhelp.add(cmd);
    }

    {
        // 'sensors'
        uniset::json::help::item cmd("sensors", "get all sensors with filtering and pagination");
        cmd.param("nameonly", "get only name sensors");
        cmd.param("offset=N", "get from N record");
        cmd.param("limit=M", "limit of records");
        cmd.param("search", "text search by sensor name (case-insensitive substring)");
        cmd.param("filter=id1,name2,id3", "filter by ID or name (mixed format, comma-separated)");
        cmd.param("iotype", "filter by type: AI|AO|DI|DO");
//        cmd.param("supplier", "[optional] But required for access control");
        myhelp.add(cmd);
    }

    return myhelp;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::httpRequest( const UHttp::HttpRequestContext& ctx )
{
    // /api/v2/ObjectName/get?...
    if( ctx.depth() >= 1 && ctx[0] == "get" )
        return request_get(ctx[0], ctx.params);

    // /api/v2/ObjectName/set?...
    if( ctx.depth() >= 1 && ctx[0] == "set" )
        return request_set(ctx[0], ctx.params);

    // /api/v2/ObjectName/freeze?...
    if( ctx.depth() >= 1 && ctx[0] == "freeze" )
        return request_freeze(ctx[0], ctx.params, true);

    // /api/v2/ObjectName/unfreeze?...
    if( ctx.depth() >= 1 && ctx[0] == "unfreeze" )
        return request_freeze(ctx[0], ctx.params, false);

    // /api/v2/ObjectName/sensors?...
    if( ctx.depth() >= 1 && ctx[0] == "sensors" )
        return request_sensors(ctx[0], ctx.params);

    return UniSetObject::httpRequest(ctx);
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_get( const string& req, const Poco::URI::QueryParameters& p )
{
    // TODO access control

    if( p.empty() )
    {
        ostringstream err;
        err << myname << "(request): 'get'. Unknown ID or Name. Use parameters: get?filter=ID1,name2,ID3,...";
        throw uniset::SystemError(err.str());
    }

    auto conf = uniset_conf();
    std::string filterParam;
    bool shortInfo = false;

    for( const auto& param : p )
    {
        if( param.first == "filter" && !param.second.empty() )
            filterParam = param.second;
        else if( param.first == "shortInfo" )
            shortInfo = true;
    }

    // backward compatibility: if filter= not found, use first key as filter
    if( filterParam.empty() )
        filterParam = p[0].first;

    auto slist = uniset::getSInfoList( filterParam, conf );

    if( slist.empty() )
    {
        ostringstream err;
        err << myname << "(request): 'get'. Unknown ID or Name. Use parameters: get?filter=ID1,name2,ID3,...";
        throw uniset::SystemError(err.str());
    }

    // {
    //   "sensors" [
    //           { name: string, value: long, error: string, ...},
    //           { name: string, value: long, error: string, ...},
    //           ...
    //   ],
    //
    //   "object" { mydata... }
    //  }

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
                jr->set("error", "not found");
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
Poco::JSON::Object::Ptr IOController::request_set( const string& req, const Poco::URI::QueryParameters& p )
{
    if( disabledHttpSetApi )
    {
        std::ostringstream err;
        err << "(IHttpRequest::Request): " << req << " disabled";
        throw uniset::SystemError(err.str());
    }

    if( p.empty() )
    {
        ostringstream err;
        err << myname << "(request_set): 'set'. Unknown ID or Name. Use parameters: set?ID1=val1&name2=val2&ID3=val3&...";
        throw uniset::SystemError(err.str());
    }

    // {
    //   "errors" [
    //           { name: string, error: string },
    //           { name: string, error: string },
    //           ...
    //   ],
    //
    //   "object" { mydata... }
    //  }

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    auto my = httpGetMyInfo(jdata);
    auto jerrs = uniset::json::make_child_array(jdata, "errors");

    auto conf = uniset_conf();
    uniset::ObjectId sup_id = DefaultObjectId;

    bool skipFirst = false;

    if( p[0].first == "supplier" )
    {
        sup_id = conf->getObjectID(p[0].second);
        skipFirst = true;
    }

    if( !disableHttpAccessControl )
    {
        if( sup_id == DefaultObjectId )
        {
            ostringstream err;
            err << myname << "(request_set): 'set' requires 'supplier' parameter. Example: /set?supplier=Name&...";
            throw uniset::SystemError(err.str());
        }
    }
    else if( sup_id == DefaultObjectId )
        sup_id = getId();

    for( const auto& p : p )
    {
        if( skipFirst )
        {
            skipFirst = false;
            continue;
        }

#if __cplusplus >= 201703L
        auto s = uniset::parseSInfo_sv(p.first, conf);
        s.val = uniset::uni_atoi_sv(p.second);
#else
        auto s = uniset::parseSInfo(p.first, conf);
        s.val = uniset::uni_atoi(p.second);
#endif

        if( s.si.id != uniset::DefaultObjectId )
        {
            try
            {
                setValue(s.si.id, s.val, sup_id );
            }
            catch( IOController_i::NameNotFound& ex )
            {
                Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
                jr->set("name", s.fname);
                jr->set("error", string(ex.err));
                jerrs->add(jr);
            }
            catch( std::exception& ex )
            {
                Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
                jr->set("name", s.fname);
                jr->set("error", string(ex.what()));
                jerrs->add(jr);
            }
        }
        else
        {
            Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
            jr->set("name", p.first);
            jr->set("error", "not found");
            jerrs->add(jr);
        }
    }

    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_freeze( const string& req, const Poco::URI::QueryParameters& p, bool set )
{
    if( disabledHttpFreezeApi )
    {
        std::ostringstream err;
        err << "(IHttpRequest::Request): " << req << " disabled";
        throw uniset::SystemError(err.str());
    }

    if( p.empty() )
    {
        ostringstream err;
        err << myname << "(request_freeze): 'freeze/unfreeze'. Unknown ID or Name. Use parameters: freeze?ID1=val1&name2=val2&ID3=val3&...";
        throw uniset::SystemError(err.str());
    }

    // {
    //   "errors" [
    //           { name: string, error: string },
    //           { name: string, error: string },
    //           ...
    //   ],
    //
    //   "object" { mydata... }
    //  }

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    auto my = httpGetMyInfo(jdata);
    auto jerrs = uniset::json::make_child_array(jdata, "errors");

    auto conf = uniset_conf();
    uniset::ObjectId sup_id = DefaultObjectId;

    bool skipFirst = false;

    if( p[0].first == "supplier" )
    {
        sup_id = conf->getObjectID(p[0].second);
        skipFirst = true;
    }

    if( !disableHttpAccessControl )
    {
        if( sup_id == DefaultObjectId )
        {
            ostringstream err;
            err << myname << "(request_freeze): 'freeze/unfreeze' requires 'supplier' parameter. Example: /set?supplier=Name&...";
            throw uniset::SystemError(err.str());
        }
    }
    else if( sup_id == DefaultObjectId )
        sup_id = getId();

    for( const auto& p : p )
    {
        if( skipFirst )
        {
            skipFirst = false;
            continue;
        }

#if __cplusplus >= 201703L
        auto s = uniset::parseSInfo_sv(p.first, conf);
        s.val = uniset::uni_atoi_sv(p.second);
#else
        auto s = uniset::parseSInfo(p.first, conf);
        s.val = uniset::uni_atoi(p.second);
#endif

        if( s.si.id != uniset::DefaultObjectId )
        {
            try
            {
                freezeValue(s.si.id, set, s.val, sup_id );
            }
            catch( IOController_i::NameNotFound& ex )
            {
                Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
                jr->set("name", s.fname);
                jr->set("error", string(ex.err));
                jerrs->add(jr);
            }
            catch( std::exception& ex )
            {
                Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
                jr->set("name", s.fname);
                jr->set("error", string(ex.what()));
                jerrs->add(jr);
            }
        }
        else
        {
            Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
            jr->set("name", p.first);
            jr->set("error", "not found");
            jerrs->add(jr);
        }
    }

    return jdata;
}
// -----------------------------------------------------------------------------
void IOController::getSensorInfo( Poco::JSON::Array::Ptr& jdata, std::shared_ptr<USensorInfo>& s, bool shortInfo )
{
    // TODO access control

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
    jsens->set("undefined", s->undefined);
    jsens->set("frozen", s->frozen);
    jsens->set("blocked", s->blocked);
    jsens->set("readonly", s->readonly);

    if( s->depend_sid != DefaultObjectId )
    {
        jsens->set("depend_sensor", ORepHelpers::getShortName(uniset_conf()->oind->getMapName(s->depend_sid)));
        jsens->set("depend_sensor_id", s->depend_sid);
        jsens->set("depend_value", s->d_value);
        jsens->set("depend_off_value", s->d_off_value);
    }

    Poco::JSON::Object::Ptr calibr = uniset::json::make_child(jsens, "calibration");
    calibr->set("cmin", s->ci.minCal);
    calibr->set("cmax", s->ci.maxCal);
    calibr->set("rmin", s->ci.minRaw);
    calibr->set("rmax", s->ci.maxRaw);
    calibr->set("precision", s->ci.precision);

    //  ::CORBA::Boolean undefined;
    //  ::CORBA::Boolean blocked;
    //  ::CORBA::Long priority;
    //  long d_value = { 1 }; /*!< разрешающее работу значение датчика от которого зависит данный */
    //  long d_off_value = { 0 }; /*!< блокирующее значение */
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_sensors( const string& req, const Poco::URI::QueryParameters& params )
{
    // TODO access control

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    Poco::JSON::Array::Ptr jsens = uniset::json::make_child_array(jdata, "sensors");
    auto my = httpGetMyInfo(jdata);

    size_t offset = 0;
    size_t limit = 0;
    std::string search;  // text search by name (substring, case-insensitive)
    std::string filter;  // filter by ID/name (comma-separated list)
    UniversalIO::IOType iotypeFilter = UniversalIO::UnknownIOType;

    for( const auto& p : params )
    {
        if( p.first == "offset" )
            offset = uni_atoi(p.second);
        else if( p.first == "limit" )
            limit = uni_atoi(p.second);
        else if( p.first == "search" && !p.second.empty() )
            search = p.second;
        else if( p.first == "filter" && !p.second.empty() )
            filter = p.second;
        else if( p.first == "iotype" && !p.second.empty() )
            iotypeFilter = uniset::getIOType(p.second);
    }

    // For backward compatibility: if search is AI/AO/DI/DO and iotype is empty, treat as iotype
    if( iotypeFilter == UniversalIO::UnknownIOType && !search.empty() )
    {
        auto t = uniset::getIOType(search);
        if( t != UniversalIO::UnknownIOType )
        {
            iotypeFilter = t;
            search.clear();
        }
    }

    auto conf = uniset_conf();

    // Build filter set from filter parameter (ID/name list)
    std::unordered_set<uniset::ObjectId> filterIds;
    if( !filter.empty() )
    {
        auto slist = uniset::getSInfoList(filter, conf);
        filterIds.reserve(slist.size());
        for( const auto& s : slist )
            filterIds.insert(s.si.id);
    }

    size_t total = 0;
    size_t skipped = 0;
    size_t count = 0;

    for( auto it = myioBegin(); it != myioEnd(); ++it )
    {
        auto& s = it->second;

        // Apply filter by ID/name (if specified)
        if( !filterIds.empty() && filterIds.find(s->si.id) == filterIds.end() )
            continue;

        // Apply iotype filter (enum comparison - fast)
        if( iotypeFilter != UniversalIO::UnknownIOType && s->type != iotypeFilter )
            continue;

        // Apply text search (case-insensitive substring match by name)
        if( !search.empty() )
        {
            std::string sensorName = ORepHelpers::getShortName(conf->oind->getMapName(s->si.id));
            if( !uniset::containsIgnoreCase(sensorName, search) )
                continue;
        }

        total++;

        // Apply offset
        if( skipped < offset )
        {
            skipped++;
            continue;
        }

        // Apply limit (0 = no limit)
        if( limit > 0 && count >= limit )
            continue;

        getSensorInfo(jsens, it->second, false);
        count++;
    }

    jdata->set("count", count);
    jdata->set("total", total);
    jdata->set("size", ioCount());
    return jdata;
}
// -----------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
