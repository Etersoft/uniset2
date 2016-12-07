#include <memory>
#include "Configuration.h"
#include "NCRestorer.h"
#include "NullSM.h"
// --------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------------

NullSM::NullSM( ObjectId id, const std::string& datfile ):
	IONotifyController(id)
{
	shared_ptr<NCRestorer_XML> r = make_shared<NCRestorer_XML>(datfile);
	restorer = std::static_pointer_cast<NCRestorer>(r);
}
// --------------------------------------------------------------------------------
NullSM::~NullSM()
{
}
// --------------------------------------------------------------------------------
