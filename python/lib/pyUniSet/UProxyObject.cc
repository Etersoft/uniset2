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
#include "UProxyObject.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
// --------------------------------------------------------------------------
/*! PIMPL  реализация UProxyObject */
class UProxyObject_impl:
	public UObject_SK
{
	public:

		UProxyObject_impl( UniSetTypes::ObjectId id );
		virtual ~UProxyObject_impl();

		void impl_addToAsk( UniSetTypes::ObjectId id ) throw(UException);

		long impl_getValue( long id ) throw(UException);
		void impl_setValue( long id, long val ) throw(UException);
		float impl_getFloatValue( long id ) throw(UException);

	protected:
		virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;

	private:

		struct SInfo
		{
			IOController_i::SensorInfo si;
			long value = { 0 };
			float fvalue = { 0.0 };
		};

		std::mutex mutexSMap;
		std::unordered_map<UniSetTypes::ObjectId,SInfo> smap;
};
// --------------------------------------------------------------------------
UProxyObject::UProxyObject() throw(UException)
{
	throw UException("(UProxyObject): Unknown 'name'' or 'ID'");
}
// --------------------------------------------------------------------------
UProxyObject::UProxyObject( const std::string& name ) throw( UException ):
	UProxyObject::UProxyObject(uniset_conf()->getObjectID(name))
{
}
// --------------------------------------------------------------------------
UProxyObject::UProxyObject( long id ) throw( UException )
{
	try
	{
		uobj = std::make_shared<UProxyObject_impl>(id);
		auto act = UniSetActivator::Instance();
		act->add(uobj);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << "(UProxyObject): id='" << id << "' error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
UProxyObject::~UProxyObject()
{
}
// --------------------------------------------------------------------------
long UProxyObject::getValue( long id ) throw(UException)
{
	return uobj->impl_getValue(id);
}
// --------------------------------------------------------------------------
void UProxyObject::setValue( long id, long val ) throw(UException)
{
	uobj->impl_setValue(id,val);
}
// --------------------------------------------------------------------------
float UProxyObject::getFloatValue( long id ) throw(UException)
{
	return uobj->impl_getFloatValue(id);
}
// --------------------------------------------------------------------------
void UProxyObject::addToAsk( long id ) throw(UException)
{
	try
	{
		uobj->impl_addToAsk(id);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << uobj->getName() << "(addToAsk): " << id << " error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
UProxyObject_impl::UProxyObject_impl( ObjectId id ):
	UObject_SK(id)
{

}
// --------------------------------------------------------------------------
UProxyObject_impl::~UProxyObject_impl()
{

}
// --------------------------------------------------------------------------
void UProxyObject_impl::impl_addToAsk( ObjectId id ) throw( UException )
{
	UProxyObject_impl::SInfo i;
	i.si.id = id;
	i.si.node = uniset_conf()->getLocalNode();

	std::unique_lock<std::mutex> lk(mutexSMap);
	smap[id] = i;
}
// --------------------------------------------------------------------------
long UProxyObject_impl::impl_getValue( long id ) throw(UException)
{
	std::unique_lock<std::mutex> lk(mutexSMap);
	auto i = smap.find(id);
	if( i == smap.end() )
	{
		std::ostringstream err;
		err << myname << "(getValue): " << id << "  not found in proxy sensors list..";
		throw UException(err.str());
	}

	return i->second.value;
}
// --------------------------------------------------------------------------
float UProxyObject_impl::impl_getFloatValue( long id ) throw(UException)
{
	std::unique_lock<std::mutex> lk(mutexSMap);
	auto i = smap.find(id);
	if( i == smap.end() )
	{
		std::ostringstream err;
		err << myname << "(getFloatValue): " << id << "  not found in proxy sensors list..";
		throw UException(err.str());
	}

	return i->second.fvalue;
}
// --------------------------------------------------------------------------
void UProxyObject_impl::impl_setValue( long id, long val ) throw(UException)
{
	try
	{
		UObject_SK::setValue(id,val);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << myname << "(setValue): " << id << " error: " << std::string(ex.what());
		throw UException(err.str());
	}
}
// --------------------------------------------------------------------------
void UProxyObject_impl::askSensors( UniversalIO::UIOCommand cmd )
{
	std::unique_lock<std::mutex> lk(mutexSMap);
	for( const auto& i: smap )
	{
		try
		{
			ui->askRemoteSensor(i.second.si.id,cmd,i.second.si.node, getId());
		}
		catch( std::exception& ex )
		{
			std::ostringstream err;
			err << myname << "(askSensors): " << i.second.si.id << " error: " << std::string(ex.what());
			throw UException(err.str());
		}
	}
}
// --------------------------------------------------------------------------
void UProxyObject_impl::sensorInfo( const SensorMessage* sm )
{
	std::unique_lock<std::mutex> lk(mutexSMap);
	auto i = smap.find(sm->id);
	if( i != smap.end() )
	{
		i->second.value = sm->value;
		i->second.fvalue = (float)sm->value / pow(10.0, sm->ci.precision);
	}
}
// --------------------------------------------------------------------------
