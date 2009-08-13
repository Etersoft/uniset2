#include <iostream>

using namespace std;

#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"

int main()
{
	try
	{
		UniXML xml("test.xml");
		
		xmlNode* cnode = xml.findNode(xml.getFirstNode(),"testnode");
		if( !cnode )
		{	
			cerr << "<testnode> not found" << endl;
			return 1;
		} 

		UniXML_iterator it(cnode);
		cout << "string id=" << it.getProp("id")
			 << " int id=" << it.getIntProp("id")
			 << endl;

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
	
	return 0;
}
