/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#include "ORepHelpers.h"
#include "Configuration.h"
#include "ObjectIndex_hashXML.h"
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------------------
    ObjectIndex_hashXML::ObjectIndex_hashXML( const string& xmlfile )
    {
        auto xml = make_shared<UniXML>();
        //    try
        //    {
        xml->open(xmlfile);
        build(xml);
        //    }
        //    catch(...){}
    }
    // -----------------------------------------------------------------------------------------
    ObjectIndex_hashXML::ObjectIndex_hashXML( const shared_ptr<UniXML>& xml )
    {
        build(xml);
    }
    // -----------------------------------------------------------------------------------------
    ObjectIndex_hashXML::~ObjectIndex_hashXML()
    {
    }
    // -----------------------------------------------------------------------------------------
    ObjectId ObjectIndex_hashXML::getIdByName( const string& name ) const noexcept
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
    string ObjectIndex_hashXML::getMapName( const ObjectId id ) const noexcept
    {
        try
        {
            auto it = omap.find(id);

            if( it != omap.end() )
                return it->second.repName;
        }
        catch(...) {}

        return "";
    }
    // -----------------------------------------------------------------------------------------
    string ObjectIndex_hashXML::getTextName( const ObjectId id ) const noexcept
    {
        try
        {
            auto it = omap.find(id);

            if( it != omap.end() )
                return it->second.textName;
        }
        catch(...) {}

        return "";
    }
    // -----------------------------------------------------------------------------------------
    std::ostream& operator<<(std::ostream& os, ObjectIndex_hashXML& oi )
    {
        return oi.printMap(os);
    }
    // -----------------------------------------------------------------------------------------
    std::ostream& ObjectIndex_hashXML::printMap( std::ostream& os ) const noexcept
    {
        os << "size: " << omap.size() << endl;

        for( auto it = omap.begin(); it != omap.end(); ++it )
        {
            if( it->second.repName.empty() )
                continue;

            os  << setw(5) << it->second.id << "  "
                //            << setw(45) << ORepHelpers::getShortName(it->repName,'/')
                << setw(45) << it->second.repName
                << "  " << it->second.textName << endl;
        }

        return os;
    }
    // -----------------------------------------------------------------------------------------
    void ObjectIndex_hashXML::build( const shared_ptr<UniXML>& xml )
    {
        read_section(xml, "sensors");
        read_section(xml, "objects");
        read_section(xml, "controllers");
        read_section(xml, "services");
        read_nodes(xml, "nodes");
    }
    // ------------------------------------------------------------------------------------------
    void ObjectIndex_hashXML::read_section( const std::shared_ptr<UniXML>& xml, const std::string& sec )
    {
        string secRoot = xml->getProp( xml->findNode(xml->getFirstNode(), "RootSection"), "name");

        if( secRoot.empty() )
        {
            ostringstream msg;
            msg << "(ObjectIndex_hashXML::build):: не нашли параметр RootSection в конф. файле ";
            ucrit << msg.str() << endl;
            throw SystemError(msg.str());
        }

        xmlNode* root( xml->findNode(xml->getFirstNode(), sec) );

        if( !root )
        {
            ostringstream msg;
            msg << "(ObjectIndex_hashXML::build): не нашли корневого раздела " << sec;
            throw NameNotFound(msg.str());
        }

        // Считываем список элементов
        UniXML::iterator it(root);

        if( !it.goChildren() )
        {
            ostringstream msg;
            msg << "(ObjectIndex_hashXML::build): не удалось перейти к списку элементов " << sec;
            throw NameNotFound(msg.str());
        }

        string secname = xml->getProp(root, "section");

        if( secname.empty() )
            secname = xml->getProp(root, "name");

        if( secname.empty() )
        {
            ostringstream msg;
            msg << "(ObjectIndex_hashXML::build): у секции " << sec << " не указано свойство 'name' ";
            throw NameNotFound(msg.str());
        }

        // прибавим корень
        secname = secRoot + "/" + secname + "/";

        for( ; it.getCurrent(); it.goNext() )
        {
            if( !it.getProp("id").empty() )
            {
                ostringstream err;
                err << "(ObjectIndex_hashXML): ERROR in " << xml->getFileName() << " format: Don`t use id='xxx' or use flag 'idfromfile' in <ObjectsMap idfromfile='1'>";
                throw uniset::SystemError(err.str());
            }

            ObjectInfo inf;

            const std::string nm = it.getProp("name");

            if( nm.empty() )
            {
                ostringstream err;
                err << "Unknown name fot item (section '" << secname << "'). User name='xxx'";
                throw uniset::SystemError(err.str());
            }

            inf.id = uniset::hash32(nm);

            // name
            ostringstream n;
            n << secname << nm;
            const string name(n.str());
            inf.repName = name;

            string textname(xml->getProp(it, "textname"));

            if( textname.empty() )
                textname = nm;

            inf.textName = textname;
            inf.xmlnode = it;

            auto mret = mok.emplace(name, inf.id);

            if( !mret.second )
            {
                ostringstream err;
                err << "Name collision. The '" << name << "' already exists.";
                throw uniset::SystemError(err.str());
            }

            auto ret = omap.emplace(inf.id, std::move(inf));

            if( !ret.second )
            {
                auto coll = omap[inf.id];
                ostringstream err;
                err << "ID '" << nm << "' has collision with '" << coll.name << "'";
                throw uniset::SystemError(err.str());
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    void ObjectIndex_hashXML::read_nodes( const std::shared_ptr<UniXML>& xml, const std::string& sec )
    {
        xmlNode* root( xml->findNode(xml->getFirstNode(), sec) );

        if( !root )
        {
            ostringstream msg;
            msg << "(ObjectIndex_hashXML::build): не нашли корневого раздела " << sec;
            throw NameNotFound(msg.str());
        }

        // Считываем список элементов
        UniXML::iterator it(root);

        if( !it.goChildren() )
        {
            ostringstream msg;
            msg << "(ObjectIndex_hashXML::build): не удалось перейти к списку элементов "
                << " секция " << sec;
            throw NameNotFound(msg.str());
        }

        for( ; it.getCurrent(); it.goNext() )
        {
            ObjectInfo inf;

            if( !it.getProp("id").empty() )
            {
                ostringstream err;
                err << "(ObjectIndex_hashXML): ERROR in " << xml->getFileName() << " format: Don`t use id='xxx' or use flag 'idfromfile' in <ObjectsMap idfromfile='1'>";
                throw uniset::SystemError(err.str());
            }

            const string name(it.getProp("name"));
            inf.id = uniset::hash32(name);

            inf.repName = name;

            // textname
            string textname(xml->getProp(it, "textname"));

            if( textname.empty() )
                textname = name;

            inf.textName = textname;
            inf.xmlnode = it;

            auto ret = omap.emplace(inf.id, inf);

            if( !ret.second )
            {
                auto coll = omap[inf.id];
                ostringstream err;
                err << "NodeID '" << name << "' has collision with '" << coll.name << "'";
                throw uniset::SystemError(err.str());
            }

            auto mret = mok.emplace(name, inf.id);

            if( !mret.second )
            {
                ostringstream err;
                err << "Node name collision. The '" << name << "' already exists.";
                throw uniset::SystemError(err.str());
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    const ObjectInfo* ObjectIndex_hashXML::getObjectInfo( const ObjectId id ) const noexcept
    {
        try
        {
            auto it = omap.find(id);

            if( it != omap.end() )
                return &(it->second);
        }
        catch(...) {}

        return nullptr;
    }
    // ------------------------------------------------------------------------------------------
    const ObjectInfo* ObjectIndex_hashXML::getObjectInfo( const std::string& name ) const noexcept
    {
        try
        {
            auto it = mok.find(name);

            if( it != mok.end() )
                return getObjectInfo(it->second);
        }
        catch(...) {}

        return nullptr;
    }
    // ------------------------------------------------------------------------------------------
} // end of namespace uniset
// ------------------------------------------------------------------------------------------
