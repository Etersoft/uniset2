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
#include <omniORB4/CORBA.h>
#include <omniORB4/omniURI.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "ORepHelpers.h"
#include "UInterface.h"
#include "Configuration.h"
#include "PassiveTimer.h"

// -----------------------------------------------------------------------------
using namespace omni;
using namespace UniversalIO;
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
UInterface::UInterface( const std::shared_ptr<UniSetTypes::Configuration>& _uconf ):
	rep(_uconf),
	myid(UniSetTypes::DefaultObjectId),
	orb(CORBA::ORB::_nil()),
	rcache(100, 20),
	oind(_uconf->oind),
	uconf(_uconf)
{
	init();
}
// -----------------------------------------------------------------------------
UInterface::UInterface( const ObjectId backid, CORBA::ORB_var orb, const shared_ptr<ObjectIndex> _oind ):
	rep(UniSetTypes::uniset_conf()),
	myid(backid),
	orb(orb),
	rcache(200, 40),
	oind(_oind),
	uconf(UniSetTypes::uniset_conf())
{
	if( oind == nullptr )
		oind = uconf->oind;

	init();
}

UInterface::~UInterface()
{
}

void UInterface::init()
{
	// пытаемся получить ссылку на NameSerivice
	// в любом случае. даже если включён режим
	// localIOR
	localctx = CosNaming::NamingContext::_nil();

	try
	{
		ostringstream s;
		s << uconf << oind->getNodeName(uconf->getLocalNode());

		if( CORBA::is_nil(orb) )
		{
			CORBA::ORB_var _orb = uconf->getORB();
			localctx = ORepHelpers::getRootNamingContext( _orb, s.str() );
		}
		else
			localctx = ORepHelpers::getRootNamingContext( orb, s.str() );
	}
	catch( const Exception& ex )
	{
		//        if( !uconf->isLocalIOR() )
		//            throw ex;

		localctx = CosNaming::NamingContext::_nil();
	}
	catch( const std::exception& ex )
	{
		//        if( !uconf->isLocalIOR() )
		//            throw;

		localctx = CosNaming::NamingContext::_nil();
	}
}
// ------------------------------------------------------------------------------------------------------------
void UInterface::initBackId( const UniSetTypes::ObjectId backid )
{
	myid = backid;
}
// ------------------------------------------------------------------------------------------------------------
/*!
 * \param id - идентификатор датчика
 * \return текущее значение датчика
 * \exception IOBadParam - генерируется если указано неправильное имя датчика или секции
 * \exception IOTimeOut - генерируется если в течение времени timeout небыл получен ответ
*/
long UInterface::getValue( const ObjectId id, const ObjectId node ) const
throw(UI_THROW_EXCEPTIONS)
{
	if ( id == DefaultObjectId )
		throw ORepFailed("UI(getValue): error id=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getValue): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( id, node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				return iom->getValue(id);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch( const IOController_i::NameNotFound& ex )
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound("UI(getValue): " + string(ex.err));
	}
	catch( const IOController_i::IOBadParam& ex )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(getValue): " + string(ex.err));
	}
	catch( const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getValue): ORepFailed", id, node));
	}
	catch( const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getValue): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getValue): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(getValue): TimeOut", id, node));
}

long UInterface::getValue( const ObjectId name ) const
{
	return getValue(name, uconf->getLocalNode());
}


// ------------------------------------------------------------------------------------------------------------
void UInterface::setUndefinedState( const IOController_i::SensorInfo& si, bool undefined, UniSetTypes::ObjectId sup_id )
{
	if( si.id == DefaultObjectId )
	{
		uwarn << "UI(setUndefinedState): ID=UniSetTypes::DefaultObjectId" << endl;
		return;
	}

	if( sup_id == DefaultObjectId )
		sup_id = myid;

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(si.id, si.node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( si.id, si.node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				iom->setUndefinedState(si.id, undefined, sup_id );
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(si.id, si.node);
		uwarn << set_err("UI(setUndefinedState):" + string(ex.err), si.id, si.node) << endl;
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam("UI(setUndefinedState): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(si.id, si.node);
		// не смогли получить ссылку на объект
		uwarn << set_err("UI(setUndefinedState): resolve failed", si.id, si.node) << endl;
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(si.id, si.node);
		uwarn << set_err("UI(setUndefinedState): method no implement", si.id, si.node) << endl;
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(si.id, si.node);
		uwarn << set_err("UI(setUndefinedState): object not exist", si.id, si.node) << endl;
	}
	catch( const CORBA::COMM_FAILURE ) {}
	catch( const CORBA::SystemException& ex ) {}
	catch(...) {}

	rcache.erase(si.id, si.node);
	uwarn << set_err("UI(setUndefinedState): Timeout", si.id, si.node) << endl;
}
// ------------------------------------------------------------------------------------------------------------
/*!
 * \param id - идентификатор датчика
 * \param value - значение которое необходимо установить
 * \return текущее значение датчика
 * \exception IOBadParam - генерируется если указано неправильное имя вывода или секции
*/
void UInterface::setValue( const ObjectId id, long value, const ObjectId node, const UniSetTypes::ObjectId sup_id ) const
throw(UI_THROW_EXCEPTIONS)
{
	if ( id == DefaultObjectId )
		throw ORepFailed("UI(setValue): error: id=UniSetTypes::DefaultObjectId");

	if ( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(setValue): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}
/*
	if ( sup_id == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(setValue): id='" << id << "' error: supplier=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}
*/
	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( id, node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				iom->setValue(id, value, sup_id);
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound(set_err("UI(setValue): const NameNotFound&  для объекта", id, node));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(setValue): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(setValue): resolve failed ", id, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(setValue): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(setValue): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(setValue): Timeout", id, node));
}

void UInterface::setValue( const ObjectId name, long value ) const
{
	setValue(name, value, uconf->getLocalNode(), myid);
}


void UInterface::setValue( const IOController_i::SensorInfo& si, long value, const UniSetTypes::ObjectId sup_id ) const
{
	setValue(si.id, value, si.node,sup_id);
}

// ------------------------------------------------------------------------------------------------------------
// функция не вырабатывает исключий!
void UInterface::fastSetValue( const IOController_i::SensorInfo& si, long value, UniSetTypes::ObjectId sup_id ) const
{
	if ( si.id == DefaultObjectId )
	{
		uwarn << "UI(fastSetValue): ID=UniSetTypes::DefaultObjectId" << endl;
		return;
	}

	if( sup_id == DefaultObjectId )
		sup_id = myid;

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(si.id, si.node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( si.id, si.node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				iom->fastSetValue(si.id, value, sup_id);
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(si.id, si.node);
		uwarn << set_err("UI(fastSetValue): const NameNotFound&  для объекта", si.id, si.node) << endl;
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam("UI(fastSetValue): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(si.id, si.node);
		// не смогли получить ссылку на объект
		uwarn << set_err("UI(fastSetValue): resolve failed ", si.id, si.node) << endl;
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(si.id, si.node);
		uwarn << set_err("UI(fastSetValue): method no implement", si.id, si.node) << endl;
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(si.id, si.node);
		uwarn << set_err("UI(fastSetValue): object not exist", si.id, si.node) << endl;
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(setValue): CORBA::SystemException" << endl;
	}
	catch(...) {}

	rcache.erase(si.id, si.node);
	uwarn << set_err("UI(fastSetValue): Timeout", si.id, si.node) << endl;
}


// ------------------------------------------------------------------------------------------------------------
/*!
 * \param id     - идентификатор датчика
 * \param node        - идентификатор узла на котором заказывается датчик
 * \param cmd - команда см. \ref UniversalIO::UIOCommand
 * \param backid - обратный адрес (идентификатор заказчика)
*/
void UInterface::askRemoteSensor( const ObjectId id, UniversalIO::UIOCommand cmd, const ObjectId node,
								  UniSetTypes::ObjectId backid ) const throw(UI_THROW_EXCEPTIONS)
{
	if( backid == UniSetTypes::DefaultObjectId )
		backid = myid;

	if( backid == UniSetTypes::DefaultObjectId )
		throw UniSetTypes::IOBadParam("UI(askRemoteSensor): unknown back ID");

	if ( id == DefaultObjectId )
		throw ORepFailed("UI(askRemoteSensor): error: id=UniSetTypes::DefaultObjectId");

	if ( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(askRemoteSensor): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( id, node );

				IONotifyController_i_var inc = IONotifyController_i::_narrow(oref);
				ConsumerInfo_var ci;
				ci->id = backid;
				ci->node = uconf->getLocalNode();
				inc->askSensor(id, ci, cmd );
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound("UI(askSensor): " + string(ex.err) );
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(askSensor): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(askSensor): resolve failed ", id, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(askSensor): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(askSensor): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(askSensor): ошибка системы коммуникации" << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(askSensor): CORBA::SystemException" << endl;
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(askSensor): Timeout", id, node));
}

void UInterface::askSensor( const ObjectId name, UniversalIO::UIOCommand cmd, const UniSetTypes::ObjectId backid ) const
{
	askRemoteSensor(name, cmd, uconf->getLocalNode(), backid);
}

// ------------------------------------------------------------------------------------------------------------
/*!
 * \param id - идентификатор объекта
 * \param node - идентификатор узла
*/
IOType UInterface::getIOType( const ObjectId id, const ObjectId node ) const
throw(UI_THROW_EXCEPTIONS)
{
	if ( id == DefaultObjectId )
		throw ORepFailed("UI(getIOType): error: id=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getIOType): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(id, node);

				IOController_i_var inc = IOController_i::_narrow(oref);
				return inc->getIOType(id);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch(const IOController_i::NameNotFound& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound("UI(getIOType): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(getIOType): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getIOType): resolve failed ", id, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getIOType): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getIOType): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getIOType): ошибка системы коммуникации" << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getIOType): CORBA::SystemException" << endl;
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(getIOType): Timeout", id, node));
}

IOType UInterface::getIOType( const ObjectId id ) const
{
	return getIOType(id, uconf->getLocalNode() );
}
// ------------------------------------------------------------------------------------------------------------
/*!
 * \param id - идентификатор объекта
 * \param node - идентификатор узла
*/
ObjectType UInterface::getType(const ObjectId name, const ObjectId node) const
throw(UI_THROW_EXCEPTIONS)
{
	if ( name == DefaultObjectId )
		throw ORepFailed("UI(getType): попытка обратиться к объекту с id=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getType): id='" << name << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(name, node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( name, node );

				UniSetObject_i_var uo = UniSetObject_i::_narrow(oref);
				return uo->getType();
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch(const IOController_i::NameNotFound& ex)
	{
		rcache.erase(name, node);
		throw UniSetTypes::NameNotFound("UI(getType): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(name, node);
		throw UniSetTypes::IOBadParam("UI(getType): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(name, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getType): resolve failed ", name, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(name, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getType): method no implement", name, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(name, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getType): object not exist", name, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getType): ошибка системы коммуникации" << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getType): CORBA::SystemException" << endl;
	}
	catch( const UniSetTypes::TimeOut& ) {}

	rcache.erase(name, node);
	throw UniSetTypes::TimeOut(set_err("UI(getType): Timeout", name, node));
}

ObjectType UInterface::getType( const ObjectId name ) const
{
	return getType(name, uconf->getLocalNode());
}

// ------------------------------------------------------------------------------------------------------------
void UInterface::registered( const ObjectId id, const ObjectPtr oRef, bool force ) const throw(UniSetTypes::ORepFailed)
{
	// если влючён режим использования локальных файлов
	// то пишем IOR в файл
	if( uconf->isLocalIOR() )
	{
		if( CORBA::is_nil(orb) )
			orb = uconf->getORB();

		uconf->iorfile->setIOR(id, orb->object_to_string(oRef));
		return;
	}

	try
	{
		rep.registration( oind->getNameById(id), oRef, force );
	}
	catch( const Exception& ex )
	{
		throw;
	}
}

// ------------------------------------------------------------------------------------------------------------
void UInterface::unregister( const ObjectId id )throw(ORepFailed)
{
	if( uconf->isLocalIOR() )
	{
		uconf->iorfile->unlinkIOR(id);
		return;
	}

	try
	{
		rep.unregistration( oind->getNameById(id) );
	}
	catch( const Exception& ex )
	{
		throw;
	}
}

// ------------------------------------------------------------------------------------------------------------
ObjectPtr UInterface::resolve( const ObjectId rid , const ObjectId node ) const
throw(ResolveNameError, UniSetTypes::TimeOut )
{
	if ( rid == DefaultObjectId )
		throw ResolveNameError("UI(resolve): ID=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(resolve): id='" << rid << "' error: node=UniSetTypes::DefaultObjectId";
		throw ResolveNameError(err.str());
	}

	CosNaming::NamingContext_var ctx;
	rcache.erase(rid, node);

	try
	{
		if( uconf->isLocalIOR() )
		{
			if( CORBA::is_nil(orb) )
				orb = uconf->getORB();

			string sior(uconf->iorfile->getIOR(rid));

			if( !sior.empty() )
			{
				CORBA::Object_var nso = orb->string_to_object(sior.c_str());
				rcache.cache(rid, node, nso); // заносим в кэш
				return nso._retn();
			}
			else
			{
				// если NameService недоступен то,
				// сразу выдаём ошибку
				uwarn << "not found IOR-file for " << uconf->oind->getNameById(rid) << endl;
				throw UniSetTypes::ResolveNameError();
			}
		}

		if( node != uconf->getLocalNode() )
		{
			// Получаем доступ к NameService на данном узле
			ostringstream s;
			s << uconf << oind->getNodeName(node);
			string nodeName(s.str());
			string bname(nodeName); // сохраняем базовое название

			for(unsigned int curNet = 1; curNet <= uconf->getCountOfNet(); curNet++)
			{
				try
				{
					if( CORBA::is_nil(orb) )
						orb = uconf->getORB();

					ctx = ORepHelpers::getRootNamingContext( orb, nodeName );
					break;
				}
				//                catch( const CORBA::COMM_FAILURE& ex )
				catch( const ORepFailed& ex )
				{
					// нет связи с этим узлом
					// пробуем связатся по другой сети
					// ПО ПРАВИЛАМ узел в другой должен иметь имя NodeName1...NodeNameX
					ostringstream s;
					s << bname << curNet;
					nodeName = s.str();
				}
			}

			if( CORBA::is_nil(ctx) )
			{
				// uwarn << "NameService недоступен на узле "<< node << endl;
				throw NSResolveError();
			}
		}
		else
		{
			if( CORBA::is_nil(localctx) )
			{
				ostringstream s;
				s << uconf << oind->getNodeName(node);
				string nodeName(s.str());

				if( CORBA::is_nil(orb) )
				{
					CORBA::ORB_var _orb = uconf->getORB();
					localctx = ORepHelpers::getRootNamingContext( _orb, nodeName );
				}
				else
					localctx = ORepHelpers::getRootNamingContext( orb, nodeName );
			}

			ctx = localctx;
		}

		CosNaming::Name_var oname = omniURI::stringToName( oind->getNameById(rid).c_str() );

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				CORBA::Object_var nso = ctx->resolve(oname);

				if( CORBA::is_nil(nso) )
					throw UniSetTypes::ResolveNameError();

				// Для var
				rcache.cache(rid, node, nso); // заносим в кэш
				return nso._retn();
			}
			catch( const CORBA::TRANSIENT& ) {}

			msleep(uconf->getRepeatTimeout());
		}

		throw UniSetTypes::TimeOut();
	}
	catch(const CosNaming::NamingContext::NotFound& nf) {}
	catch(const CosNaming::NamingContext::InvalidName& nf) {}
	catch(const CosNaming::NamingContext::CannotProceed& cp) {}
	catch( const Exception ) {}
	catch( const CORBA::OBJECT_NOT_EXIST )
	{
		throw UniSetTypes::ResolveNameError("ObjectNOTExist");
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		throw UniSetTypes::ResolveNameError("CORBA::CommFailure");
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(resolve): CORBA::SystemException" << endl;
		throw UniSetTypes::TimeOut();
	}

	throw UniSetTypes::ResolveNameError();
}

// -------------------------------------------------------------------------------------------
void UInterface::send( const ObjectId name, const TransportMessage& msg, const ObjectId node )
throw(UI_THROW_EXCEPTIONS)
{
	if ( name == DefaultObjectId )
		throw ORepFailed("UI(send): ERROR: id=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(send): id='" << name << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(name, node);
		}
		catch( const NameNotFound& ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( name, node );

				UniSetObject_i_var obj = UniSetObject_i::_narrow(oref);
				obj->push(msg);
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const ORepFailed )
	{
		rcache.erase(name, node);
		throw UniSetTypes::IOBadParam(set_err("UI(send): resolve failed ", name, node));
	}
	catch( const CORBA::NO_IMPLEMENT )
	{
		rcache.erase(name, node);
		throw UniSetTypes::IOBadParam(set_err("UI(send): method no implement", name, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST )
	{
		rcache.erase(name, node);
		throw UniSetTypes::IOBadParam(set_err("UI(send): object not exist", name, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(send): ошибка системы коммуникации" << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(send): CORBA::SystemException" << endl;
	}

	rcache.erase(name, node);
	throw UniSetTypes::TimeOut(set_err("UI(send): Timeout", name, node));
}

void UInterface::send( const ObjectId name, const TransportMessage& msg )
{
	send(name, msg, uconf->getLocalNode());
}

// ------------------------------------------------------------------------------------------------------------
IOController_i::ShortIOInfo UInterface::getChangedTime( const ObjectId id, const ObjectId node ) const
{
	if( id == DefaultObjectId )
		throw ORepFailed("UI(getChangedTime): Unknown id=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getChangedTime): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound& ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( id, node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				return iom->getChangedTime(id);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch( const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(id, node);
		uwarn << "UI(getChangedTime): " << ex.err << endl;
	}
	catch( const IOController_i::IOBadParam& ex )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(getChangedTime): " + string(ex.err));
	}
	catch( const ORepFailed& )
	{
		rcache.erase(id, node);
		uwarn << set_err("UI(getChangedTime): resolve failed ", id, node) << endl;
	}
	catch( const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		uwarn << set_err("UI(getChangedTime): method no implement", id, node) << endl;
	}
	catch( const CORBA::OBJECT_NOT_EXIST& e )
	{
		rcache.erase(id, node);
		uwarn << set_err("UI(getChangedTime): object not exist", id, node) << endl;
	}
	catch( const CORBA::COMM_FAILURE& e )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(saveState): CORBA::COMM_FAILURE " << endl;
	}
	catch( const CORBA::SystemException& ex)
	{
		// ошибка системы коммуникации
		// uwarn << "UI(saveState): CORBA::SystemException" << endl;
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(getChangedTime): Timeout", id, node));
}
// ------------------------------------------------------------------------------------------------------------

ObjectPtr UInterface::CacheOfResolve::resolve( const ObjectId id, const ObjectId node ) const
throw(NameNotFound)
{
	UniSetTypes::uniset_rwmutex_rlock l(cmutex);

	auto it = mcache.find( key(id, node) );

	if( it == mcache.end() )
		throw UniSetTypes::NameNotFound();

	it->second.ncall++;

	// т.к. функция возвращает указатель
	// и тот кто вызывает отвечает за освобождение памяти
	// то мы делаем _duplicate....

	if( !CORBA::is_nil(it->second.ptr) )
		return CORBA::Object::_duplicate(it->second.ptr);

	throw UniSetTypes::NameNotFound();
}
// ------------------------------------------------------------------------------------------------------------
void UInterface::CacheOfResolve::cache( const ObjectId id, const ObjectId node, ObjectVar& ptr ) const
{
	UniSetTypes::uniset_rwmutex_wrlock l(cmutex);

	UniSetTypes::KeyType k( key(id, node) );

	auto it = mcache.find(k);

	if( it == mcache.end() )
		mcache.emplace(k, Item(ptr));
	else
	{
		it->second.ptr = ptr; // CORBA::Object::_duplicate(ptr);
		it->second.ncall++;
	}
}
// ------------------------------------------------------------------------------------------------------------
bool UInterface::CacheOfResolve::clean()
{
	UniSetTypes::uniset_rwmutex_wrlock l(cmutex);

	uinfo << "UI: clean cache...." << endl;

	for( auto it = mcache.begin(); it != mcache.end();)
	{
		if( it->second.ncall <= minCallCount )
			mcache.erase(it++);
		else
			++it;
	}

	if( mcache.size() < MaxSize )
		return true;

	return false;
}
// ------------------------------------------------------------------------------------------------------------

void UInterface::CacheOfResolve::erase( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const
{
	UniSetTypes::uniset_rwmutex_wrlock l(cmutex);
	auto it = mcache.find( key(id, node) );

	if( it != mcache.end() )
		mcache.erase(it);
}

// ------------------------------------------------------------------------------------------------------------
bool UInterface::isExist( const UniSetTypes::ObjectId id ) const
{
	try
	{
		if( uconf->isLocalIOR() )
		{
			if( CORBA::is_nil(orb) )
				orb = uconf->getORB();

			string sior( uconf->iorfile->getIOR(id) );

			if( !sior.empty() )
			{
				CORBA::Object_var oref = orb->string_to_object(sior.c_str());
				return rep.isExist( oref );
			}

			return false;
		}

		return rep.isExist( oind->getNameById(id) );
	}
	catch( const UniSetTypes::Exception& ex )
	{
		// uwarn << "UI(isExist): " << ex << endl;
	}
	catch(...) {}

	return false;
}
// ------------------------------------------------------------------------------------------------------------
bool UInterface::isExist( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const
{
	if( node == uconf->getLocalNode() )
		return isExist(id);

	CORBA::Object_var oref;

	try
	{
		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound& ) {}

		if( CORBA::is_nil(oref) )
			oref = resolve(id, node);

		return rep.isExist( oref );
	}
	catch(...) {}

	return false;
}
// --------------------------------------------------------------------------------------------
string UInterface::set_err( const std::string& pre, const ObjectId id, const ObjectId node ) const
{
	if( id == UniSetTypes::DefaultObjectId )
		return string(pre + " DefaultObjectId");

	string nm( oind->getNameById(node) );

	if( nm.empty() )
		nm = "UnknownName";

	ostringstream s;
	s << pre << " (" << id << ":" << node << ")" << nm;
	return s.str();
}
// --------------------------------------------------------------------------------------------
void UInterface::askThreshold( const ObjectId sid, const ThresholdId tid,
							   UniversalIO::UIOCommand cmd,
							   long low, long hi, bool invert,
							   const ObjectId backid ) const
{
	askRemoteThreshold(sid, uconf->getLocalNode(), tid, cmd, low, hi, invert, backid);
}
// --------------------------------------------------------------------------------------------
void UInterface::askRemoteThreshold( const ObjectId sid, const ObjectId node,
									 ThresholdId tid, UniversalIO::UIOCommand cmd,
									 long lowLimit, long hiLimit, bool invert,
									 ObjectId backid ) const
{
	if( backid == UniSetTypes::DefaultObjectId )
		backid = myid;

	if( backid == UniSetTypes::DefaultObjectId )
		throw UniSetTypes::IOBadParam("UI(askRemoteThreshold): unknown back ID");

	if ( sid == DefaultObjectId )
		throw ORepFailed("UI(askRemoteThreshold): error: id=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(askRemoteThreshold): id='" << sid << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(sid, node);
		}
		catch( const NameNotFound&  ) {}

		for (unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( sid, node );

				IONotifyController_i_var inc = IONotifyController_i::_narrow(oref);
				ConsumerInfo_var ci;
				ci->id = backid;
				ci->node = uconf->getLocalNode();

				inc->askThreshold(sid, ci, tid, lowLimit, hiLimit, invert, cmd);
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound& ex)
	{
		rcache.erase(sid, node);
		throw UniSetTypes::NameNotFound("UI(askThreshold): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(sid, node);
		throw UniSetTypes::IOBadParam("UI(askThreshold): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(sid, node);
		throw UniSetTypes::IOBadParam(set_err("UI(askThreshold): resolve failed ", sid, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(sid, node);
		throw UniSetTypes::IOBadParam(set_err("UI(askThreshold): method no implement", sid, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(sid, node);
		throw UniSetTypes::IOBadParam(set_err("UI(askThreshold): object not exist", sid, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(askThreshold): ошибка системы коммуникации" << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(askThreshold): CORBA::SystemException" << endl;
	}

	rcache.erase(sid, node);
	throw UniSetTypes::TimeOut(set_err("UI(askThreshold): Timeout", sid, node));

}
// --------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdInfo
UInterface::getThresholdInfo( const ObjectId sid, const ThresholdId tid ) const
{
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = uconf->getLocalNode();
	return getThresholdInfo(si, tid);
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdInfo
UInterface::getThresholdInfo( const IOController_i::SensorInfo& si, const UniSetTypes::ThresholdId tid ) const
{
	if ( si.id == DefaultObjectId )
		throw ORepFailed("UI(getThresholdInfo): error: id=UniSetTypes::DefaultObjectId");

	if( si.node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getThresholdInfo): id='" << si.id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(si.id, si.node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( si.id, si.node );

				IONotifyController_i_var iom = IONotifyController_i::_narrow(oref);
				return iom->getThresholdInfo(si.id, tid);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::NameNotFound("UI(getThresholdInfo): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam("UI(getThresholdInfo): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(si.id, si.node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getThresholdInfo): resolve failed ", si.id, si.node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(getThresholdInfo): method no implement", si.id, si.node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(getThresholdInfo): object not exist", si.id, si.node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(si.id, si.node);
	throw UniSetTypes::TimeOut(set_err("UI(getThresholdInfo): Timeout", si.id, si.node));
}
// --------------------------------------------------------------------------------------------
long UInterface::getRawValue( const IOController_i::SensorInfo& si )
{
	if ( si.id == DefaultObjectId )
		throw ORepFailed("UI(getRawValue): error: id=UniSetTypes::DefaultObjectId");

	if( si.node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getRawValue): id='" << si.id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(si.id, si.node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( si.id, si.node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				return iom->getRawValue(si.id);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::NameNotFound("UI(getRawValue): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam("UI(getRawValue): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(si.id, si.node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getRawValue): resolve failed ", si.id, si.node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(getRawValue): method no implement", si.id, si.node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(getRawValue): object not exist", si.id, si.node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(si.id, si.node);
	throw UniSetTypes::TimeOut(set_err("UI(getRawValue): Timeout", si.id, si.node));
}
// --------------------------------------------------------------------------------------------
void UInterface::calibrate(const IOController_i::SensorInfo& si,
						   const IOController_i::CalibrateInfo& ci,
						   UniSetTypes::ObjectId admId )
{
	if( admId == UniSetTypes::DefaultObjectId )
		admId = myid;

	//    if( admId==UniSetTypes::DefaultObjectId )
	//        throw UniSetTypes::IOBadParam("UI(askTreshold): неизвестен ID администратора");

	if( si.id == DefaultObjectId )
		throw ORepFailed("UI(calibrate): error: id=UniSetTypes::DefaultObjectId");

	if( si.node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(calibrate): id='" << si.id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(si.id, si.node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( si.id, si.node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				iom->calibrate(si.id, ci, admId);
				return;
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::NameNotFound("UI(calibrate): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam("UI(calibrate): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(si.id, si.node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(calibrate): resolve failed ", si.id, si.node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(calibrate): method no implement", si.id, si.node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(calibrate): object not exist", si.id, si.node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(si.id, si.node);
	throw UniSetTypes::TimeOut(set_err("UI(calibrate): Timeout", si.id, si.node));
}
// --------------------------------------------------------------------------------------------
IOController_i::CalibrateInfo UInterface::getCalibrateInfo( const IOController_i::SensorInfo& si )
{
	if ( si.id == DefaultObjectId )
		throw ORepFailed("UI(getCalibrateInfo): попытка обратиться к объекту с id=UniSetTypes::DefaultObjectId");

	if( si.node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getCalibrateInfo): id='" << si.id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(si.id, si.node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve( si.id, si.node );

				IOController_i_var iom = IOController_i::_narrow(oref);
				return iom->getCalibrateInfo(si.id);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::NameNotFound("UI(getCalibrateInfo): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam("UI(getCalibrateInfo): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(si.id, si.node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getCalibrateInfo): resolve failed ", si.id, si.node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(getCalibrateInfo): method no implement", si.id, si.node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(si.id, si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(getCalibrateInfo): object not exist", si.id, si.node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(si.id, si.node);
	throw UniSetTypes::TimeOut(set_err("UI(getCalibrateInfo): Timeout", si.id, si.node));
}
// --------------------------------------------------------------------------------------------
IOController_i::SensorInfoSeq_var UInterface::getSensorSeq( const UniSetTypes::IDList& lst )
{
	if( lst.empty() )
		return IOController_i::SensorInfoSeq_var();

	ObjectId sid = lst.getFirst();

	if ( sid == DefaultObjectId )
		throw ORepFailed("UI(getSensorSeq): попытка обратиться к объекту с id=UniSetTypes::DefaultObjectId");

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(sid, uconf->getLocalNode());
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(sid, uconf->getLocalNode());

				IOController_i_var iom = IOController_i::_narrow(oref);

				UniSetTypes::IDSeq_var seq(lst.getIDSeq());
				return iom->getSensorSeq(seq);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::NameNotFound("UI(getSensorSeq): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::IOBadParam("UI(getSensorSeq): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(sid, uconf->getLocalNode());
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getSensorSeq): resolve failed ", sid, uconf->getLocalNode()));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::IOBadParam(set_err("UI(getSensorSeq): method no implement", sid, uconf->getLocalNode()));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::IOBadParam(set_err("UI(getSensorSeq): object not exist", sid, uconf->getLocalNode()));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(sid, uconf->getLocalNode());
	throw UniSetTypes::TimeOut(set_err("UI(getSensorSeq): Timeout", sid, uconf->getLocalNode()));

}
// --------------------------------------------------------------------------------------------
IDSeq_var UInterface::setOutputSeq( const IOController_i::OutSeq& lst, UniSetTypes::ObjectId sup_id )
{
	if( lst.length() == 0 )
		return UniSetTypes::IDSeq_var();


	if ( lst[0].si.id == DefaultObjectId )
		throw ORepFailed("UI(setOutputSeq): попытка обратиться к объекту с id=UniSetTypes::DefaultObjectId");

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(lst[0].si.id, lst[0].si.node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(lst[0].si.id, lst[0].si.node);

				IONotifyController_i_var iom = IONotifyController_i::_narrow(oref);
				return iom->setOutputSeq(lst, sup_id);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(lst[0].si.id, lst[0].si.node);
		throw UniSetTypes::NameNotFound("UI(setOutputSeq): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(lst[0].si.id, lst[0].si.node);
		throw UniSetTypes::IOBadParam("UI(setOutputSeq): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(lst[0].si.id, lst[0].si.node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(setOutputSeq): resolve failed ", lst[0].si.id, lst[0].si.node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(lst[0].si.id, lst[0].si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(setOutputSeq): method no implement", lst[0].si.id, lst[0].si.node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(lst[0].si.id, lst[0].si.node);
		throw UniSetTypes::IOBadParam(set_err("UI(setOutputSeq): object not exist", lst[0].si.id, lst[0].si.node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(lst[0].si.id, lst[0].si.node);
	throw UniSetTypes::TimeOut(set_err("UI(setOutputSeq): Timeout", lst[0].si.id, lst[0].si.node));
}
// --------------------------------------------------------------------------------------------
UniSetTypes::IDSeq_var UInterface::askSensorsSeq( const UniSetTypes::IDList& lst,
		UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId backid )
{
	if( lst.empty() )
		return UniSetTypes::IDSeq_var();

	if( backid == UniSetTypes::DefaultObjectId )
		backid = myid;

	if( backid == UniSetTypes::DefaultObjectId )
		throw UniSetTypes::IOBadParam("UI(askSensorSeq): unknown back ID");

	ObjectId sid = lst.getFirst();

	if ( sid == DefaultObjectId )
		throw ORepFailed("UI(askSensorSeq): попытка обратиться к объекту с id=UniSetTypes::DefaultObjectId");

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(sid, uconf->getLocalNode());
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(sid, uconf->getLocalNode());

				IONotifyController_i_var iom = IONotifyController_i::_narrow(oref);

				ConsumerInfo_var ci;
				ci->id = backid;
				ci->node = uconf->getLocalNode();
				UniSetTypes::IDSeq_var seq = lst.getIDSeq();

				return iom->askSensorsSeq(seq, ci, cmd);
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::NameNotFound("UI(getSensorSeq): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::IOBadParam("UI(getSensorSeq): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(sid, uconf->getLocalNode());
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(askSensorSeq): resolve failed ", sid, uconf->getLocalNode()));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::IOBadParam(set_err("UI(askSensorSeq): method no implement", sid, uconf->getLocalNode()));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(sid, uconf->getLocalNode());
		throw UniSetTypes::IOBadParam(set_err("UI(askSensorSeq): object not exist", sid, uconf->getLocalNode()));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(sid, uconf->getLocalNode());
	throw UniSetTypes::TimeOut(set_err("UI(askSensorSeq): Timeout", sid, uconf->getLocalNode()));
}
// -----------------------------------------------------------------------------
IOController_i::ShortMapSeq* UInterface::getSensors( const UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )
{
	if ( id == DefaultObjectId )
		throw ORepFailed("UI(getSensors): error node=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getSensors): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(id, node);

				IOController_i_var iom = IOController_i::_narrow(oref);
				return iom->getSensors();
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound("UI(getSensors): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(getSensors): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getSensors): resolve failed ", id, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getSensors): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getSensors): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
		// uwarn << "UI(getValue): CORBA::SystemException" << endl;
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(getSensors): Timeout", id, node));
}
// -----------------------------------------------------------------------------
IOController_i::SensorInfoSeq* UInterface::getSensorsMap( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node )
{
	if ( id == DefaultObjectId )
		throw ORepFailed("UI(getSensorsMap): error node=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getSensorsMap): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(id, node);

				IOController_i_var iom = IOController_i::_narrow(oref);
				return iom->getSensorsMap();
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound("UI(getSensorsMap): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(getSensorsMap): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getSensorsMap): resolve failed ", id, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getSensorsMap): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getSensorsMap): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(getSensorsMap): Timeout", id, node));
}
// -----------------------------------------------------------------------------
IONotifyController_i::ThresholdsListSeq* UInterface::getThresholdsList( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node )
{
	if ( id == DefaultObjectId )
		throw ORepFailed("UI(getThresholdsList): error node=UniSetTypes::DefaultObjectId");

	if( node == DefaultObjectId )
	{
		ostringstream err;
		err << "UI(getThresholdsList): id='" << id << "' error: node=UniSetTypes::DefaultObjectId";
		throw ORepFailed(err.str());
	}

	try
	{
		CORBA::Object_var oref;

		try
		{
			oref = rcache.resolve(id, node);
		}
		catch( const NameNotFound&  ) {}

		for( unsigned int i = 0; i < uconf->getRepeatCount(); i++)
		{
			try
			{
				if( CORBA::is_nil(oref) )
					oref = resolve(id, node);

				IONotifyController_i_var iom = IONotifyController_i::_narrow(oref);
				return iom->getThresholdsList();
			}
			catch( const CORBA::TRANSIENT& ) {}
			catch( const CORBA::OBJECT_NOT_EXIST& ) {}
			catch( const CORBA::SystemException& ex ) {}

			msleep(uconf->getRepeatTimeout());
			oref = CORBA::Object::_nil();
		}
	}
	catch( const UniSetTypes::TimeOut& ) {}
	catch(const IOController_i::NameNotFound&  ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::NameNotFound("UI(getThresholdsList): " + string(ex.err));
	}
	catch(const IOController_i::IOBadParam& ex)
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam("UI(getThresholdsList): " + string(ex.err));
	}
	catch(const ORepFailed& )
	{
		rcache.erase(id, node);
		// не смогли получить ссылку на объект
		throw UniSetTypes::IOBadParam(set_err("UI(getThresholdsList): resolve failed ", id, node));
	}
	catch(const CORBA::NO_IMPLEMENT& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getThresholdsList): method no implement", id, node));
	}
	catch( const CORBA::OBJECT_NOT_EXIST& )
	{
		rcache.erase(id, node);
		throw UniSetTypes::IOBadParam(set_err("UI(getThresholdsList): object not exist", id, node));
	}
	catch( const CORBA::COMM_FAILURE& ex )
	{
		// ошибка системы коммуникации
	}
	catch( const CORBA::SystemException& ex )
	{
		// ошибка системы коммуникации
	}

	rcache.erase(id, node);
	throw UniSetTypes::TimeOut(set_err("UI(getThresholdsList): Timeout", id, node));
}
// -----------------------------------------------------------------------------
bool UInterface::waitReady( const ObjectId id, int msec, int pmsec, const ObjectId node )
{
	if( msec < 0 )
		msec = 0;

	if( pmsec < 0 )
		pmsec = 0;

	PassiveTimer ptReady(msec);
	bool ready = false;

	while( !ptReady.checkTime() && !ready )
	{
		try
		{
			ready = isExist(id, node);

			if( ready )
				break;
		}
		catch( const CORBA::OBJECT_NOT_EXIST& )
		{
		}
		catch( const CORBA::COMM_FAILURE& ex )
		{
		}
		catch(...)
		{
			break;
		}

		msleep(pmsec);
	}

	return ready;
}
// -----------------------------------------------------------------------------
bool UInterface::waitWorking( const ObjectId id, int msec, int pmsec, const ObjectId node )
{
	if( msec < 0 )
		msec = 0;

	if( pmsec < 0 )
		pmsec = 0;

	PassiveTimer ptReady(msec);
	bool ready = false;

	while( !ptReady.checkTime() && !ready )
	{
		try
		{
			getValue(id, node);
			ready = true;
			break;
		}
		catch(...) {}

		msleep(pmsec);
	}

	return ready;
}
// -----------------------------------------------------------------------------
UniversalIO::IOType UInterface::getConfIOType( const UniSetTypes::ObjectId id ) const
{
	if( !uconf )
		return UniversalIO::UnknownIOType;

	xmlNode* x = uconf->getXMLObjectNode(id);

	if( !x )
		return UniversalIO::UnknownIOType;

	UniXML::iterator it(x);
	return UniSetTypes::getIOType( it.getProp("iotype") );
}
// -----------------------------------------------------------------------------
