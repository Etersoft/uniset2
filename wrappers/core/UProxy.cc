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
#include <queue>
#include <string>
#include "unisetstd.h"
#include "Configuration.h"
#include "UProxy.h"
#include "Extensions.h"
#include "PassiveTimer.h"
#include "UInterface.h"
#include "IOController_i.hh"
// --------------------------------------------------------------------------
using namespace uniset;
// --------------------------------------------------------------------------
/*! PIMPL  реализация UProxy */
class UProxy_impl
{
	public:

		UProxy_impl( uniset::ObjectId id, const std::string& name );
		virtual ~UProxy_impl();

		long impl_getValue( long id ) throw(UException);
		void impl_setValue( long id, long val ) throw(UException);
		void impl_askSensor( long id ) throw(UException);

		UTypes::ShortIOInfo impl_waitMessage( int timeout_msec ) throw(UException);

		bool impl_isExist( long id ) throw(UException);

		void run( int sleep_msec=100 );
		void terminate();

	protected:

		void poll( uint sleep_msec );

		std::unique_ptr<std::thread> thr;
		std::atomic_bool term;
		bool initOK = { false };

		std::queue<UTypes::ShortIOInfo> qmsg;
		std::mutex qmutex;

		IOController_i::SensorInfoSeq_var prev;
		uniset::ObjectId smId = { uniset::DefaultObjectId };

		uniset::PassiveCondTimer ptWait;

		std::unique_ptr<uniset::UInterface> ui;


	private:
		std::string myname;

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
		uobj = std::make_shared<UProxy_impl>(id, myname);
	}
	catch( std::exception& ex )
	{
		std::ostringstream err;
		err << myname << "(UProxy::make): error: " << std::string(ex.what());
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
		return UTypes::ResultValue( uobj->impl_getValue(id) );
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
		uobj->impl_askSensor(id);
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
bool UProxy::run( int sleep_msec ) noexcept
{
	try
	{
		uobj->run(sleep_msec);
		return true;
	}
	catch(...){}

	return false;
}
// --------------------------------------------------------------------------
void UProxy::terminate() noexcept
{
	try
	{
		uobj->terminate();
	}
	catch(...){}
}
// --------------------------------------------------------------------------
UTypes::ShortIOInfo UProxy::waitMessage( int timeout_msec ) throw(UException)
{
	return uobj->impl_waitMessage(timeout_msec);
}
// --------------------------------------------------------------------------
UTypes::ResultIO UProxy::safeWaitMessage( int timeout_msec ) noexcept
{
	try
	{
		return UTypes::ResultIO( uobj->impl_waitMessage(timeout_msec), true );
	}
	catch(...){}

	return UTypes::ResultIO();
}
// --------------------------------------------------------------------------
UProxy_impl::UProxy_impl( ObjectId id, const std::string& name ):
	myname(name)
{
	smId = uniset::extensions::getSharedMemoryID();
	if( smId == uniset::DefaultObjectId )
	{
		std::cerr << myname << "(init): Unknown SharedMemory ID..." << std::endl;
		std::terminate();
	}

	ui = unisetstd::make_unique<uniset::UInterface>(id);
}
// --------------------------------------------------------------------------
UProxy_impl::~UProxy_impl()
{
	if( thr && !term )
		terminate();
}
// --------------------------------------------------------------------------
long UProxy_impl::impl_getValue( long id ) throw(UException)
{
	try
	{
		return ui->getValue(id);
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
		ui->setValue(id, val);
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
		ui->askSensor(id, UniversalIO::UIONotify);
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
UTypes::ShortIOInfo UProxy_impl::impl_waitMessage( int timeout_msec ) throw(UException)
{
	{
		std::lock_guard<std::mutex> lk(qmutex);
		if( !qmsg.empty() )
		{
			auto m = qmsg.front();
			qmsg.pop();
			return m;
		}
	}

	ptWait.wait(timeout_msec);

	std::lock_guard<std::mutex> lk(qmutex);
	if( !qmsg.empty() )
	{
		auto m = qmsg.front();
		qmsg.pop();
		return m;
	}

	throw UTimeOut();
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
void UProxy_impl::run( int sleep_msec )
{
	if( !thr )
		thr = std::unique_ptr<std::thread>( new std::thread(std::mem_fun(&UProxy_impl::poll),this,sleep_msec) );
}
// --------------------------------------------------------------------------
void UProxy_impl::terminate()
{
	std::cerr << "************** TERMINATE ***** " << std::endl;
	term = true;
	if( thr )
	{
		thr->join();
		thr = nullptr;
	}
}
// --------------------------------------------------------------------------
static UTypes::ShortIOInfo convert( const IOController_i::SensorIOInfo* si )
{
	UTypes::ShortIOInfo m;
	m.id = si->si.id;
	m.value = si->value;
	m.tv_sec = si->tv_sec;
	m.tv_nsec = si->tv_nsec;
	m.supplier = si->supplier;
	m.node = uniset::DefaultObjectId;
	m.consumer = uniset::DefaultObjectId;

	return m;
}
// --------------------------------------------------------------------------
void UProxy_impl::poll( uint sleep_msec )
{
	term = false;

	std::cerr << "*************** POLL THREAD START ************ " << std::endl;

	while( !term )
	{
		try
		{
			IOController_i::SensorInfoSeq_var smap = ui->getSensorsMap(smId);

			if( !initOK )
			{
				prev = smap;
				initOK = true;
			}
			else if( smap->length() == prev->length() )
			{
				// ищем разницу
				for( size_t i=0; i<smap->length(); i++ )
				{
					if( term )
						break;

					auto& s1 = smap[i];
					auto& s2 = prev[i];

					if( s1.si.id != s2.si.id )
					{
						// invalidate map...
						prev = smap;
						break;
					}

					if( s1.value != s2.value )
					{
						s2 = s1;
						std::lock_guard<std::mutex> lk(qmutex);
						qmsg.push( convert(&s1) );
						ptWait.terminate();
					}
				}
			}
			else
				prev = smap;
		}
		catch( std::exception& ex )
		{
			std::cerr << "(poll): " << ex.what() << std::endl;
		}

		msleep(sleep_msec);
	}

	std::cerr << "*************** POLL THREAD FINISHED ************ " << std::endl;
}
// --------------------------------------------------------------------------
