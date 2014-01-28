#include <sstream>
#include <iostream>
#include "UniXML.h"
#include "Schema.h"
#include "TDelay.h"

// -------------------------------------------------------------------------
using namespace std;

// -------------------------------------------------------------------------
SchemaXML::SchemaXML()
{
}

SchemaXML::~SchemaXML()
{
}
// -------------------------------------------------------------------------
void SchemaXML::read( const string& xmlfile )
{
    UniXML xml;
    
    const string sec("elements");
    const string conn_sec("connections");
    
//    try
//    {
        xml.open(xmlfile);
//    }
//    catch(...){}
    
    xmlNode* root( xml.findNode(xml.getFirstNode(),sec) );
    if( !root )
    {
        ostringstream msg;
        msg << "(SchemaXML::read): не нашли корневого раздела " << sec;
        throw LogicException(msg.str());
    }

    // Считываем список элементов
    UniXML_iterator it(root);
    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(SchemaXML::read): не удалось перейти к списку элементов " << sec;
        throw LogicException(msg.str());
    }

    for( ;it.getCurrent(); it.goNext() )
    {
        string type(xml.getProp(it, "type"));
        string ID(xml.getProp(it, "id"));
        int inCount = xml.getPIntProp(it, "inCount", 1);
        
        if( type == "OR" )
            manage( new TOR(ID, inCount) );
        else if( type == "AND" )
            manage( new TAND(ID, inCount) );
        else if( type == "Delay" )
        {
            int delayMS = xml.getIntProp(it,"delayMS");
            manage( new TDelay(ID,delayMS,inCount) );
        }
        else if( type == "NOT" )
        {
            bool defout = xml.getIntProp(it,"default_out_state");
            manage( new TNOT(ID,defout) );
        }
        else
        {
            ostringstream msg;
            msg << "(SchemaXML::read): НЕИЗВЕСТНЫЙ ТИП ЭЛЕМЕНТА -->" << type;
            throw LogicException(msg.str());
        }
    }

    // Строим связи
    xmlNode* conNode( xml.findNode(xml.getFirstNode(),conn_sec) );
    if( !conNode )
    {
        ostringstream msg;
        msg << "(SchemaXML::read): не нашли корневого раздела " << conn_sec;
        throw LogicException(msg.str());
    }
    
    it = conNode;
    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(SchemaXML::read): не удалось перейти к списку элементов " << conn_sec;
        throw LogicException(msg.str());
    }

    for( ;it.getCurrent(); it.goNext() )
    {
        string type(xml.getProp(it, "type"));
        string fID(xml.getProp(it, "from"));
        string tID(xml.getProp(it, "to"));
        int toIn = xml.getIntProp(it, "toInput");
        
        if( type == "ext" )
        {
            cout <<"SchemaXML: set EXTlink: from=" << fID << " to=" << tID << " toInput=" << toIn  << endl;
            extlink(fID,tID,toIn);
        }
        else if( type == "int" )
        {
            cout <<"SchemaXML: set INTlink: from=" << fID << " to=" << tID << " toInput=" << toIn  << endl;
            link(fID,tID,toIn);        
        }
        else if( type == "out" )
        {
            Element* el = find(fID);
            if( el== 0 )
            {
                ostringstream msg;
                msg << "(SchemaXML::read): НЕ НАЙДЕН ЭЛЕМЕНТ С ID=" << fID;
                throw LogicException(msg.str());            
            }
        
            cout <<"SchemaXML: set Out: from=" << fID << " to=" << tID << endl;
            outList.push_front( EXTOut(tID,el) );
        }
    }
}
// -------------------------------------------------------------------------
