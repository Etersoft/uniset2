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
// -----------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "ObjectIndex_XML.h"
#include "ORepHelpers.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------------------
ObjectIndex_XML::ObjectIndex_XML(const string& xmlfile, size_t minSize )
{
	omap.reserve(minSize);

	shared_ptr<UniXML> xml = make_shared<UniXML>();
	//    try
	//    {
	xml->open(xmlfile);
	build(xml);
	//    }
	//    catch(...){}
}
// -----------------------------------------------------------------------------------------
ObjectIndex_XML::ObjectIndex_XML(const std::shared_ptr<UniXML>& xml, size_t minSize )
{
	omap.reserve(minSize);
	build(xml);
}
// -----------------------------------------------------------------------------------------
ObjectIndex_XML::~ObjectIndex_XML()
{
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex_XML::getIdByName( const string& name ) const noexcept
{
	try
	{
		auto it = mok.find(name);

		if( it != mok.end() )
			return it->second;
	}
	catch(...) {}

	return DefaultObjectId;
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_XML::getMapName( const ObjectId id ) const noexcept
{
	if( (unsigned)id < omap.size() && (unsigned)id > 0 )
		return omap[id].repName;

	return "";
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_XML::getTextName( const ObjectId id ) const noexcept
{
	if( (unsigned)id < omap.size() && (unsigned)id > 0 )
		return omap[id].textName;

	return "";
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, ObjectIndex_XML& oi )
{
	return oi.printMap(os);
}
// -----------------------------------------------------------------------------------------
std::ostream& ObjectIndex_XML::printMap( std::ostream& os ) const noexcept
{
	os << "size: " << omap.size() << endl;

	for( auto it = omap.begin(); it != omap.end(); ++it )
	{
		if( it->repName.empty() )
			continue;

		os  << setw(5) << it->id << "  "
			//            << setw(45) << ORepHelpers::getShortName(it->repName,'/')
			<< setw(45) << it->repName
			<< "  " << it->textName << endl;
	}

	return os;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex_XML::build( const std::shared_ptr<UniXML>& xml )
{
	// выделяем память
	//    ObjectInfo* omap = new ObjectInfo[maxSize];
	size_t ind = 1;
	ind = read_section(xml, "sensors", ind);
	ind = read_section(xml, "objects", ind);
	ind = read_section(xml, "controllers", ind);
	ind = read_section(xml, "services", ind);
	ind = read_nodes(xml, "nodes", ind);

	//
	omap.resize(ind);
	omap.shrink_to_fit();
	//    omap[ind].repName=NULL;
	//    omap[ind].textName=NULL;
	//    omap[ind].id = ind;
}
// ------------------------------------------------------------------------------------------
size_t ObjectIndex_XML::read_section( const std::shared_ptr<UniXML>& xml, const std::string& sec, size_t ind )
{
	if( ind >= omap.size() )
	{
		uwarn << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size()
			  << "... Делаем resize + 100" << endl;

		omap.resize(omap.size() + 100);
	}

	string secRoot = xml->getProp( xml->findNode(xml->getFirstNode(), "RootSection"), "name");

	if( secRoot.empty() )
	{
		ostringstream msg;
		msg << "(ObjectIndex_XML::build):: не нашли параметр RootSection в конф. файле ";
		ucrit << msg.str() << endl;
		throw SystemError(msg.str());
	}

	xmlNode* root( xml->findNode(xml->getFirstNode(), sec) );

	if( !root )
	{
		ostringstream msg;
		msg << "(ObjectIndex_XML::build): не нашли корневого раздела " << sec;
		ucrit << msg.str() << endl;
		throw NameNotFound(msg.str());
	}

	// Считываем список элементов
	UniXML::iterator it(root);

	if( !it.goChildren() )
	{
		ostringstream msg;
		msg << "(ObjectIndex_XML::build): не удалось перейти к списку элементов " << sec;
		ucrit << msg.str() << endl;
		throw NameNotFound(msg.str());
	}

	string secname = xml->getProp(root, "section");

	if( secname.empty() )
		secname = xml->getProp(root, "name");

	if( secname.empty() )
	{
		ostringstream msg;
		msg << "(ObjectIndex_XML::build): у секции " << sec << " не указано свойство 'name' ";
		ucrit << msg.str() << endl;
		throw NameNotFound(msg.str());
	}

	// прибавим корень
	secname = secRoot + "/" + secname + "/";

	for( ; it.getCurrent(); it.goNext() )
	{
		omap[ind].id = ind;

		// name
		ostringstream n;
		n << secname << xml->getProp(it, "name");
		const string name(n.str());
		omap[ind].repName = name;

		// mok
		mok[name] = ind; // mok[omap[ind].repName] = ind;

		// textname
		string textname(xml->getProp(it, "textname"));

		if( textname.empty() )
			textname = xml->getProp(it, "name");

		omap[ind].textName = textname;

		omap[ind].xmlnode = it;

		//        cout << "read: " << "(" << ind << ") " << omap[ind].repName << "\t" << omap[ind].textName << endl;
		ind++;

		if( ind >= omap.size() )
		{
			uinfo << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size()
				  << "... Делаем resize + 100" << endl;

			omap.resize(omap.size() + 100);
		}
	}

	return ind;
}
// ------------------------------------------------------------------------------------------
size_t ObjectIndex_XML::read_nodes(const std::shared_ptr<UniXML>& xml, const std::string& sec, size_t ind )
{
	if( ind >= omap.size() )
	{
		uinfo << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size()
			  << "... Делаем resize + 100" << endl;
		omap.resize(omap.size() + 100);
	}

	xmlNode* root( xml->findNode(xml->getFirstNode(), sec) );

	if( !root )
	{
		ostringstream msg;
		msg << "(ObjectIndex_XML::build): не нашли корневого раздела " << sec;
		throw NameNotFound(msg.str());
	}

	// Считываем список элементов
	UniXML::iterator it(root);

	if( !it.goChildren() )
	{
		ostringstream msg;
		msg << "(ObjectIndex_XML::build): не удалось перейти к списку элементов " << sec;
		throw NameNotFound(msg.str());
	}

	//    string secname = xml->getProp(root,"section");

	for( ; it.getCurrent(); it.goNext() )
	{
		omap[ind].id = ind;
		string nodename(xml->getProp(it, "name"));

		omap[ind].repName = nodename;

		// textname
		string textname(xml->getProp(it, "textname"));

		if( textname.empty() )
			textname = nodename;

		omap[ind].textName = textname;
		omap[ind].xmlnode = it;
		//
		mok[omap[ind].repName] = ind;

		//        cout << "read: " << "(" << ind << ") " << omap[ind].repName << "\t" << omap[ind].textName << endl;
		ind++;

		if( (unsigned)ind >= omap.size() )
		{
			ostringstream msg;
			msg << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size();
			uwarn << msg.str() << "... Делаем resize + 100" << endl;
			omap.resize(omap.size() + 100);
		}
	}

	return ind;
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_XML::getObjectInfo( const ObjectId id ) const noexcept
{
	if( (unsigned)id < omap.size() && (unsigned)id > 0 )
		return &omap[id];

	return nullptr;
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_XML::getObjectInfo( const std::string& name ) const noexcept
{
	try
	{
		auto it = mok.find(name);

		if( it != mok.end() )
			return &(omap[it->second]);
	}
	catch(...) {}

	return nullptr;
}
// ------------------------------------------------------------------------------------------
