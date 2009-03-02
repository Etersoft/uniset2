// --------------------------------------------------------------------------
/*! $Id: SMInterface.cc,v 1.1 2008/12/14 21:57:50 vpashka Exp $ */
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
	{	 \
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
					msleep(conf->getRepeatTimeout());	\
					continue;\
				} \

#define BEG_FUNC1(name) \
	try \
	{	 \
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
			msleep(conf->getRepeatTimeout());	\
		}\
	} \
	catch( IOController_i::NameNotFound &ex ) \
	{ \
		unideb[Debug::WARN] << "(" << __STRING(fname) << "): " << ex.err << endl; \
	} \
	catch( IOController_i::IOBadParam &ex ) \
	{ \
		unideb[Debug::WARN] << "(" << __STRING(fname) << "): " << ex.err << endl; \
	} \
	catch( Exception& ex ) \
	{ \
		unideb[Debug::WARN] << "(" << __STRING(fname) << "): " << ex << endl; \
	} \
	catch(CORBA::SystemException& ex) \
	{ \
		unideb[Debug::WARN] << "(" << __STRING(fname) << "): óORBA::SystemException: " \
			<< ex.NP_minorString() << endl; \
	} \
	catch(...) \
	{ \
		unideb[Debug::WARN] << "(" << __STRING(fname) << "): catch ..." << endl; \
	}	\
 \
	oref = CORBA::Object::_nil(); \
	throw UniSetTypes::TimeOut(); \

#define CHECK_IC_PTR(fname) \
		if( !ic )  \
		{ \
			unideb[Debug::WARN] << "(" << __STRING(fname) << "): function NOT DEFINED..." << endl; \
			throw UniSetTypes::TimeOut(); \
		} \

// --------------------------------------------------------------------------
SMInterface::SMInterface( UniSetTypes::ObjectId _shmID, UniversalInterface* _ui, 
							UniSetTypes::ObjectId _myid, IONotifyController* ic ):
	ic(ic),
	ui(_ui),
	oref( CORBA::Object::_nil() ),
	shmID(_shmID),
	myid(_myid)
{
}
// --------------------------------------------------------------------------
SMInterface::~SMInterface()
{

}
// --------------------------------------------------------------------------
void SMInterface::setState ( UniSetTypes::ObjectId id, bool state )
{
	IOController_i::SensorInfo si;
	si.id = id;
	si.node = conf->getLocalNode();

	if( ic )
	{
		BEG_FUNC1(SMInterface::setState)
		ic->fastSetState(si,state,myid);
		return;
		END_FUNC(SMInterface::setState)
	}
	
	BEG_FUNC(SMInterface::setState)
	shm->fastSetState(si,state,myid);
	return;
	END_FUNC(SMInterface::setState)
}
// --------------------------------------------------------------------------
void SMInterface::setValue ( UniSetTypes::ObjectId id, long value )
{
	IOController_i::SensorInfo si;
	si.id = id;
	si.node = conf->getLocalNode();

	if( ic )
	{
		BEG_FUNC1(SMInterface::setValue)
		ic->fastSetValue(si,value,myid);
		return;
		END_FUNC(SMInterface::setValue)
	}
	
	BEG_FUNC(SMInterface::setValue)
	shm->fastSetValue(si,value,myid);
	return;
	END_FUNC(SMInterface::setValue)
}
// --------------------------------------------------------------------------
bool SMInterface::saveState ( IOController_i::SensorInfo& si, bool state, 
								UniversalIO::IOTypes type, UniSetTypes::ObjectId sup_id )
{
	if( ic )
	{
		BEG_FUNC1(SMInterface::saveState)
		ic->fastSaveState(si,state,type,sup_id);
		return true;
		END_FUNC(SMInterface::saveState)
	}

	BEG_FUNC(SMInterface::saveState)
	shm->fastSaveState(si,state,type,sup_id);
	return true;
	END_FUNC(SMInterface::saveState)
}
// --------------------------------------------------------------------------
bool SMInterface::saveValue ( IOController_i::SensorInfo& si, long value, 
								UniversalIO::IOTypes type, UniSetTypes::ObjectId sup_id )
{
	if( ic )
	{
		BEG_FUNC1(SMInterface::saveValue)
		ic->fastSaveValue(si,value,type,sup_id);
		return true;
		END_FUNC(SMInterface::saveValue)
	}

	BEG_FUNC(SMInterface::saveValue)
	shm->fastSaveValue(si,value,type,sup_id);
	return true;
	END_FUNC(SMInterface::saveValue)
}
// --------------------------------------------------------------------------
long SMInterface::getValue ( UniSetTypes::ObjectId id )
{
	IOController_i::SensorInfo si;
	si.id = id;
	si.node = conf->getLocalNode();

	if( ic )
	{
		BEG_FUNC1(SMInterface::getValue)
		return ic->getValue(si);
		END_FUNC(SMInterface::getValue)
	}

	BEG_FUNC(SMInterface::getValue)
	return shm->getValue(si);
	END_FUNC(SMInterface::getValue)
}
// --------------------------------------------------------------------------
bool SMInterface::getState ( UniSetTypes::ObjectId id )
{
	IOController_i::SensorInfo si;
	si.id = id;
	si.node = conf->getLocalNode();

	if( ic )
	{
		BEG_FUNC1(SMInterface::getState)
		return ic->getState(si);
		END_FUNC(SMInterface::getState)
	}
	
	BEG_FUNC(SMInterface::getState)
	return shm->getState(si);
	END_FUNC(SMInterface::getState)
}
// --------------------------------------------------------------------------
bool SMInterface::saveLocalState( UniSetTypes::ObjectId id, bool state, 
								UniversalIO::IOTypes type )
{
	IOController_i::SensorInfo si;
	si.id = id;
	si.node = conf->getLocalNode();
	return saveState(si,state,type,myid);
}
// --------------------------------------------------------------------------
bool SMInterface::saveLocalValue ( UniSetTypes::ObjectId id, long value, 
								UniversalIO::IOTypes type )
{
	IOController_i::SensorInfo si;
	si.id = id;
	si.node = conf->getLocalNode();
	return saveValue(si,value,type,myid);
}
// --------------------------------------------------------------------------
void SMInterface::askSensor( UniSetTypes::ObjectId id, 
								UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId backid )
{
	IOController_i::SensorInfo_var si;
	si->id 		= id;
	si->node 	= conf->getLocalNode();
	ConsumerInfo_var ci;
	ci->id 		= (backid==DefaultObjectId) ? myid : backid;
	ci->node 	= conf->getLocalNode();

	if( ic )
	{
		BEG_FUNC1(SMInterface::askSensor)
		ic->askSensor(si, ci, cmd );
		return;
		END_FUNC(SMInterface::askSensor)
	}

	BEG_FUNC(SMInterface::askSensor)
	shm->askSensor(si, ci, cmd );
	return;
	END_FUNC(SMInterface::askSensor)
}
// --------------------------------------------------------------------------
IOController_i::DSensorInfoSeq* SMInterface::getDigitalSensorsMap()
{
	if( ic )
	{
		BEG_FUNC1(SMInterface::getDigitalSensorsMap)
		return ic->getDigitalSensorsMap();
		END_FUNC(SMInterface::getDigitalSensorsMap)
	}

	BEG_FUNC(SMInterface::getDigitalSensorsMap)
	return shm->getDigitalSensorsMap();
	END_FUNC(SMInterface::getDigitalSensorsMap)
}
// --------------------------------------------------------------------------
IOController_i::ASensorInfoSeq* SMInterface::getAnalogSensorsMap()
{
	if( ic )
	{
		BEG_FUNC1(SMInterface::getAnalogSensorsMap)
		return ic->getAnalogSensorsMap();
		END_FUNC(SMInterface::getAnalogSensorsMap)
	}

	BEG_FUNC(SMInterface::getAnalogSensorsMap)
	return shm->getAnalogSensorsMap();
	END_FUNC(SMInterface::getAnalogSensorsMap)
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
		ic->setUndefinedState(si,undefined,sup_id);
		return;
		END_FUNC(SMInterface::setUndefinedState)
	}
	BEG_FUNC(SMInterface::setUndefinedState)
	shm->setUndefinedState(si,undefined,sup_id);
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
	BEG_FUNC(SMInterface::exist)
	return shm->exist();
	END_FUNC(SMInterface::exist)
}
// --------------------------------------------------------------------------
IOController::DIOStateList::iterator SMInterface::dioEnd()
{
	CHECK_IC_PTR(dioEnd)
	return ic->dioEnd();
}
// --------------------------------------------------------------------------
IOController::AIOStateList::iterator SMInterface::aioEnd()
{
	CHECK_IC_PTR(aioEnd)
	return ic->aioEnd();
}
// --------------------------------------------------------------------------
void SMInterface::localSaveValue( IOController::IOController::AIOStateList::iterator& it, 
									UniSetTypes::ObjectId sid, 
									CORBA::Long nval, UniSetTypes::ObjectId sup_id )
{
	if( !ic )
	{
		saveLocalValue( sid, nval );
		return;
	}
	
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	ic->localSaveValue(it,si,nval,sup_id);
}										
// --------------------------------------------------------------------------
void SMInterface::localSaveState( IOController::DIOStateList::iterator& it, 
									UniSetTypes::ObjectId sid, 
									CORBA::Boolean nstate, UniSetTypes::ObjectId sup_id )
{
	if( !ic )
	{
		saveLocalState( sid, nstate );
		return;
	}
	
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	ic->localSaveState(it,si,nstate,sup_id);
}
// --------------------------------------------------------------------------
void SMInterface::localSetState( IOController::DIOStateList::iterator& it, 
									UniSetTypes::ObjectId sid, 
									CORBA::Boolean newstate, UniSetTypes::ObjectId sup_id )
{
	if( !ic )
		return setState( sid, newstate );
//	CHECK_IC_PTR(localSetState)
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	ic->localSetState(it,si,newstate,sup_id);
}
// --------------------------------------------------------------------------
void SMInterface::localSetValue( IOController::AIOStateList::iterator& it, 
									UniSetTypes::ObjectId sid, 
								CORBA::Long value, UniSetTypes::ObjectId sup_id )
{
	if( !ic )
		return setValue(sid,value);

//	CHECK_IC_PTR(localSetValue)
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	ic->localSetValue(it,si,value,sup_id);
}
// --------------------------------------------------------------------------
bool SMInterface::localGetState( IOController::DIOStateList::iterator& it, UniSetTypes::ObjectId sid )
{
//	CHECK_IC_PTR(localGetState)
	if( !ic )
		return getState(sid);

	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	return ic->localGetState(it,si);
}
// --------------------------------------------------------------------------
long SMInterface::localGetValue( IOController::AIOStateList::iterator& it, UniSetTypes::ObjectId sid )
{
	if( !ic )
		return getValue( sid );

//	CHECK_IC_PTR(localGetValue)
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	return ic->localGetValue(it,si);
}
// --------------------------------------------------------------------------
void SMInterface::localSetUndefinedState( IOController::AIOStateList::iterator& it, 
											bool undefined,
											UniSetTypes::ObjectId sid )
{
//	CHECK_IC_PTR(localSetUndefinedState)
	if( !ic )
	{
		IOController_i::SensorInfo si;
		si.id 	= sid;
		si.node = conf->getLocalNode();
		setUndefinedState( si,undefined,myid);
		return;
	}

	IOController_i::SensorInfo si;
	si.id 	= sid;
	si.node = conf->getLocalNode();
	ic->localSetUndefinedState(it,undefined,si);
}												
// --------------------------------------------------------------------------
void SMInterface::initAIterator( IOController::AIOStateList::iterator& it )
{
	if( ic )
		it = ic->aioEnd();
	else	
		cerr << "(SMInterface::initAIterator): ic=NULL" << endl;
}
// --------------------------------------------------------------------------
void SMInterface::initDIterator( IOController::DIOStateList::iterator& it )
{
	if( ic )
		it = ic->dioEnd();
	else	
		cerr << "(SMInterface::initDIterator): ic=NULL" << endl;
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
			getState(sid);
			sm_ready = true;
			break;
		}
		catch(...){}
		msleep(pmsec);
	}

	return sm_ready;
}
// --------------------------------------------------------------------------
