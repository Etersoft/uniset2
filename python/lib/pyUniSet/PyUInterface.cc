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
// -------------------------------------------------------------------------
#include <ostream>
#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "Configuration.h"
#include "UniSetActivator.h"
#include "UniSetTypes.h"
#include "PyUInterface.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
static uniset::UInterface* uInterface = 0;
//---------------------------------------------------------------------------
void pyUInterface::uniset_init_params( UTypes::Params* p, const std::string& xmlfile )throw(UException)
{
	pyUInterface::uniset_init(p->argc, p->argv, xmlfile);
}
//---------------------------------------------------------------------------

void pyUInterface::uniset_init( int argc, char* argv[], const std::string& xmlfile )throw(UException)
{
	if( uInterface )
		return;

	try
	{
		uniset::uniset_init(argc, argv, xmlfile);
		uInterface = new uniset::UInterface();
		return;
	}
	catch( uniset::Exception& ex )
	{
		throw UException(ex.what());
	}
	catch( std::exception& ex )
	{
		throw UException(ex.what());
	}
}
//---------------------------------------------------------------------------
long pyUInterface::getValue( long id )throw(UException)
{
	auto conf = uniset::uniset_conf();

	if( !conf || !uInterface )
		throw USysError();

	using namespace uniset;

	UniversalIO::IOType t = conf->getIOType(id);

	if( t == UniversalIO::UnknownIOType )
	{
		ostringstream e;
		e << "(getValue): Unknown iotype for id=" << id;
		throw UException(e.str());
	}

	try
	{
		return uInterface->getValue(id);
	}
	catch( UException& ex )
	{
		throw;
	}
	catch( uniset::Exception& ex )
	{
		throw UException(ex.what());
	}
	catch( std::exception& ex )
	{
		throw UException(ex.what());
	}
}
//---------------------------------------------------------------------------
void pyUInterface::setValue( long id, long val, long supplier )throw(UException)
{
	auto conf = uniset::uniset_conf();

	if( !conf || !uInterface )
		throw USysError();

	using namespace uniset;

	UniversalIO::IOType t = conf->getIOType(id);

	if( t == UniversalIO::UnknownIOType )
	{
		ostringstream e;
		e << "(setValue): Unknown iotype for id=" << id;
		throw UException(e.str());
	}

	try
	{
		uInterface->setValue(id, val, supplier);
	}
	catch( UException& ex )
	{
		throw;
	}
	catch( uniset::Exception& ex )
	{
		throw UException(ex.what());
	}
	catch( std::exception& ex )
	{
		throw UException(ex.what());
	}
}
//---------------------------------------------------------------------------
long pyUInterface::getSensorID(const string& name )
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->getSensorID(name);

	return uniset::DefaultObjectId;
}
//---------------------------------------------------------------------------
long pyUInterface::getObjectID(const string& name )
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->getObjectID(name);

	return uniset::DefaultObjectId;
}
//---------------------------------------------------------------------------
std::string pyUInterface::getName( long id )
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->oind->getMapName(id);

	return "";
}
//---------------------------------------------------------------------------
string pyUInterface::getShortName( long id )
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return uniset::ORepHelpers::getShortName(conf->oind->getMapName(id));

	return "";
}
//---------------------------------------------------------------------------
std::string pyUInterface::getTextName( long id )
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->oind->getTextName(id);

	return "";
}
//---------------------------------------------------------------------------
string pyUInterface::getConfFileName()
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->getConfFileName();

	return "";

}
//---------------------------------------------------------------------------
void pyUInterface::uniset_activate_objects()throw(UException)
{
	try
	{
		auto act = uniset::UniSetActivator::Instance();
		act->run(true);
	}
	catch( const std::exception& ex )
	{
		throw UException("(uniset_activate_objects): catch " + std::string(ex.what()) );
	}
}
//---------------------------------------------------------------------------

