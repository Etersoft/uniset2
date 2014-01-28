#include "Configuration.h"
#include "NCRestorer.h"
#include "NullController.h"
#include "PassiveTimer.h"
// --------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------------

NullController::NullController( ObjectId id, const string& askdump,
                                const std::string& s_filterField,
                                const std::string& s_filterValue,
                                const std::string& c_filterField,
                                const std::string& c_filterValue,
                                bool _dumpingToDB ):
IONotifyController(id),
dumpingToDB(_dumpingToDB)
{
    restorer = NULL;

    NCRestorer_XML* askd = new NCRestorer_XML(askdump);
    askd->setItemFilter(s_filterField, s_filterValue);
    askd->setConsumerFilter(c_filterField, c_filterValue);

    restorer = askd;

/*
//    askd->setReadItem( sigc::mem_fun(this,&NullController::readSItem) );
    askd->setNCReadItem( sigc::mem_fun(this,&NullController::readSItem) );
    askd->setReadThresholdItem( sigc::mem_fun(this,&NullController::readTItem) );
    askd->setReadConsumerItem( sigc::mem_fun(this,&NullController::readCItem) );
*/
}

// --------------------------------------------------------------------------------

NullController::~NullController()
{
    if( restorer != NULL )
    {
        delete restorer;
        restorer=NULL;
    }
}

// --------------------------------------------------------------------------------
void NullController::dumpToDB()
{
    if( dumpingToDB )
        IONotifyController::dumpToDB();
}
// --------------------------------------------------------------------------------
/*
//bool NullController::readSItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
bool NullController::readSItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec, NCRestorer::SInfo& inf )
{
    cout << "******************* (readSItem): sec=" << sec->name << " it=" << it.getProp("name") << endl;

    inf.default_val = 1;
    return true;
}
// --------------------------------------------------------------------------------
bool NullController::readTItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
    cout << "******************* (readTItem): sec=" << sec->name << " it=" << it.getProp("name") << endl;
    return true;
}
// --------------------------------------------------------------------------------
bool NullController::readCItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
    cout << "******************* (readCItem): sec=" << sec->name << " it=" << it.getProp("name") << endl;
    return true;
}
*/
// --------------------------------------------------------------------------------
