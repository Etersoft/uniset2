#include "SMViewer.h"
#include "Extensions.h"
//--------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
//--------------------------------------------------------------------------------
SMViewer::SMViewer( UniSetTypes::ObjectId shmID ):
	SViewer(uniset_conf()->getControllersSection(), true),
	shm(make_shared<SMInterface>(shmID, ui, DefaultObjectId))
{

}
// --------------------------------------------------------------------------
SMViewer::~SMViewer()
{
}
// --------------------------------------------------------------------------
void SMViewer::run()
{
	IOController_i::SensorInfoSeq_var amap = shm->getSensorsMap();
	IONotifyController_i::ThresholdsListSeq_var tlst = shm->getThresholdsList();

	try
	{
		updateSensors(amap, getSharedMemoryID());
	}
	catch(...) {}

	try
	{
		updateThresholds(tlst, getSharedMemoryID());
	}
	catch(...) {}
}
// --------------------------------------------------------------------------
