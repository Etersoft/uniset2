// This file is part of the UniSet project.

/* This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov <lav@etersoft.ru>
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
/*! 
    \todo Добавить проверку на предельный номер id 
*/
// -----------------------------------------------------------------------------------------
#include "ObjectIndex.h"
#include "ORepHelpers.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
// -----------------------------------------------------------------------------------------
const std::string ObjectIndex::sepName = "@";
const std::string ObjectIndex::sepNode = ":";
// -----------------------------------------------------------------------------------------

string ObjectIndex::getNameById( const ObjectId id )
{
    string n(getMapName(id));
    if( n.empty() )
        return "";

    return mkRepName(n,nmLocalNode);
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::getNameById( const ObjectId id, const ObjectId node )
{
    const string t(getMapName(id));
    if( t.empty() )
        return "";
    
    // оптимизация
    if( node == conf->getLocalNode() )
        return mkRepName(t,nmLocalNode);
    
    return mkRepName(t,getMapName(node));
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::mkRepName( const std::string& repname, const std::string& nodename )
{
    return repname + sepName + nodename; 
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::mkFullNodeName( const std::string& realnode, const std::string& virtnode )
{
    // realnode справа, что поиск и вырезание происходили быстрее
    // эта функция часто используется...
    return virtnode + sepNode + realnode; 
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getBaseName( const std::string& fname )
{
    string::size_type pos = fname.rfind('/');
    if( pos != string::npos )
        return fname.substr(pos+1);
    
    return fname;
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::getFullNodeName( const string& oname )
{
    string::size_type pos = oname.rfind(sepName);
    if( pos != string::npos )
        return oname.substr(pos+1);

    // Если не нашли разделитель name@vnode:rnode
    // то пытаемся найти в данной строке сочетание vnode:rnode
    string vnode = getVirtualNodeName(oname);
    string rnode = getRealNodeName(oname);
    if( !rnode.empty() )
    {
        if( vnode.empty() )
            vnode = rnode;
        return mkFullNodeName(rnode,vnode);
    }

    // Если не нашли, то возвращаем, полное имя 
    // ЛОКАЛЬНОГО узла...(для оптимизации хранимого в классе)
    if( !nmLocalNode.empty() )
        return mkFullNodeName(nmLocalNode,nmLocalNode);
    
    nmLocalNode = getMapName(UniSetTypes::conf->getLocalNode());
    return mkFullNodeName(nmLocalNode,nmLocalNode);
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::getVirtualNodeName( const string& fname )
{
    string::size_type pos1 = fname.rfind(sepName);
    string::size_type pos2 = fname.rfind(sepNode);

    if( pos2 == string::npos )
        return "";
    
    if( pos1!=string::npos )
        return fname.substr( pos1+1, pos2-pos1-1 );

    return "";
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::getRealNodeName( const string& fname )
{
    string::size_type pos = fname.rfind(sepNode);
    if( pos != string::npos )
        return fname.substr(pos+1); 

    if( !nmLocalNode.empty() )
        return nmLocalNode;
    
    nmLocalNode = getMapName(UniSetTypes::conf->getLocalNode());
    return nmLocalNode;
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getRealNodeName( const ObjectId id )
{
    string n(getMapName(id));
    if( n.empty() )
        return "";
        
    return getRealNodeName(n);
}
// -----------------------------------------------------------------------------------------
string ObjectIndex::getName( const string& fname )
{
    string::size_type pos = fname.rfind(sepName);
    if( pos != string::npos )
        return fname.substr(0,pos); 

    return "";
}
// -----------------------------------------------------------------------------------------
void ObjectIndex::initLocalNode( ObjectId nodeid )
{
    nmLocalNode = getMapName(nodeid);
}
// -----------------------------------------------------------------------------------------
