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
#include <sstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include "Configuration.h"
#include "extensions/UObject_SK.h"
#include "UniSetActivator.h"
#include "UProxy.h"
// --------------------------------------------------------------------------
using namespace uniset;
// --------------------------------------------------------------------------
/*! PIMPL  реализация UProxy */
class UProxy_impl:
	public UObject_SK
{
	public:

		UProxy_impl( uniset::ObjectId id );
		virtual ~UProxy_impl();

		long impl_getValue( long id ) throw(UException);
		void impl_setValue( long id, long val ) throw(UException);
		void impl_askSensor( long id ) throw(UException);

		UTypes::ShortIOInfo impl_waitMessage( unsigned long timeout_msec ) throw(UException);

		bool impl_isExist( long id ) throw(UException);

	protected:

	private:

};
// --------------------------------------------------------------------------
UProxy::UProxy() throw(UException)
{
	throw UException("(UProxy): Unknown 'name'' or 'ID'");
}
// --------------------------------------------------------------------------
UProxy::UProxy( const std::string& name ) throw( UException )
{
	auto conf = uniset_conf();

	if ( !conf )
	{
		std::ostringstream err;
		err << "(UProxy:init): Create '" << name << "' failed. Unknown configuration";
		std::cerr << err.str() << std::endl;
		throw UException(err.str());
	}

	long id = conf->getObjectID(name);
	init(id);
}
// --------------------------------------------------------------------------
UProxy::UProxy( long id ) throw( UException )
{
	init(id);
}
// --------------------------------------------------------------------------
void UProxy::init( long id ) throw( UException )
{
	try
	{
		uobj = std::make_shared<UProxy_impl>(id);
		auto act = UniSetActivator::Instance();
		act->add(uobj);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << "(UProxy): id='" << id << "' error: " << std::string(ex.what());
		std::cerr << err.str() << std::endl;
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
UProxy::~UProxy()
{
}
// --------------------------------------------------------------------------
void UProxy::askSensor( long id ) throw(UException)
{
	return uobj->impl_askSensor(id);
}
// --------------------------------------------------------------------------
long UProxy::getValue( long id ) throw(UException)
{
	return uobj->impl_getValue(id);
}
// --------------------------------------------------------------------------
void UProxy::setValue( long id, long val ) throw( UException )
{
	uobj->impl_setValue(id, val);
}
// --------------------------------------------------------------------------
UTypes::ResultBool UProxy::safeSetValue( long id, long val ) noexcept
{
	try
	{
		uobj->impl_setValue(id, val);
		return UTypes::ResultBool(true);
	}
	catch( std::exception& ex )
	{
		return UTypes::ResultBool( false, ex.what() );
	}
	catch(...){}

	return UTypes::ResultBool(false,"Unknown error");
}
// --------------------------------------------------------------------------
UTypes::ResultValue UProxy::safeGetValue( long id ) noexcept
{
	try
	{
		return UTypes::ResultValue( uobj->getValue(id) );
	}
	catch( std::exception& ex )
	{
		return UTypes::ResultValue( 0, ex.what() );
	}
	catch(...){}

	return UTypes::ResultValue(0,"Unknown error");
}
// --------------------------------------------------------------------------
UTypes::ResultBool UProxy::safeAskSensor( long id ) noexcept
{
	try
	{
		uobj->askSensor(id,UniversalIO::UIONotify);
		return UTypes::ResultBool(true);
	}
	catch( std::exception& ex )
	{
		return UTypes::ResultBool(false, ex.what());
	}
	catch(...){}

	return UTypes::ResultBool(false,"Unknown error");
}
// --------------------------------------------------------------------------
bool UProxy::isExist( long id ) noexcept
{
	try
	{
		return uobj->impl_isExist(id);
	}
	catch(...){}
	return false;
}
// --------------------------------------------------------------------------
UTypes::ShortIOInfo UProxy::waitMessage( unsigned long timeout_msec ) throw(UException)
{
	return uobj->impl_waitMessage(timeout_msec);
}
// --------------------------------------------------------------------------
UTypes::ResultIO UProxy::safeWaitMessage( unsigned long timeout_msec ) noexcept
{
	try
	{
		return UTypes::ResultIO( uobj->impl_waitMessage(timeout_msec), true );
	}
	catch(...){}

	return UTypes::ResultIO();
}
// --------------------------------------------------------------------------
UProxy_impl::UProxy_impl( ObjectId id ):
	UObject_SK(id, nullptr)
{
	// отключаем собственный поток обработки сообщений
	offThread();
}
// --------------------------------------------------------------------------
UProxy_impl::~UProxy_impl()
{

}
// --------------------------------------------------------------------------
long UProxy_impl::impl_getValue( long id ) throw(UException)
{
	try
	{
		return getValue(id);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << myname << "(impl_getValue): " << id << " error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
void UProxy_impl::impl_setValue( long id, long val ) throw(UException)
{
	try
	{
		UObject_SK::setValue(id, val);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << myname << "(impl_setValue): " << id << " error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
void UProxy_impl::impl_askSensor( long id ) throw(UException)
{
	try
	{
		UObject_SK::askSensor(id, UniversalIO::UIONotify);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << myname << "(askSensor): " << id << " error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
static UTypes::ShortIOInfo smConvert( const uniset::SensorMessage* sm )
{
	UTypes::ShortIOInfo m;
	m.id = sm->id;
	m.value = sm->value;
	m.tv_sec = sm->sm_tv.tv_sec;
	m.tv_nsec = sm->sm_tv.tv_nsec;
	m.supplier = sm->supplier;
	m.node = sm->node;
	m.consumer = sm->consumer;

	return m;
}
// --------------------------------------------------------------------------
UTypes::ShortIOInfo UProxy_impl::impl_waitMessage( unsigned long timeout_msec ) throw(UException)
{
	while( true )
	{
		auto vm = waitMessage(timeout_msec);
		if( !vm )
			throw UTimeOut();

		if( vm->type == uniset::Message::SensorInfo )
			return smConvert( reinterpret_cast<const SensorMessage*>(vm.get()) );
	}
}
// --------------------------------------------------------------------------
bool UProxy_impl::impl_isExist( long id ) throw(UException)
{
	try
	{
		return ui->isExist(id);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << myname << "(isExist): " << id << " error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
