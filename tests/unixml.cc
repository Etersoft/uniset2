#include <iostream>

using namespace std;

#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"

int main()
{
/*    try
    {
*/
        UniXML xml("test.xml");

        xmlNode* cnode = xml.findNode(xml.getFirstNode(),"UniSet");
        if( cnode == NULL )
        {
            cerr << "<testnode> not found" << endl;
            return 1;
        }
        UniXML_iterator it(cnode);

        cout << "string id=" << it.getProp("id")
             << " int id=" << it.getIntProp("id")
             << endl;

        {
        UniXML_iterator it = xml.findNode(xml.getFirstNode(),"LocalInfoServer", "InfoServer");
        if( it == NULL )
        {
            cerr << "<testnode> not found" << endl;
            return 1;
        }

        cout << "string id=" << it.getProp("dbrepeat")
             << " int id=" << it.getIntProp("dbrepeat")
             << endl;
        }

                {
        xmlNode* cnode = xml.findNode(xml.getFirstNode(),"ObjectsMap");
        if( cnode == NULL )
        {
            cerr << "<testnode> not found" << endl;
            return 1;
        }
        UniXML_iterator it = xml.findNode(xml.getFirstNode(),"item", "LocalhostNode");
        if( it == NULL )
        {
            cerr << "<testnode> not found" << endl;
            return 1;
        }
        cout << "textname=" << it.getProp("textname");
                }

        xml.save("test.out.xml");
/*
    }
    catch( UniSetTypes::Exception& ex )
    {
        cerr << ex << endl;
        return 1;
    }
    catch( ... )
    {
        cerr << "catch ... " << endl;
        return 1;
    }
*/
    return 0;
}
