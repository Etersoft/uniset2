// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "SMInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
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
            catch(CORBA::TRANSIENT){}\
            catch(CORBA::OBJECT_NOT_EXIST){}\
            catch(CORBA::SystemException& ex){}\
            oref = CORBA::Object::_nil();\
            msleep(conf->getRepeatTimeout());    \
        }\
    } \
    catch( IOController_i::NameNotFound &ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex.err << endl; \
    } \
    catch( IOController_i::IOBadParam &ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex.err << endl; \
    } \
    catch( Exception& ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex << endl; \
    } \
    catch(CORBA::SystemException& ex) \
    { \
        uwarn << "(" << __STRING(fname) << "): CORBA::SystemException: " \
            << ex.NP_minorString() << endl; \
    } \
    catch(...) \
    { \
        uwarn << "(" << __STRING(fname) << "): catch ..." << endl; \
    }    \
 \
    oref = CORBA::Object::_nil(); \
    throw UniSetTypes::TimeOut(); \

#define CHECK_IC_PTR(fname) \
        if( !ic )  \
        { \
            uwarn << "(" << __STRING(fname) << "): function NOT DEFINED..." << endl; \
            throw UniSetTypes::TimeOut(); \
        } \

// --------------------------------------------------------------------------
SMInterface::SMInterface( UniSetTypes::ObjectId _shmID, UInterface* _ui,
                            UniSetTypes::ObjectId _myid, const std::shared_ptr<IONotifyController> ic ):
    ic(ic),
    ui(_ui),
    oref( CORBA::Object::_nil() ),
    shmID(_shmID),
    myid(_myid)
{
    if( shmID == DefaultObjectId )
        throw UniSetTypes::SystemError("(SMInterface): Unknown shmID!" );
}
// --------------------------------------------------------------------------
SMInterface::~SMInterface()
{

}
// --------------------------------------------------------------------------
void SMInterface::setValue( UniSetTypes::ObjectId id, long value )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::setValue)
        ic->fastSetValue(id,value,myid);
        return;
        END_FUNC(SMInterface::setValue)
    }

    IOController_i::SensorInfo si;
    si.id = id;
    si.node = ui->getConf()->getLocalNode();

    BEG_FUNC1(SMInterface::setValue)
    ui->fastSetValue(si,value,myid);
    return;
    END_FUNC(SMInterface::setValue)
}
// --------------------------------------------------------------------------
long SMInterface::getValue( UniSetTypes::ObjectId id )
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
void SMInterface::askSensor( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId backid )
{
    ConsumerInfo_var ci;
    ci->id   = (backid==DefaultObjectId) ? myid : backid;
    ci->node = ui->getConf()->getLocalNode();

    if( ic )
    {
        BEG_FUNC1(SMInterface::askSensor)
        ic->askSensor(id, ci, cmd);
        return;
        END_FUNC(SMInterface::askSensor)
    }

    BEG_FUNC1(SMInterface::askSensor)
    ui->askRemoteSensor(id,cmd,conf->getLocalNode(),ci->id);
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
void SMInterface::setUndefinedState( IOController_i::SensorInfo& si, bool undefined, 
                                        UniSetTypes::ObjectId sup_id )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::setUndefinedState)
        ic->setUndefinedState(si.id,undefined,sup_id);
        return;
        END_FUNC(SMInterface::setUndefinedState)
    }
    BEG_FUNC(SMInterface::setUndefinedState)
    shm->setUndefinedState(si.id,undefined,sup_id);
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
                                    UniSetTypes::ObjectId sid,
                                CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
    if( !ic )
        return setValue(sid,value);

//    CHECK_IC_PTR(localSetValue)
    IOController_i::SensorInfo si;
    si.id = sid;
    si.node = ui->getConf()->getLocalNode();
    ic->localSetValue(it,si.id,value,sup_id);
}
// --------------------------------------------------------------------------
long SMInterface::localGetValue( IOController::IOStateList::iterator& it, UniSetTypes::ObjectId sid )
{
    if( !ic )
        return getValue( sid );

//    CHECK_IC_PTR(localGetValue)
    return ic->localGetValue(it,sid);
}
// --------------------------------------------------------------------------
void SMInterface::localSetUndefinedState( IOController::IOStateList::iterator& it,
                                            bool undefined,
                                            UniSetTypes::ObjectId sid )
{
//    CHECK_IC_PTR(localSetUndefinedState)
    if( !ic )
    {
        IOController_i::SensorInfo si;
        si.id     = sid;
        si.node = ui->getConf()->getLocalNode();
        setUndefinedState(si,undefined,myid);
        return;
    }

    ic->localSetUndefinedState(it,undefined,sid);
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
    PassiveTimer ptSMready(ready_timeout);
    bool sm_ready = false;
    while( !ptSMready.checkTime() && !sm_ready )
    {
        try
        {
            sm_ready = exist();
            if( sm_ready )
                break;
        }
        catch(...){}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMworking( UniSetTypes::ObjectId sid, int msec, int pmsec )
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
        catch(...){}
        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
