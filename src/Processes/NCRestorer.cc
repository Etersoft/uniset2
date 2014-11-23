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
void NCRestorer::addlist( IONotifyController* ic, SInfo&& inf, IONotifyController::ConsumerListInfo&& lst, bool force )
{
    // Проверка зарегистрирован-ли данный датчик
    // если такого дискретного датчика нет, то здесь сработает исключение...
    if( !force )
    {
        try
        {
            ic->getIOType(inf.si.id);
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
                    ic->ioRegistration( std::move(inf) );
                break;

                default:
                    ucrit << ic->getName() << "(askDumper::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА! -> "
                                    << uniset_conf()->oind->getNameById(inf.si.id) << endl;
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
            ic->askIOList[inf.si.id]=std::move(lst);
        break;

        default:
            ucrit << ic->getName() << "(NCRestorer::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА!-> "
                  << uniset_conf()->oind->getNameById(inf.si.id) << endl;
        break;
    }
}
// ------------------------------------------------------------------------------------------
void NCRestorer::addthresholdlist( IONotifyController* ic, SInfo&& inf, IONotifyController::ThresholdExtList&& lst, bool force )
{
    // Проверка зарегистрирован-ли данный датчик    
    // если такого дискретного датчика нет сдесь сработает исключение...
    if( !force )
    {
        try
        {
            ic->getIOType(inf.si.id);
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
                    ic->ioRegistration( std::move(inf) );
                break;

                default:
                    break;
            }
        }
    }

    // default init iterators
    for( auto &it: lst )
        it.sit = ic->myioEnd();

    ic->askTMap[inf.si.id].si      = inf.si;
    ic->askTMap[inf.si.id].type    = inf.type;
    ic->askTMap[inf.si.id].list    = std::move(lst);
    ic->askTMap[inf.si.id].ait     = ic->myioEnd();
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
    for( auto it=ic->ioList.begin(); it!=ic->ioList.end(); ++it )
    {
        // обновляем итераторы...
        it->second.it = it;

        if( it->second.d_si.id == DefaultObjectId )
            continue;

        uinfo << ic->getName() << "(NCRestorer::init_depends_signals): "
                << " init depend: '" << uniset_conf()->oind->getMapName(it->second.si.id) << "'"
                << " dep_name=(" << it->second.d_si.id << ")'" << uniset_conf()->oind->getMapName(it->second.d_si.id) << "'"
                << endl;

        IOController::ChangeSignal s = ic->signal_change_value(it->second.d_si.id);
        s.connect( sigc::mem_fun( &it->second, &IOController::USensorInfo::checkDepend) );
    }
}
// -----------------------------------------------------------------------------
