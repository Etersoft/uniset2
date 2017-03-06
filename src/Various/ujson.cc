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
#ifndef DISABLE_REST_API
// --------------------------------------------------------------------------
#include "ujson.h"
// --------------------------------------------------------------------------
namespace uniset
{
	// --------------------------------------------------------------------------
	Poco::JSON::Object::Ptr json::make_object( const std::string& key, const Poco::Dynamic::Var& val )
	{
		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
		j->set(key, val);
		return j;
	}
	// --------------------------------------------------------------------------
	Poco::JSON::Object::Ptr json::make_object( const std::string& key, const Poco::Dynamic::Var&& val )
	{
		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
		j->set(key, std::move(val));
		return j;
	}
	// --------------------------------------------------------------------------
	json::help::item::item( Poco::JSON::Object::Ptr& ptr )
	{
		root = ptr;
		params = ptr->getArray("parameters");
	}
	// --------------------------------------------------------------------------
	json::help::item::item(const std::string& description)
	{
		root = new Poco::JSON::Object();
		root->set("description", description);
	}
	// --------------------------------------------------------------------------
	void json::help::item::param(const std::string& name, const std::string& description)
	{
		if( !params )
		{
			params = new Poco::JSON::Array();
			root->set("parameters", params);
		}

		params->add( uniset::json::make_object(name, description) );
	}
	// --------------------------------------------------------------------------
	Poco::JSON::Object::Ptr json::help::item::get()
	{
		return root;
	}
	// --------------------------------------------------------------------------
	uniset::json::help::item::operator Poco::Dynamic::Var()
	{
		return root;
	}
	// --------------------------------------------------------------------------
	uniset::json::help::item::operator Poco::JSON::Object::Ptr()
	{
		return root;
	}
	// --------------------------------------------------------------------------
	json::help::object::object( const std::string& name )
	{
		root = new Poco::JSON::Object();
		cmdlist = new Poco::JSON::Array();
		root->set(name, cmdlist);
	}
	// --------------------------------------------------------------------------
	json::help::object::object(const std::string& name, Poco::JSON::Object::Ptr ptr )
	{
		root = ptr;
		cmdlist = root->getArray(name);

		if( !cmdlist )
		{
			cmdlist = new Poco::JSON::Array();
			root->set(name, cmdlist);
		}
	}
	// --------------------------------------------------------------------------
	void json::help::object::add(json::help::item& i)
	{
		cmdlist->add(i);
	}
	// --------------------------------------------------------------------------
	Poco::JSON::Object::Ptr json::help::object::get()
	{
		return root;
	}
	// --------------------------------------------------------------------------
	uniset::json::help::object::operator Poco::Dynamic::Var()
	{
		return root;
	}
	// --------------------------------------------------------------------------
	uniset::json::help::object::operator Poco::JSON::Object::Ptr()
	{
		return root;
	}
	// --------------------------------------------------------------------------
	Poco::JSON::Object::Ptr json::make_child( Poco::JSON::Object::Ptr& root, const std::string& key )
	{
		Poco::JSON::Object::Ptr child = new Poco::JSON::Object();
		root->set(key, child);
		return child;
	}
	// --------------------------------------------------------------------------
	Poco::JSON::Array::Ptr json::make_child_array(Poco::JSON::Object::Ptr& root, const std::string& key)
	{
		Poco::JSON::Array::Ptr child = new Poco::JSON::Array();
		root->set(key, child);
		return child;
	}

	// --------------------------------------------------------------------------
	// --------------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
