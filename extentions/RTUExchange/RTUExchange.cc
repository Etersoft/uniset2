// $Id: RTUExchange.cc,v 1.4 2009/01/23 23:56:54 vpashka Exp $
// -----------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extentions.h"
#include "RTUExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtentions;
// -----------------------------------------------------------------------------
RTUExchange::RTUExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic ):
UniSetObject_LT(objId),
rsmap(100),
maxItem(0),
mb(0),
shm(0),
initPause(0),
force(false),
force_out(false),
mbregFromID(false),
activated(false)
{
	cout << "$Id: RTUExchange.cc,v 1.4 2009/01/23 23:56:54 vpashka Exp $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(RTUExchange): objId=-1?!! Use --rs-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(RTUExchange): Not find conf-node for " + myname );


	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--rs-filter-field");
	s_fvalue = conf->getArgParam("--rs-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	// ---------- init RS ----------
//	UniXML_iterator it(cnode);
	string dev	= conf->getArgParam("--rs-dev",it.getProp("device"));
	if( dev.empty() )
		throw UniSetTypes::SystemError(myname+"(RTUExchange): Unknown device..." );

	string speed 	= conf->getArgParam("--rs-speed",it.getProp("speed"));
	if( speed.empty() )
		speed = "38400";

	int recv_timeout = atoi(conf->getArgParam("--rs-recv-timeout",it.getProp("recv_timeout")).c_str());

	sidNotRespond = conf->getSensorID(conf->getArgParam("--rs-notRespondSensor",it.getProp("notRespondSensor")));
	

//	string saddr = conf->getArgParam("--rs-my-addr",it.getProp("addr"));
//	ModbusRTU::ModbusAddr myaddr = ModbusRTU::str2mbAddr(saddr);
//	if( saddr.empty() )
//		myaddr = 0x01;

	mbregFromID = atoi(conf->getArgParam("--mbs-reg-from-id",it.getProp("reg_from_id")).c_str());
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	mb = new ModbusRTUMaster(dev);

	if( !speed.empty() )
		mb->setSpeed(speed);

	if( recv_timeout > 0 )
		mb->setTimeout(recv_timeout);

	dlog[Debug::INFO] << myname << "(init): dev=" << dev << " speed=" << speed << endl;
	// -------------------------------

	polltime = atoi(conf->getArgParam("--rs-polltime",it.getProp("polltime")).c_str());
	if( !polltime )
		polltime = 100;

	initPause = atoi(conf->getArgParam("--rs-initPause",it.getProp("initPause")).c_str());
	if( !initPause )
		initPause = 3000;

	force = atoi(conf->getArgParam("--rs-force",it.getProp("force")).c_str());

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rsmap.resize(maxItem);
		dlog[Debug::INFO] << myname << "(init): rsmap size = " << rsmap.size() << endl;
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&RTUExchange::readItem) );

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--rs-heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);
		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": не найден идентификатор для датчика 'HeartBeat' " << heart;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = getHeartBeatTime();
		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = atoi(conf->getArgParam("--rs-heartbeat-max",it.getProp("heartbeat_max")).c_str());
		if( maxHeartBeat <=0 )
			maxHeartBeat = 10;
		test_id = sidHeartBeat;
	}
	else
	{
		test_id = conf->getSensorID("TestMode_S");
		if( test_id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(init): test_id unknown. 'TestMode_S' not found...";
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	dlog[Debug::INFO] << myname << "(init): test_id=" << test_id << endl;

	ai_polltime = atoi(conf->getArgParam("--aiPollTime").c_str());
	if( ai_polltime > 0 )
		aiTimer.setTiming(ai_polltime);

	activateTimeout	= atoi(conf->getArgParam("--activate-timeout").c_str());
	if( activateTimeout<=0 )
		activateTimeout = 20000;

	int msec = atoi(conf->getArgParam("--rs-timeout",it.getProp("timeout")).c_str());
	if( msec <=0 )
		msec = 3000;

	ptTimeout.setTiming(msec);

	dlog[Debug::INFO] << myname << "(init): rs-timeout=" << msec << " msec" << endl;
}
// -----------------------------------------------------------------------------
RTUExchange::~RTUExchange()
{
	// Опрос всех RTU...
	for( RTUMap::iterator it=rtulist.begin(); it!=rtulist.end(); ++it )
	{
		if( it->second )
		{
			delete it->second;
			it->second = 0;
		}
	}
	
	delete mb;
	delete shm;
}
// -----------------------------------------------------------------------------
void RTUExchange::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = atoi(conf->getArgParam("--rs-sm-ready-timeout","15000").c_str());
	if( ready_timeout == 0 )
		ready_timeout = 15000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout,50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): Не дождались готовности SharedMemory к работе в течение " << ready_timeout << " мсек";
		dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}
}
// -----------------------------------------------------------------------------
#if 0
void RTUExchange::execute()
{
	// waiting for SM is ready...
	waitSMReady();
	
	// вызывать обязательно после shm->wait!!!
	if( !shm->isLocalwork() )
		readConfiguration();

	rsmap.resize(maxItem);
	dlog[Debug::INFO] << myname << "(init): rsmap size = " << rsmap.size() << endl;
	
	PassiveTimer ptAct(activateTimeout);
	while( !activated && !ptAct.checkTime() )
	{	
		cout << myname << "(execute): wait activate..." << endl;
		msleep(300);
		if( activated )
		{
			cout << myname << "(execute): activate OK.." << endl;
			break;
		}
	}
			
	if( !activated )
		dlog[Debug::CRIT] << myname << "(execute): ************* don`t activate?! ************" << endl;

	// подождать пока пройдёт инициализация датчиков
	// см. activateObject() и sysCommand()
	{
		// ставим паузу чтобы это mutex сработал обязательно позже
		// чем mutex в sysCommand и activateObject
		msleep(1500); 
		UniSetTypes::uniset_mutex_lock l(mutex_start, 15000);
	}

	// ---------- init RS ----------
	UniXML_iterator it(cnode);
	string dev	= conf->getArgParam("--rs-dev",it.getProp("device"));
	if( dev.empty() )
		throw UniSetTypes::SystemError(myname+"(RTUExchange): Unknown device..." );

	string speed 	= conf->getArgParam("--rs-speed",it.getProp("speed"));
	int recv_timeout = atoi(conf->getArgParam("--rs-recv-timeout",it.getProp("recv_timeout")).c_str());

	string saddr = conf->getArgParam("--rs-my-addr",it.getProp("addr"));
	ModbusRTU::ModbusAddr myaddr = ModbusRTU::str2mbAddr(saddr);
	if( saddr.empty() )
		myaddr = 0x01;

	mb = new ModbusMaster(dev,myaddr);

	if( !speed.empty() )
		mb->setSpeed(speed);

	if( recv_timeout > 0 )
		mb->setTimeout(recv_timeout);

	dlog[Debug::INFO] << myname << "(init): myaddr=" << ModbusRTU::addr2str(myaddr) 
			<< " dev=" << dev << " speed=" << speed << endl;
	// -------------------------------

	// начальная инициализация
	if( !force )
	{
		uniset_mutex_lock l(pollMutex,2000);
		force = true;
		poll();
		force = false;
	}

	while( activated )
	{
		try
		{
			step();
		}
		catch( UniSetTypes::Exception& ex)
		{
			cerr << myname << "(step): " << ex << std::endl;
		}
		catch(...)
		{
			cerr << myname << "(step): catch ..." << std::endl;
		}	

		msleep(polltime);
	}

	cerr << "************* execute FINISH **********" << endl;
}
#endif
// -----------------------------------------------------------------------------
void RTUExchange::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmExchange )
		step();
}
// -----------------------------------------------------------------------------
void RTUExchange::step()
{
	{
		uniset_mutex_lock l(pollMutex,2000);
		poll();
	}

	if( !activated )
		return;

	if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSaveValue(aitHeartBeat,sidHeartBeat,maxHeartBeat,getId());
			ptHeartBeat.reset();
		}
		catch(Exception& ex)
		{
			dlog[Debug::CRIT] << myname
				<< "(step): (hb) " << ex << std::endl;
		}
	}
}

// -----------------------------------------------------------------------------
void RTUExchange::poll()
{
	bool respond_ok = true;
	for( RTUMap::iterator it=rtulist.begin(); it!=rtulist.end(); ++it )
	{
		if( !activated )
			return;

		try
		{
			cout << "poll RTU=(" << (int)it->second->getAddress() 
								<< ")" << ModbusRTU::addr2str(it->second->getAddress());
			if( it->second )
			{
				mb->cleanupChannel();
				it->second->poll(mb);
				cout << " [OK] " << endl;
//				cout << it->second << endl;
			}
		}
		catch( ModbusRTU::mbException& ex )
		{ 
			cout << " [ FAILED ] (" << ex << ")" << endl; 
			respond_ok = false;
		}
	}

	for( RSMap::iterator it=rsmap.begin(); it!=rsmap.end(); ++it )
	{
		if( !activated )
			return;

		if( it->mbaddr == 0 ) // || it->mbreg == 0 )
			continue;
		
		if( it->si.id == DefaultObjectId )
		{
			cerr << myname << "(poll): sid=DefaultObjectId?!" << endl;
			continue;
		}
		

		IOBase* ib = &(*it);

		try
		{
			if( it->stype == UniversalIO::AnalogInput )
			{
				long val = 0;
				if( it->devtype == dtRTU )
					val = pollRTU(it);
				else if( it->devtype == dtMTR )
					val = pollMTR(it);
				else if( it->devtype == dtRTU188 )
					val = pollRTU188(it);
				else
					continue;
				IOBase::processingAsAI( ib, val, shm, force );
			}
			else if( it->stype == UniversalIO::DigitalInput )
			{
				bool set = false;
				if( it->devtype == dtRTU )
					set = pollRTU(it) ? true : false;
				else if( it->devtype == dtMTR )
					set = pollMTR(it) ? true : false;
				else if( it->devtype == dtRTU188 )
					set = pollRTU188(it) ? true : false;
				else
					continue;

				IOBase::processingAsDI( ib, set, shm, force );
			}
			else if( it->stype == UniversalIO::AnalogOutput ||
					 it->stype == UniversalIO::DigitalOutput )
			{
				ModbusRTU::ModbusData d = 0;
				if( it->stype == UniversalIO::AnalogOutput )
					d = IOBase::processingAsAO( ib, shm, force_out );
				else 
					d = IOBase::processingAsDO( ib, shm, force_out ) ? 1 : 0;

				using namespace ModbusRTU;
				switch(it->mbfunc)
				{
					case fnWriteOutputSingleRegister:
					{
						WriteSingleOutputRetMessage ret = mb->write06( it->mbaddr,it->mbreg,d);
					}
					break;

					case fnWriteOutputRegisters:
					{
						WriteOutputMessage msg(it->mbaddr,it->mbreg);
						msg.addData(d);
						WriteOutputRetMessage ret = mb->write10(msg);
					}
					break;
					
					default:
						break;
				}
			}
			
			continue;
		}
		catch(ModbusRTU::mbException& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(poll): " << ex << endl;
		}
		catch(IOController_i::NameNotFound &ex)
		{
			dlog[Debug::LEVEL3] << myname << "(poll):(NameNotFound) " << ex.err << endl;
		}
		catch(IOController_i::IOBadParam& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(poll):(IOBadParam) " << ex.err << endl;
		}
		catch(IONotifyController_i::BadRange )
		{
			dlog[Debug::LEVEL3] << myname << "(poll): (BadRange)..." << endl;
		}
		catch( Exception& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(poll): " << ex << endl;
		}
		catch(CORBA::SystemException& ex)
		{
			dlog[Debug::LEVEL3] << myname << "(poll): СORBA::SystemException: "
				<< ex.NP_minorString() << endl;
		}
		catch(...)
		{
			dlog[Debug::LEVEL3] << myname << "(poll): catch ..." << endl;
		}
		
		respond_ok = false;
	}

	if( sidNotRespond!=DefaultObjectId )
	{
		if( trTimeout.hi( !respond_ok ) )
			ptTimeout.reset();
		else if( respond_ok )
			ptTimeout.reset();

		if( !respond_ok && ptTimeout.checkTime() )
			shm->localSaveState(ditNotRespond,sidNotRespond,true,getId());
		else
			shm->localSaveState(ditNotRespond,sidNotRespond,false,getId());
	}
}
// -----------------------------------------------------------------------------
long RTUExchange::pollRTU( RSMap::iterator& p )
{
	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(pollRTU): poll "
			<< " mbaddr=" << ModbusRTU::addr2str(p->mbaddr)
			<< " mbreg=" << ModbusRTU::dat2str(p->mbreg)
			<< " mbfunc=" << p->mbfunc
			<< endl;
	}
	
	switch( p->mbfunc )
	{
		case ModbusRTU::fnReadInputRegisters:
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(p->mbaddr,p->mbreg,1);
			return ret.data[0];
		}
		break;

		case ModbusRTU::fnReadOutputRegisters:
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(p->mbaddr, p->mbreg, 1);
			return ret.data[0];
		}
		break;
		
		case ModbusRTU::fnReadInputStatus:
		{
			ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(p->mbaddr, p->mbreg,1);
			ModbusRTU::DataBits b(ret.data[0]);
			return b[p->nbit];
		}
		break;
		
		case ModbusRTU::fnReadCoilStatus:
		{
			ModbusRTU::ReadCoilRetMessage ret = mb->read01(p->mbaddr,p->mbreg,1);
			ModbusRTU::DataBits b(ret.data[0]);
			return b[p->nbit];
		}
		break;
		
		default:
		break;
	}
	
	return 0;
}
// -----------------------------------------------------------------------------
long RTUExchange::pollRTU188( RSMap::iterator& p )
{
	if( !p->rtu )
		return 0;

	if( p->stype == UniversalIO::DigitalInput )
		return p->rtu->getState(p->rtuJack,p->rtuChan,p->stype);
	
	if( p->stype == UniversalIO::AnalogInput )
		return p->rtu->getInt(p->rtuJack,p->rtuChan,p->stype);
	
	return 0;
}
// -----------------------------------------------------------------------------
long RTUExchange::pollMTR( RSMap::iterator& p )
{
	unsigned short v1=0, v2=0;
	if( p->mbfunc == ModbusRTU::fnReadInputRegisters )
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04(p->mbaddr, p->mbreg, MTR::wsize(p->mtrType));
		v1 = ret.data[0];
		v2 = ret.data[1];
	}
	else if( p->mbfunc == ModbusRTU::fnReadOutputRegisters )
	{
		ModbusRTU::ReadOutputRetMessage ret = mb->read03(p->mbaddr, p->mbreg, MTR::wsize(p->mtrType));
		v1 = ret.data[0];
		v2 = ret.data[1];
	}
	else
	{
		cerr << myname << "(readMTR): неподдерживаемая функция чтения " << (int)p->mbfunc << endl;
		return 0;
	}

	if( p->mtrType == MTR::mtT1 )
	{
		MTR::T1 t(v1);
		return t.val;
	}
		
	if( p->mtrType == MTR::mtT2 )
	{
		MTR::T2 t(v1);
		return t.val;
	}

	if( p->mtrType == MTR::mtT3 )
	{
		MTR::T3 t(v1,v2);
		return (long)t;
	}

	if( p->mtrType == MTR::mtT4 )
	{
		MTR::T4 t(v1);
		return atoi(t.sval.c_str());
	}

	if( p->mtrType == MTR::mtT5 )
	{
		MTR::T5 t(v1,v2);
		return (long)t.val;
	}

	if( p->mtrType == MTR::mtT6 )
	{
		MTR::T6 t(v1,v2);
		return (long)t.val;
	}

	if( p->mtrType == MTR::mtT7 )
	{
		MTR::T7 t(v1,v2);
		return (long)t.val;
	}

	if( p->mtrType == MTR::mtF1 )
	{
		MTR::F1 t(v1,v2);
		return t;
	}

	return 0;
}
// -----------------------------------------------------------------------------
void RTUExchange::processingMessage(UniSetTypes::VoidMessage *msg)
{
	try
	{
		switch(msg->type)
		{
			case UniSetTypes::Message::SysCommand:
			{
				UniSetTypes::SystemMessage sm( msg );
				sysCommand( &sm );
			}

			break;

			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
				break;
			}
		
			case Message::SensorInfo:
			{
				SensorMessage sm( msg );
				sensorInfo(&sm);
			}
			break;

			default:
				break;
		}
	}
	catch( SystemError& ex )
	{
		dlog[Debug::CRIT] << myname << "(SystemError): " << ex << std::endl;
//		throw SystemError(ex);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): catch ...\n";
	}
}
// -----------------------------------------------------------------------------
void RTUExchange::sysCommand( UniSetTypes::SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( rsmap.empty() )
			{
				dlog[Debug::CRIT] << myname << "(sysCommand): rsmap EMPTY! terminated..." << endl;
				raise(SIGTERM);
				return; 
			}

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << "(sysCommand): rsmap size= "<< rsmap.size() << endl;

		
			waitSMReady();

			// подождать пока пройдёт инициализация датчиков
			// см. activateObject()
			msleep(initPause);
			PassiveTimer ptAct(activateTimeout);
			while( !activated && !ptAct.checkTime() )
			{	
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(300);
				if( activated )
					break;
			}
			
			if( !activated )
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
			
			{
				UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
				askSensors(UniversalIO::UIONotify);
				initOutput();
			}
			
			askTimer(tmExchange,polltime);

			// начальная инициализация
			if( !force )
			{
				uniset_mutex_lock l(pollMutex,2000);	
				force = true;
				poll();
				force = false;
			}
			break;
		}				

		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::WatchDog:
		{
			// ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
			// Если идёт локальная работа 
			// (т.е. RTUExchange  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(RTUExchange)
			if( shm->isLocalwork() )
				break;

			askSensors(UniversalIO::UIONotify);
			initOutput();

			if( !force )
			{
				uniset_mutex_lock l(pollMutex,2000);
				force = true;
				poll();
				force = false;
			}
		}
		break;

		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			unideb << myname << "(sysCommand): logRotate" << std::endl;
			string fname = unideb.getLogFile();
			if( !fname.empty() )
			{
				unideb.logFile(fname.c_str());
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}

			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname.c_str());
				dlog << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
void RTUExchange::initOutput()
{
}
// ------------------------------------------------------------------------------------------
void RTUExchange::askSensors( UniversalIO::UIOCommand cmd )
{
	if( !shm->waitSMworking(test_id,activateTimeout,50) )
	{
		ostringstream err;
		err << myname 
			<< "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение " 
			<< activateTimeout << " мсек";
	
		dlog[Debug::CRIT] << err.str() << endl;
		kill(SIGTERM,getpid());	// прерываем (перезапускаем) процесс...
		throw SystemError(err.str());
	}

	RSMap::iterator it=rsmap.begin();
	for( ; it!=rsmap.end(); ++it )
	{
		if( it->stype != UniversalIO::DigitalOutput && it->stype != UniversalIO::AnalogOutput )
			continue;

		if( it->safety == NoSafetyState )
			continue;

		try
		{
			shm->askSensor(it->si.id,cmd);
		}
		catch( UniSetTypes::Exception& ex )
		{
			dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
		}
		catch(...){}
	}
}
// ------------------------------------------------------------------------------------------
void RTUExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	RSMap::iterator it=rsmap.begin();
	for( ; it!=rsmap.end(); ++it )
	{
		if( it->stype != UniversalIO::DigitalOutput && it->stype != UniversalIO::AnalogOutput )
			continue;

		if( it->si.id == sm->id )
		{
			if( it->stype == UniversalIO::DigitalOutput )
			{
				uniset_spin_lock lock(it->val_lock);
				it->value = sm->state ? 1 : 0;
			}
			else if( it->stype == UniversalIO::AnalogOutput )
			{
				uniset_spin_lock lock(it->val_lock);
				it->value = sm->value;
			}
			
			break;
		}
	}
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::activateObject()
{
	// блокирование обработки Starsp 
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		activated = false;
		UniSetTypes::uniset_mutex_lock l(mutex_start, 5000);
		UniSetObject_LT::activateObject();
		initIterators();
		activated = true;
	}

	return true;
}
// ------------------------------------------------------------------------------------------
void RTUExchange::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
#warning Доделать...
	// выставление безопасного состояния на выходы....
	RSMap::iterator it=rsmap.begin();
	for( ; it!=rsmap.end(); ++it )
	{
		if( it->stype!=UniversalIO::DigitalOutput && it->stype!=UniversalIO::AnalogOutput )
			continue;
		
		if( it->safety == NoSafetyState )
			continue;

		try
		{
		}
		catch( UniSetTypes::Exception& ex )
		{
			dlog[Debug::WARN] << myname << "(sigterm): " << ex << std::endl;
		}
		catch(...){}
	}
	
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void RTUExchange::readConfiguration()
{
#warning Сделать сортировку по диапазонам адресов!!!
// чтобы запрашивать одним запросом, сразу несколько входов...
//	readconf_ok = false;
	xmlNode* root = conf->getXMLSensorsSection();
	if(!root)
	{
		ostringstream err;
		err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
		throw SystemError(err.str());
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		std::cerr << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( check_item(it) )
			initItem(it);
	}
	
//	readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::check_item( UniXML_iterator& it )
{
	if( s_field.empty() )
		return true;

	// просто проверка на не пустой field
	if( s_fvalue.empty() && it.getProp(s_field).empty() )
		return false;

	// просто проверка что field = value
	if( !s_fvalue.empty() && it.getProp(s_field)!=s_fvalue )
		return false;

	return true;
}
// ------------------------------------------------------------------------------------------

bool RTUExchange::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------

bool RTUExchange::initItem( UniXML_iterator& it )
{
	string rstype = it.getProp("mbtype");

	RSProperty p;

	bool ret = false;

	if( rstype == "mtr" )
		ret = initMTRitem(it,p);
	else if( rstype == "rtu" )
		ret = initRTUitem(it,p);
	else if ( rstype == "rtu188" )
	{
		ret = initRTU188item(it,p);
		if( ret )
		{
			RTUMap::iterator r = rtulist.find(p.mbaddr);
			if( r == rtulist.end() )
			{
				p.rtu = new RTUStorage(p.mbaddr);
				rtulist[p.mbaddr] = p.rtu;
			}
			else
				p.rtu = r->second;
		}
	}
		
	if( ret )
	{
		// если вектор уже заполнен
		// то увеличиваем его на 10 элементов (с запасом)
		// после инициализации делается resize
		// под текущее количество
		if( maxItem >= rsmap.size() )
			rsmap.resize(maxItem+10);
	
		rsmap[maxItem] = p;
		maxItem++;
		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;
	}

	return ret;
}

// ------------------------------------------------------------------------------------------
bool RTUExchange::initCommParam( UniXML_iterator& it, RSProperty& p )
{
	if( !IOBase::initItem( static_cast<IOBase*>(&p),it,shm,&dlog,myname) )
		return false;

	string addr = it.getProp("mbaddr");
	if( addr.empty() )
		return true;

	p.mbaddr 	= ModbusRTU::str2mbAddr(addr);
	p.mbfunc 	= ModbusRTU::fnUnknown;

	string f = it.getProp("mbfunc");
	if( !f.empty() )
	{
		p.mbfunc = (ModbusRTU::SlaveFunctionCode)UniSetTypes::uni_atoi(f.c_str());
		if( p.mbfunc == ModbusRTU::fnUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initCommParam): Неверный mbfunc ='" << f
					<< "' для  датчика " << it.getProp("name") << endl;

			return false;
		}
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::initRTUitem( UniXML_iterator& it, RSProperty& p )
{
	p.devtype = dtRTU;

	if( !initCommParam(it,p) )
		return false;
	
	if( mbregFromID )
	{
		p.mbreg = p.si.id;
		return true;
	}

	string reg = it.getProp("mbreg");
	if( reg.empty() )
		return true;

	p.mbreg = ModbusRTU::str2mbData(reg);
	
	
	if( p.mbfunc == ModbusRTU::fnReadCoilStatus ||
		p.mbfunc == ModbusRTU::fnReadInputStatus )
	{
		string nb = it.getProp("nbit");
		if( nb.empty() )
		{
			dlog[Debug::CRIT] << myname << "(readRTUItem): Unknown nbit. for " 
					<< it.getProp("name") 
					<< " mbfunc=" << p.mbfunc
					<< endl;

			return false;
		}
		
		p.nbit = UniSetTypes::uni_atoi(nb.c_str());
	}
	
	return true;
}
// ------------------------------------------------------------------------------------------

bool RTUExchange::initMTRitem( UniXML_iterator& it, RSProperty& p )
{
	p.devtype = dtMTR;

	if( !initCommParam(it,p) )
		return false;
	
	p.mtrType = MTR::str2type(it.getProp("mtrtype"));
	if( p.mtrType == MTR::mtUnknown )
	{
		dlog[Debug::CRIT] << myname << "(readMTRItem): Неверный MTR-тип '" 
					<< it.getProp("mtrtype")
					<< "' для  датчика " << it.getProp("name") << endl;

		return false;
	}

	if( mbregFromID )
	{
		p.mbreg = p.si.id;
		return true;
	}

	string reg = it.getProp("mbreg");
	if( reg.empty() )
		return true;

	p.mbreg = ModbusRTU::str2mbData(reg);
	return true;
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::initRTU188item( UniXML_iterator& it, RSProperty& p )
{
	p.devtype = dtRTU188;

	if( !check_item(it) )
	{
		cerr << "******* false filter: " << it.getProp("name") << endl;
		return false;
	}

	if( !initCommParam(it,p) )
		return false;

	string jack = it.getProp("jack");
	string chan	= it.getProp("channel");
	
	if( jack.empty() )
	{
		dlog[Debug::CRIT] << myname << "(readRTUItem): Не задан разъём jack='' "
					<< " для  " << it.getProp("name") << endl;
		return false;
	}
	p.rtuJack = RTUStorage::s2j(jack);
	if( p.rtuJack == RTUStorage::nUnknown )
	{
		dlog[Debug::CRIT] << myname << "(readRTUItem): Неверное значение jack=" << jack
					<< " для  " << it.getProp("name") << endl;
		return false;
	}

	if( chan.empty() )
	{
		dlog[Debug::CRIT] << myname << "(readRTUItem): Не задан разъём channel='' "
					<< " для  " << it.getProp("name") << endl;
		return false;
	}
	
	p.rtuChan = UniSetTypes::uni_atoi(chan.c_str());

	if( dlog.debugging(Debug::LEVEL2) )
		dlog[Debug::LEVEL2] << myname << "(readItem): " << p << endl; 

	return true;
}
// -----------------------------------------------------------------------------
void RTUExchange::initIterators()
{
	RSMap::iterator it=rsmap.begin();
	for( ; it!=rsmap.end(); it++ )
	{
		shm->initDIterator(it->dit);
		shm->initAIterator(it->ait);
	}

	shm->initAIterator(aitHeartBeat);
	shm->initDIterator(ditNotRespond);
}
// -----------------------------------------------------------------------------
void RTUExchange::help_print( int argc, char* argv[] )
{
	cout << "--rs-polltime msec     - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--rs-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--rs-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--rs-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;    
	cout << "--rs-force             - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
	cout << "--rs-initPause		- Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--rs-notRespondSensor - датчик связи для данного процесса " << endl;
	cout << "--rs-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола RS: " << endl;
	cout << "--rs-dev devname  - файл устройства" << endl;
	cout << "--rs-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;
	cout << "--rs-my-addr      - адрес текущего узла" << endl;
	cout << "--rs-recv-timeout - Таймаут на ожидание ответа." << endl;
}
// -----------------------------------------------------------------------------
RTUExchange* RTUExchange::init_rtuexchange( int argc, char* argv[], UniSetTypes::ObjectId icID, SharedMemory* ic )
{
	string name = conf->getArgParam("--rs-name","RTUExchange1");
	if( name.empty() )
	{
		cerr << "(rtuexchange): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(rtuexchange): идентификатор '" << name 
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(rtuexchange): name = " << name << "(" << ID << ")" << endl;
	return new RTUExchange(ID,icID,ic);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const RTUExchange::DeviceType& dt )
{
	switch(dt)
	{
		case RTUExchange::dtRTU:
			os << "RTU";
		break;

		case RTUExchange::dtRTU188:
			os << "RTU188";
		break;

		case RTUExchange::dtMTR:
			os << "MTR";
		break;
		
		default:
			os << "Unknown device type";
		break;
	}
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, RTUExchange::RSProperty& p )
{
	os << " mbaddr=(" << (int)p.mbaddr << ")" << ModbusRTU::addr2str(p.mbaddr)
				<< " mbreg=" << ModbusRTU::dat2str(p.mbreg)
				<< " mbfunc=" << p.mbfunc
				<< " devtype=" << p.devtype;

	switch(p.devtype)
	{
		case RTUExchange::dtRTU:
			os << " nbit=" << p.nbit;
		break;

		case RTUExchange::dtRTU188:
			os << " jack=" << RTUStorage::j2s(p.rtuJack)
				<< " chan=" << p.rtuChan;
		break;

		case RTUExchange::dtMTR:
			os << " mtrType=" << MTR::type2str(p.mtrType);
		break;
		
		default:
			os << "Unknown type parameters ???!!!" << endl;
		break;
	}
	
	os 	<< " sid=" << p.si.id
		<< " stype=" << p.stype
		<< " safety=" << p.safety
		<< " invert=" << p.invert;

	if( p.stype == UniversalIO::AnalogInput || p.stype == UniversalIO::AnalogOutput )
	{
		os 	<< " rmin=" << p.cal.minRaw
			<< " rmax=" << p.cal.maxRaw
			<< " cmin=" << p.cal.maxCal
			<< " cmax=" << p.cal.maxCal
			<< " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
	}		
	
	return os;
}
// -----------------------------------------------------------------------------
