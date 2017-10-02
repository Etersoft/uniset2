#include <memory>
#include "Configuration.h"
#include "IOConfig_XML.h"
#include "NullSM.h"
// --------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------------

NullSM::NullSM( ObjectId id, const std::string& datfile ):
	IONotifyController(id, static_pointer_cast<IOConfig>(make_shared<IOConfig_XML>(datfile, uniset_conf())) )
{
}
// --------------------------------------------------------------------------------
NullSM::~NullSM()
{
}
// --------------------------------------------------------------------------------
