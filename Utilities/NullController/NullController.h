// --------------------------------------------------------------------------
#ifndef NullController_H_
#define NullController_H_
// --------------------------------------------------------------------------
#include <string>
#include "IONotifyController.h"
#include "NCRestorer.h"
// --------------------------------------------------------------------------
class NullController:
	public IONotifyController
{
	public:
		NullController( UniSetTypes::ObjectId id, const std::string restorfile,
						const std::string s_filterField="",
						const std::string s_filterValue="",
						const std::string c_filterField="",
						const std::string c_filterValue="",
						const std::string d_filterField="",
						const std::string d_filterValue="",
						bool _dumpingToDB=false );

		virtual ~NullController();

	protected:

		virtual void dumpToDB();

//		bool readSItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec, NCRestorer::SInfo& inf );
//		bool readTItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec);
//		bool readCItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec);

	private:
		bool dumpingToDB;

};
// --------------------------------------------------------------------------
#endif // NullController_H_
// --------------------------------------------------------------------------
