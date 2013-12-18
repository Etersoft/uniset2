// -----------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "ORepHelpers.h"
#include "Configuration.h"
#include "ObjectIndex_idXML.h"
// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------------------
ObjectIndex_idXML::ObjectIndex_idXML( const string& xmlfile )
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
ObjectIndex_idXML::ObjectIndex_idXML( UniXML& xml )
{
    build(xml);
}
// -----------------------------------------------------------------------------------------
ObjectIndex_idXML::~ObjectIndex_idXML()
{
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex_idXML::getIdByName( const string& name )
{
    MapObjectKey::iterator it = mok.find(name);
    if( it != mok.end() )
        return it->second;

    return DefaultObjectId;
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_idXML::getMapName( const ObjectId id )
{
    MapObjects::iterator it = omap.find(id);
    if( it!=omap.end() )
        return it->second.repName;

    return "";
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_idXML::getTextName( const ObjectId id )
{
    MapObjects::iterator it = omap.find(id);
    if( it!=omap.end() )
        return it->second.textName;

    return "";
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, ObjectIndex_idXML& oi )
{
    return oi.printMap(os);
}
// -----------------------------------------------------------------------------------------
std::ostream& ObjectIndex_idXML::printMap( std::ostream& os )
{
    os << "size: " << omap.size() << endl;
    for( MapObjects::iterator it=omap.begin(); it!=omap.end(); ++it )
    {
        if( it->second.repName == NULL )
            continue;

        os  << setw(5) << it->second.id << "  "
//            << setw(45) << ORepHelpers::getShortName(it->repName,'/')
            << setw(45) << it->second.repName
            << "  " << it->second.textName << endl;
    }

    return os;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex_idXML::build(UniXML& xml)
{
    read_section(xml,"sensors");
    read_section(xml,"objects");
    read_section(xml,"controllers");
    read_section(xml,"services");
    read_nodes(xml,"nodes");
}
// ------------------------------------------------------------------------------------------
void ObjectIndex_idXML::read_section( UniXML& xml, const std::string& sec )
{
    string secRoot = xml.getProp( xml.findNode(xml.getFirstNode(),"RootSection"), "name");
    if( secRoot.empty() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build):: не нашли параметр RootSection в конф. файле ";
        ulog.crit() << msg.str() << endl;
        throw SystemError(msg.str());
    }

    xmlNode* root( xml.findNode(xml.getFirstNode(),sec) );
    if( !root )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не нашли корневого раздела " << sec;
        throw NameNotFound(msg.str());
    }

    // Считываем список элементов
    UniXML_iterator it(root);
    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не удалось перейти к списку элементов " << sec;
        throw NameNotFound(msg.str());
    }

    string secname = xml.getProp(root,"section");
    if( secname.empty() )
        secname = xml.getProp(root,"name");

    if( secname.empty() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): у секции " << sec << " не указано свойство 'name' ";
        throw NameNotFound(msg.str());
    }

    // прибавим корень
    secname = secRoot+"/"+secname+"/";

    for( ;it.getCurrent(); it.goNext() )
    {
        ObjectInfo inf;
        inf.id = it.getIntProp("id");

        if( inf.id <= 0 )
        {
            ostringstream msg;
            msg << "(ObjectIndex_idXML::build): НЕ УКАЗАН id для " << it.getProp("name")
                << " секция " << sec;
            throw NameNotFound(msg.str());
        }

        // name
        string name( secname+it.getProp("name") );

        inf.repName = new char[name.size()+1];
        strcpy( inf.repName, name.c_str() );

        // textname
        string textname(xml.getProp(it,"textname"));
        if( textname.empty() )
            textname = xml.getProp(it,"name");

        inf.textName = new char[textname.size()+1];
        strcpy( inf.textName, textname.c_str() );

        inf.data = (void*)(xmlNode*)(it);

        omap.insert(MapObjects::value_type(inf.id,inf));    // omap[inf.id] = inf;
        mok.insert(MapObjectKey::value_type(name,inf.id)); // mok[name] = inf.id;
    }
}
// ------------------------------------------------------------------------------------------
void ObjectIndex_idXML::read_nodes( UniXML& xml, const std::string& sec )
{
    xmlNode* root( xml.findNode(xml.getFirstNode(),sec) );
    if( !root )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не нашли корневого раздела " << sec;
        throw NameNotFound(msg.str());
    }

    // Считываем список элементов
    UniXML_iterator it(root);
    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не удалось перейти к списку элементов "
            << " секция " << sec;
        throw NameNotFound(msg.str());
    }

    string secname = xml.getProp(root,"section");

    for( ;it.getCurrent(); it.goNext() )
    {
        ObjectInfo inf;

        inf.id = it.getIntProp("id");
        if( inf.id <= 0 )
        {
            ostringstream msg;
            msg << "(ObjectIndex_idXML::build): НЕ УКАЗАН id для " << it.getProp("name") << endl;
            throw NameNotFound(msg.str());
        }

        string name(it.getProp("name"));
        string alias(it.getProp("alias"));
        if( alias.empty() )
            alias = name;

        string nodename = mkFullNodeName(name,alias);
        inf.repName = new char[nodename.size()+1];
        strcpy( inf.repName, nodename.c_str() );

        // textname
        string textname(xml.getProp(it,"textname"));
        if( textname.empty() )
            textname = nodename;

        inf.textName = new char[textname.size()+1];
        strcpy( inf.textName, textname.c_str() );

        inf.data = (void*)(xmlNode*)(it);

//        omap[inf.id] = inf;
//        mok[nodename] = inf.id;
        omap.insert(MapObjects::value_type(inf.id,inf));    // omap[inf.id] = inf;
        mok.insert(MapObjectKey::value_type(nodename,inf.id)); // mok[name] = inf.id;
    }
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_idXML::getObjectInfo( const ObjectId id )
{
    MapObjects::iterator it = omap.find(id);
    if( it!=omap.end() )
        return &(it->second);

    return NULL;
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_idXML::getObjectInfo( const std::string& name )
{
    const char* n = name.c_str();
    for( MapObjects::iterator it=omap.begin(); it!=omap.end(); it++ )
    {
          if( !strcmp(it->second.repName,n) )
              return &(it->second);
    }

    return NULL;
}
// ------------------------------------------------------------------------------------------
