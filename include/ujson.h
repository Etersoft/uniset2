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
#ifndef jsonHelpers_H_
#define jsonHelpers_H_
// --------------------------------------------------------------------------
#ifndef DISABLE_REST_API
#include <Poco/JSON/Object.h>
// --------------------------------------------------------------------------
namespace uniset
{
namespace json
{
Poco::JSON::Object::Ptr make_object( const std::string& key, const Poco::Dynamic::Var& val );
Poco::JSON::Object::Ptr make_object( const std::string& key, const Poco::Dynamic::Var&& val );
Poco::JSON::Object::Ptr make_child( Poco::JSON::Object::Ptr& root, const std::string& key );
Poco::JSON::Array::Ptr make_child_array( Poco::JSON::Object::Ptr& root, const std::string& key );

namespace help
{

class item
{
	public:

		item( Poco::JSON::Object::Ptr& ptr );
		item( const std::string& description );

		void param(const std::string& name, const std::string& description );
		Poco::JSON::Object::Ptr get();

		operator Poco::JSON::Object::Ptr();
		operator Poco::Dynamic::Var();

	private:
		Poco::JSON::Object::Ptr root;
		Poco::JSON::Array::Ptr params;
};

class object
{
	public:
		object( const std::string& name );
		object( const std::string& name, Poco::JSON::Object::Ptr ptr );

		void add( item& i );

		Poco::JSON::Object::Ptr get();

		operator Poco::JSON::Object::Ptr();
		operator Poco::Dynamic::Var();

	private:
		Poco::JSON::Object::Ptr root;
		Poco::JSON::Array::Ptr cmdlist;
};

} // end of namespace help
} // end of namespace json
// --------------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif // end of #ifndef DISABLE_REST_API
// --------------------------------------------------------------------------
#endif // end of _jsonHelpers_H_
