/*! \file
 *  \brief Класс работы с сообщениями
 *  \author Pavel Vainerman
 *  \date   $Date: 2008/11/22 23:22:24 $
 *  \version $Id: MessageInterface_idXML.h,v 1.5 2008/11/22 23:22:24 vpashka Exp $
 */
/**************************************************************************/
#ifndef MessageInterface_idXML_H_
#define MessageInterface_idXML_H_
// -----------------------------------------------------------------------------------------
#include <map>
#include <string>
#include "UniSetTypes.h"
#include "MessageInterface.h"
#include "UniXML.h"
// -----------------------------------------------------------------------------------------
class MessageInterface_idXML:
	public	MessageInterface
{
	public:
		MessageInterface_idXML( const std::string xmlfile );
		MessageInterface_idXML( UniXML& xml );
		virtual ~MessageInterface_idXML();

		virtual	std::string getMessage( UniSetTypes::MessageCode code );
		virtual	bool isExist( UniSetTypes::MessageCode code );

		virtual	UniSetTypes::MessageCode getCode( const std::string& msg );
		virtual	UniSetTypes::MessageCode getCodeByIdName( const std::string& name );

		virtual std::ostream& printMessagesMap(std::ostream& os);
		friend std::ostream& operator<<(std::ostream& os, MessageInterface_idXML& mi );
	
	protected:
		void build( UniXML& xml );

	private:
		typedef std::map<UniSetTypes::MessageCode, UniSetTypes::MessageInfo> MapMessageKey;
		MapMessageKey mmk;
};
// -----------------------------------------------------------------------------------------
#endif
