/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include "SMViewer.h"
#include "Extensions.h"
//--------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
//--------------------------------------------------------------------------------
SMViewer::SMViewer( uniset::ObjectId shmID ):
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
