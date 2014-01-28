/*! \file
 *  \brief Класс работы с сообщениями
 *  \author Pavel Vainerman
 */
/**************************************************************************/
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "MessageInterface_idXML.h"
#include "Configuration.h"
// -----------------------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------------------
MessageInterface_idXML::MessageInterface_idXML( const std::string xmlfile )
{
	UniXML xml;
//	try
//	{
		xml.open(xmlfile);
		build(xml);
//	}
//	catch(...){}

}
// -----------------------------------------------------------------------------------------
MessageInterface_idXML::MessageInterface_idXML( UniXML& xml )
{
	build(xml);
}
// -----------------------------------------------------------------------------------------

MessageInterface_idXML::~MessageInterface_idXML()
{

}
// -----------------------------------------------------------------------------------------
string MessageInterface_idXML::getMessage( UniSetTypes::MessageCode code )
{
	MapMessageKey::const_iterator it = mmk.find(code);
	if( it != mmk.end() )
		return it->second.text;
	
	ostringstream err;
	err  << "Неизвестное сообщение с кодом " << code; 
	return err.str();
}

// -----------------------------------------------------------------------------------------
bool MessageInterface_idXML::isExist(UniSetTypes::MessageCode code)
{
	if( mmk.find(code) != mmk.end() )
		return true;
		
	return false;
}
// -----------------------------------------------------------------------------------------
UniSetTypes::MessageCode MessageInterface_idXML::getCode( const string& msg )
{
	for( MapMessageKey::const_iterator it = mmk.begin(); it!=mmk.end(); ++it )
	{
		if( it->second.text == msg )
			return it->first;
	}
	
	return UniSetTypes::DefaultMessageCode;
}
// -----------------------------------------------------------------------------------------
UniSetTypes::MessageCode MessageInterface_idXML::getCodeByIdName( const string& name )
{
	for( MapMessageKey::const_iterator it = mmk.begin(); it!=mmk.end(); ++it )
	{
		if( it->second.idname == name )
			return it->first;
	}
	
	return UniSetTypes::DefaultMessageCode;
}
// -----------------------------------------------------------------------------------------
void MessageInterface_idXML::build( UniXML& xml )
{
	xmlNode* root = xml.findNode(xml.getFirstNode(),"messages");
	if( !root )
	{
		ostringstream msg;
		msg << "(MessageInterface_idXML::build): не нашли корневого раздела <messages>";
		throw NameNotFound(msg.str());
	}

	// Считываем список элементов
	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		ostringstream msg;
		msg << "(MessageInterface_idXML::build): не удалось перейти к списку элементов <messages>";
		throw NameNotFound(msg.str());
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		MessageInfo inf;

		inf.code = it.getIntProp("id");
		if( inf.code <= 0 )
		{
			ostringstream msg;
			msg << "(MessageInterface_idXML::build): НЕ УКАЗАН id для " << it.getProp("name");
			throw NameNotFound(msg.str());
		}

		inf.idname = it.getProp("name");
		inf.text = it.getProp("text");

		mmk[inf.code] = inf;
	}
}
// -----------------------------------------------------------------------------------------
std::ostream& MessageInterface_idXML::printMessagesMap( std::ostream& os )
{
	cout << "size: " << mmk.size() << endl;
	for( MapMessageKey::const_iterator it = mmk.begin(); it!=mmk.end(); ++it )
	{
		os  << setw(5) << it->second.code << " "
			<< setw(10) << it->second.idname << " "  
			<< setw(45) << it->second.text << endl;
	}

	return os;
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, MessageInterface_idXML& mi )
{
	return mi.printMessagesMap(os);
}
// -----------------------------------------------------------------------------------------
