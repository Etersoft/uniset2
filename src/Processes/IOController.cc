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
    ioMutex(name+"_ioMutex"),
    isPingDBServer(true)
{
}

IOController::IOController(ObjectId id):
    UniSetManager(id),
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
    bool res = UniSetManager::activateObject();
    sensorsRegistration();

    // Начальная инициализация
    activateInit();

    return res;
}
// ------------------------------------------------------------------------------------------
bool IOController::disactivateObject()
{
    sensorsUnregistration();
    return UniSetManager::disactivateObject();
}
// ------------------------------------------------------------------------------------------
void IOController::sensorsUnregistration()
{
    // Разрегистрируем аналоговые датчики
    for( IOStateList::iterator li = ioList.begin(); li!=ioList.end(); ++li)
    {
        try
        {
            ioUnRegistration( li->second.si.id );
        }
        catch( Exception& ex )
        {
            ucrit << myname << "(sensorsUnregistration): "<< ex << endl;
        }
        catch(...){}
    }
}
// ------------------------------------------------------------------------------------------
void IOController::activateInit()
{
    // Разрегистрируем аналоговые датчики
    for( IOStateList::iterator li = ioList.begin(); li != ioList.end(); ++li )
    {
        try
        {
            USensorInfo& s(li->second);

            // Проверка зависимостей
            if( s.d_si.id != DefaultObjectId )
            {
                IOStateList::iterator d_it = myiofind( UniSetTypes::key(s.d_si) );
                if( d_it != ioEnd() )
                    s.checkDepend(d_it, this);
            }

            sigInit.emit(li,this);
        }
        catch( Exception& ex )
        {
            ucrit << myname << "(activateInit): "<< ex << endl;
        }
        catch(...){}
    }
}
// ------------------------------------------------------------------------------------------
CORBA::Long IOController::getValue( UniSetTypes::ObjectId sid )
{
    IOStateList::iterator li(ioList.end());
    return localGetValue(li,sid);
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( IOController::IOStateList::iterator& li, const UniSetTypes::ObjectId sid )
{
    if( li == ioList.end() )
        li = ioList.find(sid);

    if( li!=ioList.end() )
    {
        if( li->second.undefined )
            throw IOController_i::Undefined();

        uniset_rwmutex_rlock lock(li->second.val_lock);
        return li->second.value;
    }

    // -------------
    ostringstream err;
    err << myname << "(localGetValue): Not found sensor (" << sid << ") "
        << conf->oind->getNameById(sid);

    uinfo << err.str() << endl;
    throw IOController_i::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
void IOController::setUndefinedState(UniSetTypes::ObjectId sid, CORBA::Boolean undefined, UniSetTypes::ObjectId sup_id )
{
    IOController::IOStateList::iterator li(ioList.end());
    localSetUndefinedState( li,undefined, sid );
}
// -----------------------------------------------------------------------------
void IOController::localSetUndefinedState( IOStateList::iterator& li, 
                                            bool undefined, const UniSetTypes::ObjectId sid )
{
    // сохранение текущего состояния
    if( li == ioList.end() )
        li = ioList.find(sid);

    if( li==ioList.end() )
    {
        ostringstream err;
        err << myname << "(localSetUndefined): Unknown sensor (" << sid << ")"
            << "name: " << conf->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    bool changed = false;
    {    // lock
        uniset_rwmutex_wrlock lock(li->second.val_lock);
        changed = (li->second.undefined != undefined);
        li->second.undefined = undefined;
    }    // unlock

    // сперва локальные события...
    try
    {
        if( changed )
        {
            uniset_rwmutex_wrlock l(li->second.undefMutex);
            li->second.sigUndefChange.emit(li, this);
        }
    }
    catch(...){}

    // потом глобольное, но конкретно для 'undefchange'
    try
    {
        if( changed )
        {
            uniset_mutex_lock l(siganyundefMutex);
            sigAnyUndefChange.emit(li, this);
        }
    }
    catch(...){}

    // теперь просто событие по изменению состояния
    try
    {
        if( changed )
        {
            uniset_rwmutex_wrlock(li->second.changeMutex);
            li->second.sigChange.emit(li, this);
        }
    }
    catch(...){}

    // глобальное по всем..
    try
    {
        if( changed )
        {
            uniset_mutex_lock l(siganyMutex);
            sigAnyChange.emit(li, this);
        }
    }
    catch(...){}
}
// ------------------------------------------------------------------------------------------
void IOController::fastSetValue( UniSetTypes::ObjectId sid, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
    try
    {
        IOController::IOStateList::iterator li(ioList.end());
        localSetValue( li, sid, value, sup_id );
    }
    catch(...){}
}
// ------------------------------------------------------------------------------------------
void IOController::setValue( UniSetTypes::ObjectId sid, CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
    IOController::IOStateList::iterator li(ioList.end());
    localSetValue( li, sid, value, sup_id );
}
// ------------------------------------------------------------------------------------------
void IOController::localSetValue( IOController::IOStateList::iterator& li,
                                  UniSetTypes::ObjectId sid,
                                  CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
    if( sup_id == UniSetTypes::DefaultObjectId )
        sup_id = getId();

    // сохранение текущего состояния
    if( li == ioList.end() )
        li = ioList.find(sid);

    if( li==ioList.end() )
    { 
        ostringstream err;
        err << myname << "(localSaveValue): Unknown sensor (" << sid << ")"
            << "name: " << conf->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    bool changed = false;

    {    // lock
        uniset_rwmutex_wrlock lock(li->second.val_lock);

        // фильтрам может потребоваться измениять исходное значение (например для усреднения)
        // поэтому передаём (и затем сохраняем) напрямую(ссылку) value (а не const value)
        bool blocked = ( li->second.blocked || li->second.undefined );

        if( checkIOFilters(&li->second,value,sup_id) || blocked )
        {
            uinfo << myname << ": save sensor value (" << sid << ")"
                    << " name: " << conf->oind->getNameById(sid)
                    << " value="<< value << endl;

            long prev = li->second.value;

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
            tm.tv_sec  = 0;
            tm.tv_usec = 0;
            gettimeofday(&tm,&tz);
            li->second.tv_sec  = tm.tv_sec;
            li->second.tv_usec = tm.tv_usec;
        }
    }    // unlock

    try
    {
        if( changed )
        {
            uniset_rwmutex_wrlock l(li->second.changeMutex);
            li->second.sigChange.emit(li, this);
        }
    }
    catch(...){}

    try
    {
        if( changed )
        {
            uniset_mutex_lock l(siganyMutex);
            sigAnyChange.emit(li, this);
        }
    }
    catch(...){}
}
// ------------------------------------------------------------------------------------------
IOType IOController::getIOType( UniSetTypes::ObjectId sid )
{
    IOStateList::iterator ali = ioList.find(sid);
    if( ali!=ioList.end() )
        return ali->second.type;

    ostringstream err;
    err << myname << "(getIOType): датчик имя: " << conf->oind->getNameById(sid) << " не найден";
    throw IOController_i::NameNotFound(err.str().c_str());
}
// ---------------------------------------------------------------------------
void IOController::ioRegistration( const USensorInfo& ainf, bool force )
{
    // проверка задан ли контроллеру идентификатор
    if( getId() == DefaultObjectId )
    {
        ostringstream err;
        err << "(IOCOntroller::ioRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
        uwarn << err.str() << endl;
        throw ResolveNameError(err.str().c_str());
    }

    {    // lock
        uniset_rwmutex_wrlock lock(ioMutex);
        if( !force )
        {
            IOStateList::iterator li = ioList.find(ainf.si.id);
            if( li!=ioList.end() )
            {
                ostringstream err;
                err << "Попытка повторной регистрации датчика("<< ainf.si.id << "). имя: "
                    << conf->oind->getNameById(ainf.si.id);
                throw ObjectNameAlready(err.str().c_str());
            }
        }

        IOStateList::mapped_type ai(ainf);
        // запоминаем начальное время
        struct timeval tm;
        struct timezone tz;
        tm.tv_sec   = 0;
        tm.tv_usec  = 0;
        gettimeofday(&tm,&tz);
        ai.tv_sec   = tm.tv_sec;
        ai.tv_usec  = tm.tv_usec;
        ai.value    = ai.default_val;

        // более оптимальный способ(при условии вставки первый раз)
        ioList.insert(IOStateList::value_type(ainf.si.id,ai));
    }

    try
    {
        for( unsigned int i=0; i<2; i++ )
        {
            try
            {
                uinfo << myname 
                        << "(ioRegistration): регистрирую " 
                        << conf->oind->getNameById(ainf.si.id) << endl;

                ui.registered( ainf.si.id, getRef(), true );
                return;
            }
            catch(ObjectNameAlready& ex )
            {
                uwarn << myname << "(asRegistration): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
                ui.unregister(ainf.si.id);
            }
        }
    }
    catch( Exception& ex )
    {
        ucrit << myname << "(ioRegistration): " << ex << endl;
    }
    catch(...)
    {
        ucrit << myname << "(ioRegistration): catch ..."<< endl;
    }
}
// ---------------------------------------------------------------------------
void IOController::ioUnRegistration( const UniSetTypes::ObjectId sid )
{
    ui.unregister(sid);
}
// ---------------------------------------------------------------------------
void IOController::logging( UniSetTypes::SensorMessage& sm )
{
    uniset_rwmutex_wrlock l(loggingMutex);
    try
    {
//        struct timezone tz;
//        gettimeofday(&sm.tm,&tz);
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
            ucrit << myname << "(logging): DBServer unavailable" << endl;
        }
    }
}
// --------------------------------------------------------------------------------------------------------------
void IOController::dumpToDB()
{
    // значит на этом узле нет DBServer-а
    if( conf->getDBServer() == UniSetTypes::DefaultObjectId )
        return;

    {    // lock
//        uniset_mutex_lock lock(ioMutex, 100);
        for( IOStateList::iterator li = ioList.begin(); li!=ioList.end(); ++li ) 
        {
            uniset_rwmutex_rlock lock(li->second.val_lock);
            SensorMessage sm;
            sm.id           = li->second.si.id;
            sm.node         = li->second.si.node;
            sm.sensor_type  = li->second.type;
            sm.value        = li->second.value;
            sm.undefined    = li->second.undefined;
            sm.priority     = (Message::Priority)li->second.priority;
            sm.sm_tv_sec    = li->second.tv_sec;
            sm.sm_tv_usec   = li->second.tv_usec;
            sm.ci           = li->second.ci;
            if ( !li->second.dbignore )
                logging(sm);
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

    unsigned int i=0;
    for( IOStateList::iterator it=ioList.begin(); it!=ioList.end(); ++it)
    {    
        uniset_rwmutex_rlock lock(it->second.val_lock);
        (*res)[i] = it->second;
        i++;
    }

    return res;
}
// --------------------------------------------------------------------------------------------------------------
UniSetTypes::Message::Priority IOController::getPriority( const UniSetTypes::ObjectId sid )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it!=ioList.end() )
        return (UniSetTypes::Message::Priority)it->second.priority;

    return UniSetTypes::Message::Medium; // ??
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::SensorIOInfo IOController::getSensorIOInfo( const UniSetTypes::ObjectId sid )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it!=ioList.end() )
    {
        uniset_rwmutex_rlock lock(it->second.val_lock);
        return it->second;
    }

    // -------------
    ostringstream err;
    err << myname << "(getSensorIOInfo): Unknown sensor (" << sid << ")"
        << conf->oind->getNameById(sid);

    uinfo << err.str() << endl;

    throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
CORBA::Long IOController::getRawValue( UniSetTypes::ObjectId sid )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it==ioList.end() )
    {
        ostringstream err;
        err << myname << "(getRawValue): Unknown analog sensor (" << sid << ")"
            << conf->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    // ??? получаем raw из калиброванного значения ???
    IOController_i::CalibrateInfo& ci(it->second.ci);

    if( ci.maxCal!=0 && ci.maxCal!=ci.minCal )
    {
        if( it->second.type == UniversalIO::AI )
            return UniSetTypes::lcalibrate(it->second.value,ci.minRaw,ci.maxRaw,ci.minCal,ci.maxCal,true);

        if( it->second.type == UniversalIO::AO ) 
            return UniSetTypes::lcalibrate(it->second.value,ci.minCal,ci.maxCal,ci.minRaw,ci.maxRaw,true);
    }

    return it->second.value;
}
// --------------------------------------------------------------------------------------------------------------
void IOController::calibrate( UniSetTypes::ObjectId sid,
                                const IOController_i::CalibrateInfo& ci,
                                UniSetTypes::ObjectId adminId )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it==ioList.end() )
    {
        ostringstream err;
        err << myname << "(calibrate): Unknown analog sensor (" << sid << ")"
            << conf->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    uinfo << myname <<"(calibrate): from " << conf->oind->getNameById(adminId) << endl;

    it->second.ci = ci;
}
// --------------------------------------------------------------------------------------------------------------
IOController_i::CalibrateInfo IOController::getCalibrateInfo( UniSetTypes::ObjectId sid )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it==ioList.end() )
    {
        ostringstream err;
        err << myname << "(calibrate): Unknown analog sensor (" << sid << ")"
            << conf->oind->getNameById(sid);
        throw IOController_i::NameNotFound(err.str().c_str());
    }
    return it->second.ci;
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

// ----------------------------------------------------------------------------------------
bool IOController::checkIOFilters( const USensorInfo& ai, CORBA::Long& newvalue, 
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

    for( unsigned int i=0; i<size; i++ )
    {
        IOStateList::iterator it = ioList.find( UniSetTypes::key(lst[i],conf->getLocalNode()) );
        if( it!=ioList.end() )
        {
            uniset_rwmutex_rlock lock(it->second.val_lock);
            (*res)[i] = it->second;
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
    UniSetTypes::IDList badlist; // писок не найденных идентификаторов

    int size = lst.length();

    for(int i=0; i<size; i++)
    {
        ObjectId sid = lst[i].si.id;

        {
            IOStateList::iterator it = ioList.find(sid);
            if( it!=ioList.end() )
            {
                localSetValue(it,sid,lst[i].value, sup_id);
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
    IOStateList::iterator ait = ioList.find(sid);
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
        << conf->oind->getNameById(sid) << " не найден";

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

    int i=0;
    for( IOStateList::iterator it=ioList.begin(); it!=ioList.end(); ++it)
    {
        IOController_i::ShortMap m;
        {
            uniset_rwmutex_rlock lock(it->second.val_lock);
            m.id     = it->second.si.id;
            m.value = it->second.value;
            m.type = it->second.type;
        }
        (*res)[i++] = m;
    }

    return res;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value( UniSetTypes::ObjectId sid )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it==ioList.end() )
    {
        ostringstream err;
        err << myname << "(signal_change_value): вход(выход) с именем " 
            << conf->oind->getNameById(sid) << " не найден";

        uinfo << err.str() << endl;
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    uniset_rwmutex_rlock lock(it->second.val_lock);
    return it->second.sigChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value()
{
    return sigAnyChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeUndefinedStateSignal IOController::signal_change_undefined_state( UniSetTypes::ObjectId sid )
{
    IOStateList::iterator it = ioList.find(sid);
    if( it==ioList.end() )
    {
        ostringstream err;
        err << myname << "(signal_change_undefine): вход(выход) с именем " 
            << conf->oind->getNameById(sid) << " не найден";

        uinfo << err.str() << endl;

        throw IOController_i::NameNotFound(err.str().c_str());
    }

    uniset_rwmutex_rlock lock(it->second.val_lock);
    return it->second.sigUndefChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeUndefinedStateSignal IOController::signal_change_undefined_state()
{
    return sigAnyUndefChange;
}
// -----------------------------------------------------------------------------
void IOController::USensorInfo::checkDepend( IOStateList::iterator& d_it, IOController* ic )
{
    bool changed = false;
    {
        uniset_rwmutex_wrlock lock(val_lock);
        bool prev = blocked;
        uniset_rwmutex_rlock dlock(d_it->second.val_lock);
        blocked = ( d_it->second.value == d_value ) ? false : true;
        changed = ( prev != blocked );
    }

    if( changed )
        ic->localSetValue( it, si.id, real_value, ic->getId() );
}
// -----------------------------------------------------------------------------
