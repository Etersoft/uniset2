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
	string myfullname = uniset_conf()->oind->getNameById(id);
	myname = ORepHelpers::getShortName(myfullname);
}

PassiveObject::PassiveObject( ObjectId id, ProxyManager* mngr ):
	mngr(mngr),
	id(id)
{
	string myfullname = uniset_conf()->oind->getNameById(id);
	myname = ORepHelpers::getShortName(myfullname);

	if( mngr )
		mngr->attachObject(this, id);
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
	if( _mngr == mngr || !_mngr )
		return;

	// если уже инициализирован другим mngr (см. конструктор)
	if( mngr )
		mngr->detachObject(id);

	mngr = _mngr;
	mngr->attachObject(this, id);
}

// ------------------------------------------------------------------------------------------
void PassiveObject::processingMessage( const UniSetTypes::VoidMessage* msg )
{
	try
	{
		switch( msg->type )
		{
			case Message::SensorInfo:
				sensorInfo( reinterpret_cast<const SensorMessage*>(msg) );
				break;

			case Message::Timer:
				timerInfo( reinterpret_cast<const TimerMessage*>(msg) );
				break;

			case Message::SysCommand:
				sysCommand( reinterpret_cast<const SystemMessage*>(msg) );
				break;

			default:
				break;
		}
	}
	catch( const Exception& ex )
	{
		ucrit  << myname << "(processingMessage): " << ex << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		ucrit << myname << "(processingMessage): CORBA::SystemException: " << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& ex )
	{
		uwarn << myname << "(processingMessage): CORBA::Exception: " << ex._name() << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		auto ul = ulog();

		if( ul && ul->is_crit() )
		{
			ul->crit() << myname << "(processingMessage): Caught omniORB::fatalException:" << endl;
			ul->crit() << myname << "(processingMessage): file: " << fe.file()
					   << " line: " << fe.line()
					   << " mesg: " << fe.errmsg() << endl;
		}
	}
}
// -------------------------------------------------------------------------
void PassiveObject::sysCommand( const SystemMessage* sm )
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
