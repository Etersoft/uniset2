// This file is part of the UniSet project.

/* This file is part of the UniSet project
 * Copyright (C) 2003 Pavel Vainerman <pv@etersoft.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// -----------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "ObjectIndex_XML.h"
#include "ORepHelpers.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------------------
ObjectIndex_XML::ObjectIndex_XML( const string& xmlfile, int minSize ):
    omap(minSize)
{
    UniXML xml;
//    try
//    {
        xml.open(xmlfile);
        build(xml);
//    }
//    catch(...){}
}
// -----------------------------------------------------------------------------------------
ObjectIndex_XML::ObjectIndex_XML( UniXML& xml, int minSize ):
omap(minSize)
{
    build(xml);
}
// -----------------------------------------------------------------------------------------
ObjectIndex_XML::~ObjectIndex_XML()
{
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex_XML::getIdByName( const string& name )
{
    MapObjectKey::iterator it = mok.find(name);
    if( it != mok.end() )
        return it->second;

    return DefaultObjectId;
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_XML::getMapName( const ObjectId id )
{
    if( (unsigned)id<omap.size() && (unsigned)id>=0 )
        return omap[id].repName;

    return "";
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_XML::getTextName( const ObjectId id )
{
    if( (unsigned)id<omap.size() && (unsigned)id>=0 )
        return omap[id].textName;

    return "";
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, ObjectIndex_XML& oi )
{
    return oi.printMap(os);
}
// -----------------------------------------------------------------------------------------
std::ostream& ObjectIndex_XML::printMap( std::ostream& os )
{
    cout << "size: " << omap.size() << endl;
    for( vector<ObjectInfo>::iterator it=omap.begin(); it!=omap.end(); ++it )
    {
        if( it->repName == NULL )
            continue;

        os  << setw(5) << it->id << "  " 
//            << setw(45) << ORepHelpers::getShortName(it->repName,'/') 
            << setw(45) << it->repName 
            << "  " << it->textName << endl;
    }

    return os;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex_XML::build(UniXML& xml)
{
    // выделяем память
//    ObjectInfo* omap = new ObjectInfo[maxSize];
    ObjectId ind=1;
    ind = read_section(xml,"sensors",ind);
    ind = read_section(xml,"objects",ind);
    ind = read_section(xml,"controllers",ind);
    ind = read_section(xml,"services",ind);
    ind = read_nodes(xml,"nodes",ind);
    
    // 
    omap.resize(ind);
//    omap[ind].repName=NULL;
//    omap[ind].textName=NULL;
//    omap[ind].id = ind;
}
// ------------------------------------------------------------------------------------------
unsigned int ObjectIndex_XML::read_section( UniXML& xml, const std::string& sec, unsigned int ind )
{
    if( (unsigned)ind >= omap.size() )
    {
        uwarn << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size()
              << "... Делаем resize + 100" << endl;

        omap.resize(omap.size()+100);
    }

    string secRoot = xml.getProp( xml.findNode(xml.getFirstNode(),"RootSection"), "name");
    if( secRoot.empty() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build):: не нашли параметр RootSection в конф. файле ";
        ucrit << msg.str() << endl;
        throw SystemError(msg.str());
    }

    xmlNode* root( xml.findNode(xml.getFirstNode(),sec) );
    if( !root )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build): не нашли корневого раздела " << sec;
        ucrit << msg.str() << endl;
        throw NameNotFound(msg.str());
    }

    // Считываем список элементов
    UniXML_iterator it(root);
    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build): не удалось перейти к списку элементов " << sec;
        ucrit << msg.str() << endl;
        throw NameNotFound(msg.str());
    }

    string secname = xml.getProp(root,"section");
    if( secname.empty() )
        secname = xml.getProp(root,"name");

    if( secname.empty() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build): у секции " << sec << " не указано свойство 'name' ";
        ucrit << msg.str() << endl;
        throw NameNotFound(msg.str());
    }

    // прибавим корень
    secname = secRoot+"/"+secname + "/";

    for( ;it.getCurrent(); it.goNext() )
    {
        omap[ind].id = ind;

        // name
        const string name(secname + xml.getProp(it,"name"));
        delete[] omap[ind].repName;
        omap[ind].repName = new char[name.size()+1];
        strcpy( omap[ind].repName, name.c_str() );

        // mok
        mok[name] = ind; // mok[omap[ind].repName] = ind;

        // textname
        string textname(xml.getProp(it,"textname"));
        if( textname.empty() )
            textname = xml.getProp(it,"name");

        delete[] omap[ind].textName;
        omap[ind].textName = new char[textname.size()+1];
        strcpy( omap[ind].textName, textname.c_str() );

        omap[ind].data = (void*)(xmlNode*)it;

//        cout << "read: " << "(" << ind << ") " << omap[ind].repName << "\t" << omap[ind].textName << endl;
        ind++;

        if( (unsigned)ind >= omap.size() )
        {
            uinfo << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size()
                  << "... Делаем resize + 100" << endl;

            omap.resize(omap.size()+100);
        }
    }

    return ind;
}
// ------------------------------------------------------------------------------------------
unsigned int ObjectIndex_XML::read_nodes( UniXML& xml, const std::string& sec, unsigned int ind )
{
    if( ind >= omap.size() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size();
        uinfo << msg.str() << "... Делаем resize + 100\n";
        omap.resize(omap.size()+100);
    }

    xmlNode* root( xml.findNode(xml.getFirstNode(),sec) );
    if( !root )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build): не нашли корневого раздела " << sec;
        throw NameNotFound(msg.str());
    }

    // Считываем список элементов
    UniXML_iterator it(root);
    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_XML::build): не удалось перейти к списку элементов " << sec;
        throw NameNotFound(msg.str());
    }

//    string secname = xml.getProp(root,"section");

    for( ;it.getCurrent(); it.goNext() )
    {
        omap[ind].id = ind;
        string nodename(xml.getProp(it,"name"));

        delete[] omap[ind].repName;
        omap[ind].repName = new char[nodename.size()+1];
        strcpy( omap[ind].repName, nodename.c_str() );

        // textname
        string textname(xml.getProp(it,"textname"));
        if( textname.empty() )
            textname = nodename;

        delete[] omap[ind].textName;
        omap[ind].textName = new char[textname.size()+1];
        strcpy( omap[ind].textName, textname.c_str() );

        omap[ind].data = (void*)(xmlNode*)(it);
        // 
        mok[omap[ind].repName] = ind;

//        cout << "read: " << "(" << ind << ") " << omap[ind].repName << "\t" << omap[ind].textName << endl;
        ind++;
        if( (unsigned)ind >= omap.size() )
        {
            ostringstream msg;
            msg << "(ObjectIndex_XML::build): не хватило размера массива maxSize=" << omap.size();
            uwarn << msg.str() << "... Делаем resize + 100" << endl;
            omap.resize(omap.size()+100);
        }
    }

    return ind;
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_XML::getObjectInfo( const ObjectId id )
{
    if( (unsigned)id<omap.size() && (unsigned)id>=0 )
        return &omap[id];

    return NULL;
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_XML::getObjectInfo( const std::string& name )
{
    MapObjectKey::iterator it = mok.find(name);
    if( it != mok.end() )
        return &(omap[it->second]);

    return NULL;
}
// ------------------------------------------------------------------------------------------
