/*! $Id*/
//--------------------------------------------------------------------------------
#include "SMViewer.h"
#include "Extentions.h"
//--------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtentions;
//--------------------------------------------------------------------------------
SMViewer::SMViewer( UniSetTypes::ObjectId shmID ):
	SViewer(conf->getControllersSection(),true)
{
	shm = new SMInterface(shmID,&ui,DefaultObjectId,0);
}
// --------------------------------------------------------------------------
SMViewer::~SMViewer()
{
	delete shm;
}
// --------------------------------------------------------------------------
void SMViewer::run()
{
	IOController_i::DSensorInfoSeq_var dmap = shm->getDigitalSensorsMap();
	IOController_i::ASensorInfoSeq_var amap = shm->getAnalogSensorsMap();
	IONotifyController_i::ThresholdsListSeq_var tlst = shm->getThresholdsList();
	try
	{ updateDSensors(dmap,getSharedMemoryID());
	}catch(...){};

	try
	{ updateASensors(amap,getSharedMemoryID());
	}catch(...){}

	try
	{ updateThresholds(tlst,getSharedMemoryID());
	}catch(...){}
}
// --------------------------------------------------------------------------
