#include "Configuration.h"
#include "IOConfig_XML.h"
#include "NullController.h"
#include "PassiveTimer.h"
// --------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------------

NullController::NullController( ObjectId id, const string& ioconfile,
                                const std::string& s_filterField,
                                const std::string& s_filterValue,
                                const std::string& c_filterField,
                                const std::string& c_filterValue,
                                bool _dumpingToDB ):
    IONotifyController(id),
    dumpingToDB(_dumpingToDB)
{
    restorer = NULL;

    auto ioconf = make_shared<IOConfig_XML>(ioconfile, uniset_conf());
    ioconf->setItemFilter(s_filterField, s_filterValue);
    ioconf->setConsumerFilter(c_filterField, c_filterValue);

    restorer = std::static_pointer_cast<IOConfig>(ioconf);

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
}
// --------------------------------------------------------------------------------
void NullController::dumpToDB()
{
    if( dumpingToDB )
        IONotifyController::dumpToDB();
}
// --------------------------------------------------------------------------------
/*
//bool NullController::readSItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec )
bool NullController::readSItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec, NCRestorer::SInfo& inf )
{
    cout << "******************* (readSItem): sec=" << sec->name << " it=" << it.getProp("name") << endl;

    inf.default_val = 1;
    return true;
}
// --------------------------------------------------------------------------------
bool NullController::readTItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec )
{
    cout << "******************* (readTItem): sec=" << sec->name << " it=" << it.getProp("name") << endl;
    return true;
}
// --------------------------------------------------------------------------------
bool NullController::readCItem( UniXML& xml, UniXML::iterator& it, xmlNode* sec )
{
    cout << "******************* (readCItem): sec=" << sec->name << " it=" << it.getProp("name") << endl;
    return true;
}
*/
// --------------------------------------------------------------------------------
