#include <memory>
#include "Configuration.h"
#include "IOConfig_XML.h"
#include "NullSM.h"
#include "PassiveTimer.h"
// --------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------------

NullSM::NullSM( ObjectId id, const std::string& datfile ):
    IONotifyController(id)
{
    auto r = make_shared<IOConfig_XML>(datfile, uniset_conf());
    restorer = std::static_pointer_cast<IOConfig>(r);
}
// --------------------------------------------------------------------------------
NullSM::~NullSM()
{
}
// --------------------------------------------------------------------------------
