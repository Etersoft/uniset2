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

#include "Debug.h"
#include "Configuration.h"
#include "NCRestorer.h"
#include "Exceptions.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniversalIO;
using namespace UniSetTypes;
// ------------------------------------------------------------------------------------------
NCRestorer::NCRestorer()
{

}

NCRestorer::~NCRestorer()
{
}
// ------------------------------------------------------------------------------------------
void NCRestorer::addlist( IONotifyController* ic, SInfo& inf, IONotifyController::ConsumerList& lst, bool force )
{
    UniSetTypes::KeyType k( key(inf.si.id,inf.si.node) );

    // Проверка зарегистрирован-ли данный датчик    
    // если такого дискретного датчика нет, то здесь сработает исключение...
    if( !force )
    {
        try
        {
            ic->getIOType(inf.si);
        }
        catch(...)
        {
            // Регистрируем (если не найден)
            switch(inf.type)
            {
                case UniversalIO::DI:
                case UniversalIO::DO:
                case UniversalIO::AI:
                case UniversalIO::AO:
                    ic->ioRegistration(inf);
                break;
    
                default:
                    ucrit << ic->getName() << "(askDumper::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА! -> "
                                    << conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
                    return;
                break;
    
            }
        }
    }
    
    switch(inf.type)
    {
        case UniversalIO::DI:
        case UniversalIO::AI:
        case UniversalIO::DO:
        case UniversalIO::AO:
            ic->askIOList[k]=lst;
        break;

        default:
            ucrit << ic->getName() << "(NCRestorer::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА!-> "
                            << conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
        break;
    }
}
// ------------------------------------------------------------------------------------------
void NCRestorer::addthresholdlist( IONotifyController* ic, SInfo& inf, IONotifyController::ThresholdExtList& lst, bool force )
{
    // Проверка зарегистрирован-ли данный датчик    
    // если такого дискретного датчика нет сдесь сработает исключение...
    if( !force )
    {
        try
        {
            ic->getIOType(inf.si);
        }
        catch(...)
        {
            // Регистрируем (если не найден)
            switch(inf.type)
            {
                case UniversalIO::DI:
                case UniversalIO::DO:
                case UniversalIO::AI:
                case UniversalIO::AO:
                    ic->ioRegistration(inf);
                break;

                default:
                    break;
            }
        }
    }

    // default init iterators
    for( IONotifyController::ThresholdExtList::iterator it=lst.begin(); it!=lst.end(); ++it )
        it->sit = ic->myioEnd();

    UniSetTypes::KeyType k( key(inf.si.id,inf.si.node) );
    ic->askTMap[k].si    = inf.si;
    ic->askTMap[k].type    = inf.type;
    ic->askTMap[k].list    = lst;
    ic->askTMap[k].ait     = ic->myioEnd();

    // Начальная инициализация делается в IOController (IONotifyContoller) в момент "активации". см. IOController::activateInit()
#if 0
    try
    {
        switch( inf.type )
        {
            case UniversalIO::DI:
            case UniversalIO::DO:
            break;

            case UniversalIO::AO:
            case UniversalIO::AI:
            {
                IOController::IOStateList::iterator it(ic->myioEnd());
                ic->checkThreshold(it,inf.si,false);
            }
            break;

            default:
                break;
        }
    }
    catch( Exception& ex )
    {
        uwarn << ic->getName() << "(NCRestorer::addthresholdlist): " << ex
                << " для " << conf->oind->getNameById(inf.si.id, inf.si.node) << endl;
        throw;
    }
    catch( CORBA::SystemException& ex )
    {
        uwarn << ic->getName() << "(NCRestorer::addthresholdlist): "
                << conf->oind->getNameById(inf.si.id,inf.si.node) << " недоступен!!(CORBA::SystemException): "
                << ex.NP_minorString() << endl;
        throw;
    }
#endif
}
// ------------------------------------------------------------------------------------------
NCRestorer::SInfo& NCRestorer::SInfo::operator=( IOController_i::SensorIOInfo& inf )
{
    this->si         = inf.si;
    this->type         = inf.type;
    this->priority     = inf.priority;
    this->default_val = inf.default_val;
    this->real_value = inf.real_value;
    this->ci         = inf.ci;
    this->undefined = inf.undefined;
    this->blocked = inf.blocked;
    this->dbignore = inf.dbignore;
    this->any = 0;
    return *this;
//    CalibrateInfo ci;
//    long tv_sec;
//    long v_usec;
}
// ------------------------------------------------------------------------------------------
void NCRestorer::init_depends_signals( IONotifyController* ic )
{
    for( IOController::IOStateList::iterator it=ic->ioList.begin(); it!=ic->ioList.end(); ++it )
    {
        // обновляем итераторы...
        it->second.it = it;

        if( it->second.d_si.id == DefaultObjectId )
            continue;

        uinfo << ic->getName() << "(NCRestorer::init_depends_signals): "
                << " init depend: '" << conf->oind->getMapName(it->second.si.id) << "'"
                << " dep_name=(" << it->second.d_si.id << ")'" << conf->oind->getMapName(it->second.d_si.id) << "'"
                << endl;

        IOController::ChangeSignal s = ic->signal_change_value(it->second.d_si);
        s.connect( sigc::mem_fun( &it->second, &IOController::USensorInfo::checkDepend) );
	}
}
// -----------------------------------------------------------------------------
