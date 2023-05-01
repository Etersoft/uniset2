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
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "SMInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
#define BEG_FUNC(name) \
    try \
    {     \
        auto conf = ui->getConf(); \
        uniset_rwmutex_wrlock l(shmMutex); \
        IONotifyController_i_var shm;\
        for( unsigned int i=0; i<conf->getRepeatCount(); i++)\
        {\
            try\
            {\
                if( CORBA::is_nil(oref) ) \
                    oref = ui->resolve( shmID, conf->getLocalNode() ); \
                \
                if( CORBA::is_nil(oref) ) \
                    continue;\
                \
                shm = IONotifyController_i::_narrow(oref); \
                if( CORBA::is_nil(shm) ) \
                { \
                    oref = CORBA::Object::_nil();\
                    msleep(conf->getRepeatTimeout());    \
                    continue;\
                } \

#define BEG_FUNC1(name) \
    try \
    {     \
        auto conf = ui->getConf(); \
        uniset_rwmutex_wrlock l(shmMutex); \
        if( true ) \
        { \
            try \
            { \


#define END_FUNC(fname) \
    }\
    catch( const CORBA::TRANSIENT& ){}\
    catch( const CORBA::OBJECT_NOT_EXIST& ){}\
    catch( const CORBA::SystemException& ex){}\
    oref = CORBA::Object::_nil();\
    msleep(conf->getRepeatTimeout());    \
    }\
    } \
    catch( const IOController_i::NameNotFound &ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex.err << endl; \
    } \
    catch( const IOController_i::IOBadParam &ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex.err << endl; \
    } \
    catch( const CORBA::SystemException& ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): CORBA::SystemException: " \
              << ex.NP_minorString() << endl; \
    } \
    catch( const uniset::Exception& ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex << endl; \
    } \
    oref = CORBA::Object::_nil(); \
    throw uniset::TimeOut(); \

#define CHECK_IC_PTR(fname) \
    if( !ic )  \
    { \
        uwarn << "(" << __STRING(fname) << "): function NOT DEFINED..." << endl; \
        throw uniset::TimeOut(); \
    } \

// --------------------------------------------------------------------------
SMInterface::SMInterface( uniset::ObjectId _shmID, const std::shared_ptr<UInterface>& _ui,
                          uniset::ObjectId _myid, const std::shared_ptr<IONotifyController> ic ):
    ic(ic),
    ui(_ui),
    oref( CORBA::Object::_nil() ),
    shmID(_shmID),
    myid(_myid)
{
    if( shmID == DefaultObjectId )
        throw uniset::SystemError("(SMInterface): Unknown shmID!" );
}
// --------------------------------------------------------------------------
SMInterface::~SMInterface()
{

}
// --------------------------------------------------------------------------
void SMInterface::setValue( uniset::ObjectId id, long value )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::setValue)
        ic->setValue(id, value, myid);
        return;
        END_FUNC(SMInterface::setValue)
    }

    IOController_i::SensorInfo si;
    si.id = id;
    si.node = ui->getConf()->getLocalNode();

    BEG_FUNC1(SMInterface::setValue)
    ui->setValue(si, value, myid);
    return;
    END_FUNC(SMInterface::setValue)
}
// --------------------------------------------------------------------------
long SMInterface::getValue( uniset::ObjectId id )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::getValue)
        return ic->getValue(id);
        END_FUNC(SMInterface::getValue)
    }

    BEG_FUNC1(SMInterface::getValue)
    return ui->getValue(id);
    END_FUNC(SMInterface::getValue)
}
// --------------------------------------------------------------------------
void SMInterface::askSensor( uniset::ObjectId id, UniversalIO::UIOCommand cmd, uniset::ObjectId backid )
{
    ConsumerInfo_var ci = new ConsumerInfo();
    ci->id   = (backid == DefaultObjectId) ? myid : backid;
    ci->node = ui->getConf()->getLocalNode();

    if( ic )
    {
        BEG_FUNC1(SMInterface::askSensor)
        ic->askSensor(id, ci, cmd);
        return;
        END_FUNC(SMInterface::askSensor)
    }

    BEG_FUNC1(SMInterface::askSensor)
    ui->askRemoteSensor(id, cmd, conf->getLocalNode(), ci->id);
    return;
    END_FUNC(SMInterface::askSensor)
}
// --------------------------------------------------------------------------
IOController_i::SensorInfoSeq* SMInterface::getSensorsMap()
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::getSensorsMap)
        return ic->getSensorsMap();
        END_FUNC(SMInterface::getSensorsMap)
    }

    BEG_FUNC(SMInterface::getSensorsMap)
    return shm->getSensorsMap();
    END_FUNC(SMInterface::getSensorsMap)
}
// --------------------------------------------------------------------------
IONotifyController_i::ThresholdsListSeq* SMInterface::getThresholdsList()
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::getThresholdsList)
        return ic->getThresholdsList();
        END_FUNC(SMInterface::getThresholdsList)
    }

    BEG_FUNC(SMInterface::getThresholdsList)
    return shm->getThresholdsList();
    END_FUNC(SMInterface::getThresholdsList)
}
// --------------------------------------------------------------------------
void SMInterface::setUndefinedState( const IOController_i::SensorInfo& si, bool undefined,
                                     uniset::ObjectId sup_id )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::setUndefinedState)
        ic->setUndefinedState(si.id, undefined, sup_id);
        return;
        END_FUNC(SMInterface::setUndefinedState)
    }

    BEG_FUNC(SMInterface::setUndefinedState)
    shm->setUndefinedState(si.id, undefined, sup_id);
    return;
    END_FUNC(SMInterface::setUndefinedState)
}
// --------------------------------------------------------------------------
bool SMInterface::exist()
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::exist)
        return ic->exist();
        END_FUNC(SMInterface::exist)
    }

    return ui->isExist(shmID);
}
// --------------------------------------------------------------------------
IOController::IOStateList::iterator SMInterface::ioEnd()
{
    CHECK_IC_PTR(ioEnd)
    return ic->ioEnd();
}
// --------------------------------------------------------------------------
void SMInterface::localSetValue( IOController::IOStateList::iterator& it,
                                 uniset::ObjectId sid,
                                 CORBA::Long value, uniset::ObjectId sup_id )
{
    if( !ic )
        return setValue(sid, value);

    ic->localSetValueIt(it, sid, value, sup_id);
}
// --------------------------------------------------------------------------
long SMInterface::localGetValue( IOController::IOStateList::iterator& it, uniset::ObjectId sid )
{
    if( !ic )
        return getValue( sid );

    //    CHECK_IC_PTR(localGetValue)
    return ic->localGetValue(it, sid);
}
// --------------------------------------------------------------------------
void SMInterface::localSetUndefinedState( IOController::IOStateList::iterator& it,
        bool undefined,
        uniset::ObjectId sid )
{
    //    CHECK_IC_PTR(localSetUndefinedState)
    if( !ic )
    {
        IOController_i::SensorInfo si;
        si.id     = sid;
        si.node = ui->getConf()->getLocalNode();
        setUndefinedState(si, undefined, myid);
        return;
    }

    ic->localSetUndefinedState(it, undefined, sid);
}
// --------------------------------------------------------------------------
void SMInterface::initIterator( IOController::IOStateList::iterator& it )
{
    if( ic )
        it = ic->ioEnd();
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMready( int ready_timeout, int pmsec )
{
    std::atomic_bool cancelFlag = { false };
    return waitSMreadyWithCancellation(ready_timeout, cancelFlag, pmsec);
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMworking( uniset::ObjectId sid, int msec, int pmsec )
{
    PassiveTimer ptSMready(msec);
    bool sm_ready = false;

    while( !ptSMready.checkTime() && !sm_ready )
    {
        try
        {
            getValue(sid);
            sm_ready = true;
            break;
        }
        catch(...) {}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMworkingWithCancellation( uniset::ObjectId sid, int ready_timeout, std::atomic_bool& cancelFlag, int pmsec )
{
    PassiveTimer ptSMready(ready_timeout);
    bool sm_ready = false;

    while( !ptSMready.checkTime() && !sm_ready && !cancelFlag )
    {
        try
        {
            getValue(sid);
            sm_ready = true;
            break;
        }
        catch(...) {}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMreadyWithCancellation(int ready_timeout, std::atomic_bool& cancelFlag, int pmsec)
{
    PassiveTimer ptSMready(ready_timeout);
    bool sm_ready = false;

    while( !ptSMready.checkTime() && !sm_ready && !cancelFlag )
    {
        try
        {
            sm_ready = exist();

            if( sm_ready )
                break;
        }
        catch(...) {}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
#ifndef DISABLE_REST_API
std::string SMInterface::apiRequest( const std::string& query )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::apiRequest)
        SimpleInfo_var i = ic->apiRequest(query.c_str());
        return std::string(i->info);
        END_FUNC(SMInterface::apiRequest)
    }

    BEG_FUNC(SMInterface::apiRequest)
    SimpleInfo_var i = shm->apiRequest(query.c_str());
    return std::string(i->info);
    END_FUNC(SMInterface::apiRequest)
}
#endif
// --------------------------------------------------------------------------
