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

#include "UInterface.h"
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
askIOMutex("askIOMutex"),
trshMutex("trshMutex"),
maxAttemtps(conf->getPIntField("ConsumerMaxAttempts", 5))
{

}

IONotifyController::IONotifyController(const string& name, const string& section, NCRestorer* d ): 
    IOController(name, section),
    restorer(d),
    askIOMutex(name+"askIOMutex"),
    trshMutex(name+"trshMutex"),
    maxAttemtps(conf->getPIntField("ConsumerMaxAttempts", 5))
{
    // добавляем фильтры
    addIOFilter( sigc::mem_fun(this,&IONotifyController::myIOFilter) );
}

IONotifyController::IONotifyController( ObjectId id, NCRestorer* d ):
    IOController(id),
    restorer(d),
    askIOMutex(string(conf->oind->getMapName(id))+"_askIOMutex"),
    trshMutex(string(conf->oind->getMapName(id))+"_trshMutex"),
    maxAttemtps(conf->getPIntField("ConsumerMaxAttempts", 5))
{
    signal_change_undefined_state().connect(sigc::mem_fun(*this, &IONotifyController::onChangeUndefinedState));
    signal_init().connect(sigc::mem_fun(*this, &IONotifyController::initItem));

    // добавляем фильтры
    addIOFilter( sigc::mem_fun(this,&IONotifyController::myIOFilter) );
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
 *    \param lst - указатель на список в который необходимо внести потребителя
 *    \param name - имя вносимого потребителя
 *    \note Добавление произойдет только если такого потребителя не существует в списке
*/
bool IONotifyController::addConsumer( ConsumerListInfo& lst, const ConsumerInfo& ci )
{
    uniset_rwmutex_wrlock l(lst.mut);

    for( auto it=lst.clst.begin(); it!=lst.clst.end(); ++it )
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

    lst.clst.push_front(cinf);
    return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *    \param lst - указатель на список из которго происходит удаление потребителя
 *    \param name - имя удаляемого потребителя
*/
bool IONotifyController::removeConsumer( ConsumerListInfo& lst, const ConsumerInfo& cons )
{
    uniset_rwmutex_wrlock l(lst.mut);
    for( auto li=lst.clst.begin();li!=lst.clst.end(); ++li )
    {
        if( li->id==cons.id && li->node==cons.node  )
        {
            lst.clst.erase(li);
            return true;
        }
    }

    return false;
}
// ------------------------------------------------------------------------------------------
/*! 
 *    \param si         - информация о датчике
 *    \param ci         - информация о заказчике
 *    \param cmd         - команда см. UniversalIO::UIOCommand
*/
void IONotifyController::askSensor(const UniSetTypes::ObjectId sid,
                                    const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd )
{
    uinfo << "(askSensor): поступил " << ( cmd == UIODontNotify ? "отказ" :"заказ" ) << " от "
            << conf->oind->getNameById(ci.id) << "@" << ci.node
            << " на аналоговый датчик "
            << conf->oind->getNameById(sid) << endl;

    // если такого аналогового датчика нет, здесь сработает исключение...
    auto li = myioEnd();
    localGetValue(li,sid);

    {    // lock
        uniset_rwmutex_wrlock lock(askIOMutex);
        // а раз есть заносим(исключаем) заказчика 
        ask(askIOList, sid, ci, cmd);
    }    // unlock

    // посылка первый раз состояния 
    if( cmd==UniversalIO::UIONotify || (cmd==UIONotifyFirstNotNull && li->second.value) )
    {
        SensorMessage  smsg;
        smsg.id          = sid;
        smsg.node        = conf->getLocalNode();
        smsg.consumer    = ci.id;
        smsg.supplier    = getId();
        smsg.sensor_type = li->second.type;    
        smsg.priority    = (Message::Priority)li->second.priority;
        smsg.sm_tv_sec   = li->second.tv_sec;
        smsg.sm_tv_usec  = li->second.tv_usec;
        smsg.ci          = li->second.ci;
        {
            uniset_rwmutex_rlock lock(li->second.val_lock);
            smsg.value        = li->second.value;
            smsg.undefined    = li->second.undefined;
            smsg.sm_tv_sec    = li->second.tv_sec;
            smsg.sm_tv_usec   = li->second.tv_usec;
        }

        TransportMessage tm(smsg.transport_msg());
        try
        {
            ui.send(ci.id, tm, ci.node);
        }
        catch( Exception& ex )
        {
            uwarn << myname << "(askSensor): " <<  conf->oind->getNameById(sid) << " error: "<< ex << endl;
        }
        catch( CORBA::SystemException& ex )
        {
            uwarn << myname << "(askSensor): " << conf->oind->getNameById(ci.id) << "@" << ci.node 
                  << " недоступен!!(CORBA::SystemException): "
                  << ex.NP_minorString() << endl;
        }
        catch(...)
        {
            uwarn << myname << "(askSensor): " << conf->oind->getNameById(ci.id) << "@" << ci.node
                  << " catch..." << endl;
        }
    }
}

// ------------------------------------------------------------------------------------------
void IONotifyController::ask( AskMap& askLst, const UniSetTypes::ObjectId sid, 
                                const UniSetTypes::ConsumerInfo& cons, UniversalIO::UIOCommand cmd)
{
    // поиск датчика в списке 
    auto askIterator = askLst.find(sid);

    switch (cmd)
    {
        case UniversalIO::UIONotify: // заказ
        case UniversalIO::UIONotifyChange:
        case UniversalIO::UIONotifyFirstNotNull:
        {
            if( askIterator==askLst.end() )
            {
                ConsumerListInfo lst; // создаем новый список
                addConsumer(lst,cons);
                // более оптимальный способ(при условии вставки первый раз)
                askLst.insert(AskMap::value_type(sid,lst));

                try
                {
                    dumpOrdersList(sid,lst);
                }
                catch(Exception& ex)
                {
                    uwarn << myname << " не смогли сделать dump: " << ex << endl;
                }
                catch(...)
                {
                    uwarn << myname << " не смогли сделать dump (catch...)" << endl;
                }
            }
            else
            {
                if( addConsumer(askIterator->second,cons) )
                {
                    try
                    {
                        dumpOrdersList(sid,askIterator->second);
                    }
                    catch(Exception& ex)
                    {
                        uwarn << myname << " не смогли сделать dump: " << ex << endl;
                    }
                    catch(...)
                    {
                        uwarn << myname << " не смогли сделать dump (catch...)" << endl;
                    }
                }
            }
            break;
        }
        case UniversalIO::UIODontNotify:     // отказ
        {
            if( askIterator!=askLst.end() )    // существует
            {
                if( removeConsumer(askIterator->second, cons) )
                {
                    uniset_rwmutex_wrlock l(askIterator->second.mut);
                    if( askIterator->second.clst.empty() )
                    {
//                         не удаляем, т.к. могут поломаться итераторы 
//                         используемые в это время в других потоках..
//                         askLst.erase(askIterator);
                    }
                    else
                    {
                        try
                        {
                            dumpOrdersList(sid,askIterator->second);
                        }
                        catch(Exception& ex)
                        {
                            uwarn << myname << " не смогли сделать dump: " << ex << endl;
                        }
                        catch(...)
                        {
                            uwarn << myname << " не смогли сделать dump (catch...)" << endl;
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
bool IONotifyController::myIOFilter( const USensorInfo& ai, 
                                    CORBA::Long newvalue, UniSetTypes::ObjectId sup_id )
{
    if( ai.value == newvalue )
        return false;

    return true;
}
// ------------------------------------------------------------------------------------------
void IONotifyController::localSetValue( IOController::IOStateList::iterator& li,
                                         UniSetTypes::ObjectId sid,
                                         CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
    // Если датчик не найден сдесь сработает исключение
    long prevValue = 0;

    try
    {
          prevValue = IOController::localGetValue( li, sid );
    }
    catch( IOController_i::Undefined )
    {
         // чтобы сработало prevValue!=value 
         // искусственно меняем значение
         prevValue = value+1;
    }


    if( li == myioEnd() ) // ???
    {
        ostringstream err;
        err << myname << "(localSetValue):  вход(выход) с именем " 
        << conf->oind->getNameById(sid) << " не найден";

        uinfo << err.str() << endl;
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    IOController::localSetValue(li, sid, value, sup_id);

    // сравниваем именно с li->second.value
    // т.к. фактическое сохранённое значение может быть изменено
    // фильтрами или блокировками..
    SensorMessage sm(sid,li->second.value);
    { // lock
        uniset_rwmutex_rlock lock(li->second.val_lock);

        if( prevValue == li->second.value )
            return;

        // Рассылаем уведомления только в слуае изменения значения
        sm.id           = sid;
        sm.node         = conf->getLocalNode();
        sm.value        = li->second.value;
        sm.undefined    = li->second.undefined;
        sm.priority     = (Message::Priority)li->second.priority;
        sm.supplier     = sup_id;
        sm.sensor_type  = li->second.type;
        sm.sm_tv_sec    = li->second.tv_sec;
        sm.sm_tv_usec   = li->second.tv_usec;
        sm.ci           = li->second.ci;
    } // unlock

    try
    {
        if( !li->second.dbignore )
            loggingInfo(sm);
    }
    catch(...){}

    {
        uniset_rwmutex_rlock lock(askIOMutex);
        auto it = askIOList.find(sid);
        if( it!=askIOList.end() )
            send(it->second, sm);
    }

    // проверка порогов
    try
    {
        checkThreshold(li,sid,true);
    }
    catch(...){}
}
// ------------------------------------------------------------------------------------------
/*!
    \note В случае зависания в функции push, будут остановлены рассылки другим объектам.
    Возможно нужно ввести своего агента на удалённой стороне, который будет заниматься
    только приёмом сообщений и локальной рассылкой. Lav
*/
void IONotifyController::send( ConsumerListInfo& lst, UniSetTypes::SensorMessage& sm )
{
    TransportMessage tmsg;
    uniset_rwmutex_wrlock l(lst.mut);

    for( auto li=lst.clst.begin(); li!=lst.clst.end(); ++li )
    {
        for( int i=0; i<2; i++ )    // на каждый объект по две поптыки
        {
            try
            {
                if( CORBA::is_nil(li->ref) )
                {
                    CORBA::Object_var op = ui.resolve(li->id, li->node);
                    li->ref = UniSetObject_i::_narrow(op);
                }

                sm.consumer = li->id;

                // Для оптимизации, чтобы избежать лишее копирование и создание TransportMessage
                // не используем sm.transport_msg()
                // а формируем TransportMessage самостоятельно..

                assert(sizeof(UniSetTypes::RawDataOfTransportMessage)>=sizeof(sm));
                memcpy(&tmsg.data,&sm,sizeof(sm));
                li->ref->push( tmsg );

                li->attempt = maxAttemtps; // reinit attempts
                break;
            }
            catch(Exception& ex)
            {
                uwarn << myname << "(IONotifyController::send): " << ex
                        << " for " << conf->oind->getNameById(li->id) << "@" << li->node << endl;
            }
            catch( CORBA::SystemException& ex )
            {
                uwarn << myname << "(IONotifyController::send): " 
                        << conf->oind->getNameById(li->id) << "@" << li->node << " (CORBA::SystemException): "
                        << ex.NP_minorString() << endl;
            }
            catch(...)
            {
                ucrit << myname << "(IONotifyController::send): "
                        << conf->oind->getNameById(li->id) << "@" << li->node
                        << " catch..." << endl;
            }

            if( maxAttemtps>0 && ( (li->attempt)-- <= 0 ) )
            {
                li = lst.clst.erase(li);
                if( li == lst.clst.end() ) --li;
                break;
            }

            li->ref = UniSetObject_i::_nil();
        }
    }
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::loggingInfo( UniSetTypes::SensorMessage& sm )
{
    IOController::logging(sm);
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::activateObject()
{
    // сперва вычитаем датчиков и заказчиков..
    readDump();
    // а потом уже собственно активация..
    return IOController::activateObject();
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
        uwarn << myname << "(IONotifyController::readDump): " << ex << endl;
    }
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::initItem( IOStateList::iterator& li, IOController* ic )
{
    USensorInfo& s(li->second);

    if( s.type == UniversalIO::AI || s.type == UniversalIO::AO )
        checkThreshold( li, s.si.id, false );
}
// ------------------------------------------------------------------------------------------
void IONotifyController::dumpOrdersList( const UniSetTypes::ObjectId sid,
                                         const IONotifyController::ConsumerListInfo& lst)
{
    if( restorer == NULL )
        return;

    try
    {
        NCRestorer::SInfo inf;
        IOController_i::SensorIOInfo ainf( getSensorIOInfo(sid) );
        inf=ainf;
        restorer->dump(this,inf,lst);
    }
    catch( Exception& ex )
    {
        uwarn << myname << "(IONotifyController::dumpOrderList): " << ex << endl;
    }
}
// --------------------------------------------------------------------------------------------------------------

void IONotifyController::dumpThresholdList( const UniSetTypes::ObjectId sid, const IONotifyController::ThresholdExtList& lst)
{
    if( restorer == NULL )
        return;

    try
    {
        NCRestorer::SInfo inf;
        IOController_i::SensorIOInfo ainf(getSensorIOInfo(sid));
        inf=ainf;
        restorer->dumpThreshold(this,inf,lst);
    }
    catch(Exception& ex)
    { 
        uwarn << myname << "(IONotifyController::dumpThresholdList): " << ex << endl;
    }
}
// --------------------------------------------------------------------------------------------------------------

void IONotifyController::askThreshold(UniSetTypes::ObjectId sid, const UniSetTypes::ConsumerInfo& ci, 
                                    UniSetTypes::ThresholdId tid,
                                    CORBA::Long lowLimit, CORBA::Long hiLimit,  CORBA::Boolean invert, 
                                    UniversalIO::UIOCommand cmd )
{
    if( lowLimit > hiLimit )
        throw IONotifyController_i::BadRange();

    // если такого дискретного датчика нет сдесь сработает исключение...
    auto li = myioEnd();
    CORBA::Long val = localGetValue(li,sid);

    {    // lock
        uniset_rwmutex_wrlock lock(trshMutex);

        // поиск датчика в списке 
        auto it = askTMap.find(sid);

        ThresholdInfoExt ti(tid,lowLimit,hiLimit,invert);
        ti.sit = myioEnd();

        switch( cmd )
        {
            case UniversalIO::UIONotify: // заказ
            case UniversalIO::UIONotifyChange:
            {
                if( it==askTMap.end() ) 
                {
                    ThresholdExtList lst;    // создаем новый список
                    ThresholdsListInfo tli;
                    tli.si.id   = sid;
                    tli.si.node = conf->getLocalNode();
                    tli.list   = lst;
                    tli.type   = li->second.type;
                    tli.ait    = myioEnd();
                    addThreshold(lst,ti,ci);
                    askTMap.insert(AskThresholdMap::value_type(sid,tli));
                    try
                    {
                        dumpThresholdList(sid,lst);
                    }
                    catch(Exception& ex)
                    {
                        uwarn << myname << " не смогли сделать dump: " << ex << endl;
                    }
                    catch(...)
                    {
                        uwarn << myname << " не смогли сделать dump" << endl;
                    }
                }
                else
                {
                    if( addThreshold(it->second.list,ti,ci) )
                    {
                        try
                        {
                            dumpThresholdList(sid,it->second.list);
                        }
                        catch(Exception& ex)
                        {
                            uwarn << myname << "(askThreshold): dump: " << ex << endl;
                        }
                        catch(...)
                        {
                            uwarn << myname << "(askThreshold): dump catch..." << endl;
                        }
                    }
                }

                if( cmd == UniversalIO::UIONotifyChange )
                    break;

                // посылка первый раз состояния 
                try
                {
                    SensorMessage sm;
                    sm.id         = sid;
                    sm.node       = conf->getLocalNode();
                    sm.value      = val;
                    sm.undefined  = li->second.undefined;
                    sm.sensor_type  = li->second.type;
                    sm.priority   = (Message::Priority)li->second.priority;
                    sm.consumer   = ci.id;
                    sm.tid        = tid;
                    sm.sm_tv_sec  = ti.tv_sec;
                    sm.sm_tv_usec = ti.tv_usec;
                    sm.ci         = li->second.ci;

                    // Проверка нижнего предела
                    if( val <= lowLimit )
                    {
                        sm.threshold = false;
                        CORBA::Object_var op = ui.resolve(ci.id, ci.node);
                        UniSetObject_i_var ref = UniSetObject_i::_narrow(op);
                        if(!CORBA::is_nil(ref))
                            ref->push(sm.transport_msg());
                    }
                    // Проверка верхнего предела
                    else if( val >= hiLimit )
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
                    uwarn << myname << "(askThreshod): " << ex << endl;
                }
                catch( CORBA::SystemException& ex )
                {
                    uwarn << myname << "(askThreshod): CORBA::SystemException: "
                            << ex.NP_minorString() << endl;
                }
                catch(...)
                {
                    uwarn << myname << "(askThreshold): dump catch..." << endl;
                }
            }
            break;

            case UniversalIO::UIODontNotify:     // отказ
            {
                if( it!=askTMap.end() )
                {
                    if( removeThreshold(it->second.list,ti,ci) )
                    {
                        try
                        {
                            dumpThresholdList(sid,it->second.list);
                        }
                        catch(Exception& ex)
                        {
                            uwarn << myname << "(askThreshold): dump: " << ex << endl;
                        }
                        catch(...)
                        {
                            uwarn << myname << "(askThreshold): dump catch..." << endl;
                        }
                    }
                }
            }
            break;

            default:
                break;
        }
    }    // unlock
} 
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::addThreshold( ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& ci )
{
    for( auto it=lst.begin(); it!=lst.end(); ++it) 
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
    tm.tv_sec = 0; 
    tm.tv_usec = 0;
    gettimeofday(&tm,&tz);
    ti.tv_sec  = tm.tv_sec;
    ti.tv_usec = tm.tv_usec;

    lst.push_front(ti);
    return true;
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::removeThreshold( ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& ci )
{
    for( auto it=lst.begin(); it!=lst.end(); ++it) 
    {
        if( ti == (*it) )
        {
            if( removeConsumer(it->clst, ci) )
            {
/*                  Не удаляем датчик из списка, чтобы не поломать итераторы
                  которые могут использоваться в этот момент в других потоках*/


//                uniset_rwmutex_wrlock lock(it->clst.mut);
//                if( it->clst.clst.empty() )
//                    lst.erase(it);
                return true;
            }
        }
    }

    return false;
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::checkThreshold( IOStateList::iterator& li,
                                         const UniSetTypes::ObjectId sid,
                                         bool send_msg )
{
        // поиск списка порогов
        auto lst = askTMap.end();

        {
             uniset_rwmutex_rlock lock(trshMutex);
             lst = askTMap.find(sid);
             if( lst == askTMap.end() )
                 return;

             if( lst->second.list.empty() )
                 return;
       }

        if( li == myioEnd() )
            li = myiofind(sid);

        if( li == myioEnd() )
            return; // ???

        SensorMessage sm;
        sm.id           = sid;
        sm.node         = conf->getLocalNode();
        sm.sensor_type  = li->second.type;
        sm.priority     = (Message::Priority)li->second.priority;
        sm.ci           = li->second.ci;
        {
            uniset_rwmutex_rlock lock(li->second.val_lock);
            sm.value      = li->second.value;
            sm.undefined  = li->second.undefined;
            sm.sm_tv_sec  = li->second.tv_sec;
            sm.sm_tv_usec = li->second.tv_usec;
        }

        // текущее время
        struct timeval tm;
        struct timezone tz;
        tm.tv_sec = 0; tm.tv_usec = 0;
        gettimeofday(&tm,&tz);

     {
        uniset_rwmutex_rlock l(lst->second.mut);
        for( auto it=lst->second.list.begin(); it!=lst->second.list.end(); ++it ) 
        {
            // Используем здесь sm.value чтобы не делать ещё раз lock на li->second.value

            IONotifyController_i::ThresholdState state = it->state;

            if( !it->invert )
            {
                // Если логика не инвертированная, то срабатывание это - выход за зону >= hilimit
                if( sm.value <= it->lowlimit  )
                    state = IONotifyController_i::NormalThreshold;
                else if( sm.value >= it->hilimit )
                    state = IONotifyController_i::HiThreshold;
            }
            else
            {
                // Если логика инвертированная, то срабатывание это - выход за зону <= lowlimit
                if( sm.value >= it->hilimit  )
                    state = IONotifyController_i::NormalThreshold;
                else if( sm.value <= it->lowlimit )
                    state = IONotifyController_i::LowThreshold;
            }

            // если ничего не менялось..
            if( it->state == state )
                continue;

            it->state = state;

            sm.tid = it->id;

            // если состояние не normal, значит порог сработал,
            // не важно какой.. нижний или верхний (зависит от inverse)
            sm.threshold = ( state != IONotifyController_i::NormalThreshold ) ? true : false;

            // запоминаем время изменения состояния
            it->tv_sec     = tm.tv_sec;
            it->tv_usec    = tm.tv_usec;
            sm.sm_tv_sec   = tm.tv_sec;
            sm.sm_tv_usec  = tm.tv_usec;

            // если порог связан с датчиком, то надо его выставить
            if( it->sid != UniSetTypes::DefaultObjectId )
            {
                try
                {
                     localSetValue(it->sit,it->sid,(sm.threshold ? 1:0),getId());
                }
                catch( UniSetTypes::Exception& ex )
                {
                    ucrit << myname << "(checkThreshold): " << ex << endl;
                }
            }

            // отдельно посылаем сообщения заказчикам по данному "порогу"
            if( send_msg )
                send(it->clst, sm);
        }
     }
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController::ThresholdExtList::iterator IONotifyController::findThreshold( const UniSetTypes::ObjectId sid, const UniSetTypes::ThresholdId tid )
{
    {    // lock
        uniset_rwmutex_rlock lock(trshMutex);
        // поиск списка порогов
        auto lst = askTMap.find(sid);

        if( lst!=askTMap.end() )
        {
            for( auto it=lst->second.list.begin(); it!=lst->second.list.end(); ++it) 
            {
                if( it->id == tid )
                    return it;
            }
        }
    }

    return ThresholdExtList::iterator();
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdInfo IONotifyController::getThresholdInfo( UniSetTypes::ObjectId sid, UniSetTypes::ThresholdId tid )
{
    uniset_rwmutex_rlock lock(trshMutex);

    auto it = askTMap.find(sid);
    if( it == askTMap.end() )
    {
        ostringstream err;
        err << myname << "(getThresholds): Not found sensor (" << sid << ") " 
            << conf->oind->getNameById(sid);

        uinfo << err.str() << endl;
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    for( auto it2= it->second.list.begin(); it2!=it->second.list.end(); ++it2 )
    {
        if( it2->id == tid )
            return IONotifyController_i::ThresholdInfo( *it2 );
    }

    ostringstream err;
    err << myname << "(getThresholds): Not found for sensor (" << sid << ") " 
        << conf->oind->getNameById(sid) << " ThresholdID='" << tid << "'";

    uinfo << err.str() << endl;
    throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdList* IONotifyController::getThresholds( UniSetTypes::ObjectId sid )
{
    uniset_rwmutex_rlock lock(trshMutex);

    auto it = askTMap.find(sid);
    if( it == askTMap.end() )
    {
        ostringstream err;
        err << myname << "(getThresholds): Not found sensor (" << sid << ") " 
            << conf->oind->getNameById(sid);

        uinfo << err.str() << endl;
        throw IOController_i::NameNotFound(err.str().c_str());
    }

    IONotifyController_i::ThresholdList* res = new IONotifyController_i::ThresholdList();

    try
    {
        res->si     = it->second.si;
        res->value  = IOController::localGetValue(it->second.ait,it->second.si.id);
        res->type   = it->second.type;
    }
    catch( Exception& ex )
    {
        uwarn << myname << "(getThresholdsList): для датчика " 
                << conf->oind->getNameById(it->second.si.id)
                << " " << ex << endl;
    }

    res->tlist.length( it->second.list.size() );

    unsigned int k=0;
    for( auto it2= it->second.list.begin(); it2!=it->second.list.end(); ++it2 )
    {
        res->tlist[k].id       = it2->id;
        res->tlist[k].hilimit  = it2->hilimit;
        res->tlist[k].lowlimit = it2->lowlimit;
        res->tlist[k].state    = it2->state;
        res->tlist[k].tv_sec   = it2->tv_sec;
        res->tlist[k].tv_usec  = it2->tv_usec;
        k++;
    }

    return res;
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdsListSeq* IONotifyController::getThresholdsList()
{
    IONotifyController_i::ThresholdsListSeq* res = new IONotifyController_i::ThresholdsListSeq();

    res->length( askTMap.size() );

    uniset_rwmutex_rlock lock(trshMutex);
    if( !askTMap.empty() )
    {
        unsigned int i=0;
        for( auto it=askTMap.begin(); it!=askTMap.end(); ++it )
        {
            try
            {
                (*res)[i].si     = it->second.si;
                (*res)[i].value    = IOController::localGetValue(it->second.ait,it->second.si.id);
                (*res)[i].type     = it->second.type;
            }
            catch(Exception& ex)
            {
                uwarn << myname << "(getThresholdsList): для датчика " 
                        << conf->oind->getNameById(it->second.si.id)
                        << " " << ex << endl;
                continue;
            }

            (*res)[i].tlist.length( it->second.list.size() );

            unsigned int k=0;
            for( auto it2=it->second.list.begin(); it2!=it->second.list.end(); ++it2 )
            {
                (*res)[i].tlist[k].id             = it2->id;
                (*res)[i].tlist[k].hilimit         = it2->hilimit;
                (*res)[i].tlist[k].lowlimit     = it2->lowlimit;
                (*res)[i].tlist[k].state         = it2->state;
                (*res)[i].tlist[k].tv_sec         = it2->tv_sec;
                (*res)[i].tlist[k].tv_usec         = it2->tv_usec;
                k++;
            }
            i++;
        }
    }
    return res;
}
// -----------------------------------------------------------------------------
void IONotifyController::onChangeUndefinedState( IOStateList::iterator& lit, IOController* ic )
{
    USensorInfo& it(lit->second);

    SensorMessage sm;

    // эти поля можно копировать без lock, т.к. они не меняются
    sm.id     = it.si.id;    
    sm.node = it.si.node;
    sm.undefined = it.undefined;
    sm.priority     = (Message::Priority)it.priority;
    sm.sensor_type     = it.type;
    sm.supplier     = DefaultObjectId;

    { // lock
        uniset_rwmutex_rlock lock(it.val_lock);
        sm.value         = it.value;
        sm.sm_tv_sec     = it.tv_sec;
        sm.sm_tv_usec     = it.tv_usec;
        sm.ci            = it.ci;
    } // unlock

    try
    {
        if( !it.dbignore )
            loggingInfo(sm);
    }
    catch(...){}

    { // lock
      uniset_rwmutex_rlock lock(askIOMutex);
      auto it1 = askIOList.find(it.si.id);
      if( it1!=askIOList.end() )
          send(it1->second, sm);
    } // unlock
}

// -----------------------------------------------------------------------------
IDSeq* IONotifyController::askSensorsSeq( const UniSetTypes::IDSeq& lst, 
                                            const UniSetTypes::ConsumerInfo& ci,
                                            UniversalIO::UIOCommand cmd)
{
    UniSetTypes::IDList badlist; // cписок не найденных идентификаторов

    int size = lst.length();
    ObjectId sid;
    for( int i=0; i<size; i++ )
    {
        sid = lst[i];
        try
        {
            askSensor(sid,ci,cmd);
        }
        catch(...)
        {
            badlist.add(sid);
        }
    }

    return badlist.getIDSeq();
}
// -----------------------------------------------------------------------------
