// --------------------------------------------------------------------------
#ifndef NullController_H_
#define NullController_H_
// --------------------------------------------------------------------------
#include <string>
#include "IONotifyController.h"
// --------------------------------------------------------------------------
class NullController:
	public uniset::IONotifyController
{
	public:
		NullController(uniset::ObjectId id, const std::string& ioconfile,
					   const std::string& s_filterField = "",
					   const std::string& s_filterValue = "",
					   const std::string& c_filterField = "",
					   const std::string& c_filterValue = "",
					   bool _dumpingToDB = false );

		virtual ~NullController();

	protected:

		virtual void dumpToDB();

		//        bool readSItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec, NCRestorer::SInfo& inf );
		//        bool readTItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec);
		//        bool readCItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec);

	private:
		bool dumpingToDB;

};
// --------------------------------------------------------------------------
#endif // NullController_H_
// --------------------------------------------------------------------------
