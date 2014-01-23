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
#include <iomanip>
#include "ProxyManager.h"
#include "PassiveObject.h"
#include "ORepHelpers.h"
#include "Configuration.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// ------------------------------------------------------------------------------------------
PassiveObject::PassiveObject():
    mngr(0),
    id(UniSetTypes::DefaultObjectId)
{

}

PassiveObject::PassiveObject( UniSetTypes::ObjectId id ):
    mngr(0),
    id(id)
{
    string myfullname = conf->oind->getNameById(id);
    myname = ORepHelpers::getShortName(myfullname.c_str());
}

PassiveObject::PassiveObject( ObjectId id, ProxyManager* mngr ):
    mngr(mngr),
    id(id)
{
    string myfullname = conf->oind->getNameById(id);
    myname = ORepHelpers::getShortName(myfullname.c_str());

    if( mngr )
        mngr->attachObject(this,id);
}

// ------------------------------------------------------------------------------------------

PassiveObject::~PassiveObject()
{
}

// ------------------------------------------------------------------------------------------
void PassiveObject::setID( UniSetTypes::ObjectId id_ )
{
    id = id_;
}
// ------------------------------------------------------------------------------------------
void PassiveObject::init( ProxyManager* _mngr )
{
    if(mngr)
        mngr->detachObject(id);

    mngr->attachObject(this,id);

#if 0
    if( _mngr == mngr || !_mngr )
        return;

    // если уже инициализирован другим mngr (см. конструктор)
    if( mngr )
        mngr->detachObject(id);

    mngr = _mngr;
    mngr->attachObject(this,id);
#endif
}

// ------------------------------------------------------------------------------------------
void PassiveObject::processingMessage( UniSetTypes::VoidMessage *msg )
{
    try
    {
        switch( msg->type )
        {
            case Message::SensorInfo:
                sensorInfo( reinterpret_cast<SensorMessage*>(msg) );
            break;

            case Message::Timer:
                timerInfo( reinterpret_cast<TimerMessage*>(msg) );
            break;

            case Message::SysCommand:
                sysCommand( reinterpret_cast<SystemMessage*>(msg) );
            break;

            default:
                break;
        }
    }
    catch( Exception& ex )
    {
        ucrit  << myname << "(processingMessage): " << ex << endl;
    }
    catch(CORBA::SystemException& ex)
    {
        ucrit << myname << "(processingMessage): CORBA::SystemException: " << ex.NP_minorString() << endl;
      }
    catch(CORBA::Exception& ex)
    {
        uwarn << myname << "(processingMessage): CORBA::Exception: " << ex._name() << endl;
    }
    catch( omniORB::fatalException& fe )
    {
        if( ulog.is_crit() )
        {
            ulog.crit() << myname << "(processingMessage): Caught omniORB::fatalException:" << endl;
            ulog.crit() << myname << "(processingMessage): file: " << fe.file()
                << " line: " << fe.line()
                << " mesg: " << fe.errmsg() << endl;
        }
    }
    catch(...)
    {
        ucrit << myname << "(processingMessage): catch..." << endl;
    }
}
// -------------------------------------------------------------------------
void PassiveObject::sysCommand( const SystemMessage *sm )
{
    switch( sm->command )
    {
        case SystemMessage::StartUp:
            askSensors(UniversalIO::UIONotify);
            break;

        case SystemMessage::FoldUp:
        case SystemMessage::Finish:
            askSensors(UniversalIO::UIODontNotify);
            break;

        case SystemMessage::WatchDog:
        case SystemMessage::LogRotate:
            break;

        default:
            break;
    }
}
// -------------------------------------------------------------------------
