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
#include <mutex>
#include <memory>
#include <ostream>
#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "Configuration.h"
#include "UniSetActivator.h"
#include "UniSetTypes.h"
#include "ujson.h"
#include "goUInterface.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
static std::shared_ptr<uniset::UInterface> uInterface;

static std::mutex& umutex() {
	static std::mutex _mutex;
	return _mutex;
}
//---------------------------------------------------------------------------
UTypes::ResultBool goUInterface::uniset_init_params( UTypes::Params* p, const std::string& xmlfile ) noexcept
{
	return goUInterface::uniset_init(p->argc, p->argv, xmlfile);
}
//---------------------------------------------------------------------------
UTypes::ResultBool goUInterface::uniset_init( int argc, char* argv[], const std::string& xmlfile ) noexcept
{
	std::lock_guard<std::mutex> l(umutex());

	if( uInterface )
		return UTypes::ResultBool(true);

	try
	{
		uniset::uniset_init(argc, argv, xmlfile);
		uInterface = make_shared<uniset::UInterface>();
		return UTypes::ResultBool(true);
	}
	catch( std::exception& ex )
	{
		return UTypes::ResultBool(false,ex.what());
	}

	return UTypes::ResultBool(false,"Unknown error");
}
//---------------------------------------------------------------------------
UTypes::ResultValue goUInterface::getValue( long id ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( !conf || !uInterface )
		return UTypes::ResultValue(0,"USysError");

	using namespace uniset;

	UniversalIO::IOType t = conf->getIOType(id);

	if( t == UniversalIO::UnknownIOType )
	{
		ostringstream e;
		e << "(getValue): Unknown iotype for id=" << id;
		return UTypes::ResultValue(0, e.str());
	}

	try
	{
		return UTypes::ResultValue( uInterface->getValue(id) );
	}
	catch( UException& ex )
	{
		return UTypes::ResultValue(0, ex.err);
	}
	catch( std::exception& ex )
	{
		return UTypes::ResultValue(0, ex.what());
	}

	return UTypes::ResultValue(0, "Unknown error");
}
//---------------------------------------------------------------------------
UTypes::ResultBool goUInterface::setValue( long id, long val, long supplier ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( !conf || !uInterface )
		return UTypes::ResultBool(0,"USysError");

	using namespace uniset;

	UniversalIO::IOType t = conf->getIOType(id);

	if( t == UniversalIO::UnknownIOType )
	{
		ostringstream e;
		e << "(setValue): Unknown iotype for id=" << id;
		return UTypes::ResultBool(0, e.str());
	}

	try
	{
		uInterface->setValue(id, val, conf->getLocalNode(), supplier);
		return UTypes::ResultBool(true);
	}
	catch( UException& ex )
	{
		return UTypes::ResultBool(false, ex.err);
	}
	catch( std::exception& ex )
	{
		return UTypes::ResultBool(false, ex.what());
	}

	return UTypes::ResultBool(false, "Unknown error");
}
//---------------------------------------------------------------------------
long goUInterface::getSensorID(const string& name ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->getSensorID(name);

	return uniset::DefaultObjectId;
}
//---------------------------------------------------------------------------
long goUInterface::getObjectID(const string& name ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->getObjectID(name);

	return uniset::DefaultObjectId;
}
//---------------------------------------------------------------------------
std::string goUInterface::getName( long id ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->oind->getMapName(id);

	return "";
}
//---------------------------------------------------------------------------
string goUInterface::getShortName( long id ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return uniset::ORepHelpers::getShortName(conf->oind->getMapName(id));

	return "";
}
//---------------------------------------------------------------------------
std::string goUInterface::getTextName( long id ) noexcept
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->oind->getTextName(id);

	return "";
}
//---------------------------------------------------------------------------
string goUInterface::getConfFileName() noexcept
{
	auto conf = uniset::uniset_conf();

	if( conf )
		return conf->getConfFileName();

	return "";

}
//---------------------------------------------------------------------------
std::string goUInterface::getConfigParamsByName( const std::string& name , const std::string& section )
{
	auto conf = uniset::uniset_conf();

	if( !conf )
		return "";

	auto xml = conf->getConfXML();

	std::string s( (section.empty() ? name : section) );

	xmlNode* cnode = conf->findNode( xml->getFirstNode(), s, name );

	if( !cnode )
		return "";

	uniset::UniXML::iterator it(cnode);
	auto proplist = it.getPropList();

	Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
	Poco::JSON::Array::Ptr jdata = uniset::json::make_child_array(json, "config");

	for( const auto& p: proplist )
	{
		Poco::JSON::Object::Ptr item = new Poco::JSON::Object();
		item->set("prop", p.first);
		item->set("value", p.second);
		jdata->add(item);
	}

	ostringstream result;
	json->stringify(result);

	// Poco::JSON::Object::Ptr это SharedPtr поэтому вызывать delete не нужно
	return result.str();
}
//---------------------------------------------------------------------------
bool goUInterface::isUniSetInitOK() noexcept
{
	std::lock_guard<std::mutex> l(umutex());
	return uInterface != nullptr;
}
//---------------------------------------------------------------------------
