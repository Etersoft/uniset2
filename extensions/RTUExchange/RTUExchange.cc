// -----------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "RTUExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
RTUExchange::RTUExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic ):
UniSetObject_LT(objId),
mb(0),
defSpeed(ComPort::ComSpeed0),
use485F(false),
transmitCtl(false),
shm(0),
initPause(0),
force(false),
force_out(false),
mbregFromID(false),
activated(false),
rs_pre_clean(false),
noQueryOptimization(false),
allNotRespond(false)
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
	devname	= conf->getArgParam("--rs-dev",it.getProp("device"));
	if( devname.empty() )
		throw UniSetTypes::SystemError(myname+"(RTUExchange): Unknown device..." );

	string speed = conf->getArgParam("--rs-speed",it.getProp("speed"));
	if( speed.empty() )
		speed = "38400";

	use485F = conf->getArgInt("--rs-use485F",it.getProp("use485F"));
	transmitCtl = conf->getArgInt("--rs-transmit-ctl",it.getProp("transmitCtl"));
	defSpeed = ComPort::getSpeed(speed);

	recv_timeout = conf->getArgPInt("--rs-recv-timeout",it.getProp("recv_timeout"), 50);

	int alltout = conf->getArgPInt("--rs-all-timeout",it.getProp("all_timeout"), 2000);
		
	ptAllNotRespond.setTiming(alltout);

	rs_pre_clean = conf->getArgInt("--rs-pre-clean",it.getProp("pre_clean"));
	noQueryOptimization = conf->getArgInt("--rs-no-query-optimization",it.getProp("no_query_optimization"));

	mbregFromID = conf->getArgInt("--mbs-reg-from-id",it.getProp("reg_from_id"));
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	polltime = conf->getArgPInt("--rs-polltime",it.getProp("polltime"), 100);

	initPause = conf->getArgPInt("--rs-initPause",it.getProp("initPause"), 3000);

	force = conf->getArgInt("--rs-force",it.getProp("force"));
	force_out = conf->getArgInt("--rs-force-out",it.getProp("force_out"));

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
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
			err << myname << ": ID not found ('HeartBeat') for " << heart;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = getHeartBeatTime();
		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--rs-heartbeat-max",it.getProp("heartbeat_max"), 10);
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

	activateTimeout	= conf->getArgPInt("--activate-timeout", 20000);

	initMB(false);

	printMap(rmap);
//	abort();
}
// -----------------------------------------------------------------------------
RTUExchange::~RTUExchange()
{
	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		if( it1->second->rtu )
		{
			delete it1->second->rtu;
			it1->second->rtu = 0;
		}
		
		RTUDevice* d(it1->second);
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
			delete it->second;

		delete it1->second;
	}
	
	delete mb;
	delete shm;
}
// -----------------------------------------------------------------------------
void RTUExchange::initMB( bool reopen )
{
	if( !file_exist(devname) )
	{
		if( mb )
		{
			delete mb;
			mb = 0;
		}
		return;
	}

	if( mb )
	{
		if( !reopen )
			return;
		
		delete mb;
		mb = 0;
	}

	try
	{
		mb = new ModbusRTUMaster(devname,use485F,transmitCtl);

		if( defSpeed != ComPort::ComSpeed0 )
			mb->setSpeed(defSpeed);
	
//		mb->setLog(dlog);

		if( recv_timeout > 0 )
			mb->setTimeout(recv_timeout);

		dlog[Debug::INFO] << myname << "(init): dev=" << devname << " speed=" << ComPort::getSpeed(defSpeed) << endl;
	}
	catch(...)
	{
		if( mb )
			delete mb;
		mb = 0;
	}
}
// -----------------------------------------------------------------------------
void RTUExchange::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--rs-sm-ready-timeout","15000");
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
	if( trAllNotRespond.hi(allNotRespond) )
		ptAllNotRespond.reset();
	
	if( allNotRespond && mb && ptAllNotRespond.checkTime() )
	{
		ptAllNotRespond.reset();
		initMB(true);
	}
	
	if( !mb )
	{
		initMB(false);
		if( !mb )
		{
			for( RTUExchange::RTUDeviceMap::iterator it=rmap.begin(); it!=rmap.end(); ++it )
				it->second->resp_real = false;
		}
		updateSM();
		return;
	}

	ComPort::Speed s = mb->getSpeed();
	
	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
	
		if( d->speed != s )
		{
			s = d->speed;
			mb->setSpeed(d->speed);
		}
		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr) << endl;
	
		if( d->dtype==RTUExchange::dtRTU188 )
		{
			if( !d->rtu )
				continue;

			if( dlog.debugging(Debug::INFO) )
			{
				dlog[Debug::INFO] << myname << "(pollRTU188): poll RTU188 "
					<< " mbaddr=" << ModbusRTU::addr2str(d->mbaddr)
					<< endl;
			}

			try
			{
				if( rs_pre_clean )
					mb->cleanupChannel();

				d->rtu->poll(mb);
				d->resp_real = true;
			}
			catch( ModbusRTU::mbException& ex )
			{ 
				if( d->resp_real )
				{
					dlog[Debug::CRIT] << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr) 
						<< " -> " << ex << endl;
					d->resp_real = false;
				}
			}
		}
		else 
		{
			d->resp_real = false;
			for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
			{
				try
				{
					if( d->dtype==RTUExchange::dtRTU || d->dtype==RTUExchange::dtMTR )
					{
						if( rs_pre_clean )
							mb->cleanupChannel();
						if( pollRTU(d,it) )
							d->resp_real = true;
					}
				}
				catch( ModbusRTU::mbException& ex )
				{ 
					if( d->resp_real )
					{
						dlog[Debug::CRIT] << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr) 
							<< " reg=" << ModbusRTU::dat2str(it->second->mbreg)
							<< " -> " << ex << endl;
				//		d->resp_real = false;
					}
				}

				if( it==d->regmap.end() )
					break;
			}
		}
	}

	// update SharedMemory...
	updateSM();
	
	// check thresholds
	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			RegInfo* r(it->second);
			for( PList::iterator i=r->slst.begin(); i!=r->slst.end(); ++i )
				IOBase::processingThreshold( &(*i),shm,force);
		}
	}
	
//	printMap(rmap);
}
// -----------------------------------------------------------------------------
bool RTUExchange::pollRTU( RTUDevice* dev, RegMap::iterator& it )
{
	RegInfo* p(it->second);

	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(pollRTU): poll "
			<< " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
			<< " mbreg=" << ModbusRTU::dat2str(p->mbreg)
			<< " mboffset=" << p->offset
			<< " mbfunc=" << p->mbfunc
			<< " q_count=" << p->q_count
			<< endl;
	}
	
	if( p->q_count == 0 )
	{
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(pollRTU): q_count=0 for mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE register..." << endl;
		return false;
	}
	
	switch( p->mbfunc )
	{
		case ModbusRTU::fnReadInputRegisters:
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(dev->mbaddr,p->mbreg+p->offset,p->q_count);
			for( int i=0; i<p->q_count; i++,it++ )
				it->second->mbval = ret.data[i];
			it--;
		}
		break;

		case ModbusRTU::fnReadOutputRegisters:
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(dev->mbaddr, p->mbreg+p->offset,p->q_count);
			for( int i=0; i<p->q_count; i++,it++ )
				it->second->mbval = ret.data[i];
			it--;
		}
		break;
		
		case ModbusRTU::fnReadInputStatus:
		{
			ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(dev->mbaddr,p->mbreg+p->offset,p->q_count);
			int m=0;
			for( int i=0; i<ret.bcnt; i++ )
			{
				ModbusRTU::DataBits b(ret.data[i]);
				for( int k=0;k<ModbusRTU::BitsPerByte && m<p->q_count; k++,it++,m++ )
					it->second->mbval = b[k];
			}
			it--;
		}
		break;
		
		case ModbusRTU::fnReadCoilStatus:
		{
			ModbusRTU::ReadCoilRetMessage ret = mb->read01(dev->mbaddr,p->mbreg+p->offset,p->q_count);
			int m = 0;
			for( int i=0; i<ret.bcnt; i++ )
			{
				ModbusRTU::DataBits b(ret.data[i]);
				for( int k=0;k<ModbusRTU::BitsPerByte && m<p->q_count; k++,it++,m++ )
					it->second->mbval = b[k] ? 1 : 0;
			}
			it--;
		}
		break;
		
		case ModbusRTU::fnWriteOutputSingleRegister:
		{
			if( p->q_count != 1 )
			{
				dlog[Debug::CRIT] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE WRITE SINGLE REGISTER (0x06) q_count=" << p->q_count << " ..." << endl;
				return false;
			}
			ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06(dev->mbaddr,p->mbreg+p->offset,p->mbval);
		}
		break;

		case ModbusRTU::fnWriteOutputRegisters:
		{
			ModbusRTU::WriteOutputMessage msg(dev->mbaddr,p->mbreg+p->offset);
			for( int i=0; i<p->q_count; i++,it++ )
				msg.addData(it->second->mbval);
			it--;
			ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		}
		break;

		case ModbusRTU::fnForceSingleCoil:
		{
			if( p->q_count != 1 )
			{
				dlog[Debug::CRIT] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE FORCE SINGLE COIL (0x05) q_count=" << p->q_count << " ..." << endl;
				return false;
			}

			ModbusRTU::ForceSingleCoilRetMessage ret = mb->write05(dev->mbaddr,p->mbreg+p->offset,p->mbval);
		}
		break;

		case ModbusRTU::fnForceMultipleCoils:
		{
				ModbusRTU::ForceCoilsMessage msg(dev->mbaddr,p->mbreg+p->offset);
				for( int i=0; i<p->q_count; i++,it++ )
					msg.addBit( (it->second->mbval ? true : false) );

				it--;
//				cerr << "*********** (write multiple): " << msg << endl;
				ModbusRTU::ForceCoilsRetMessage ret = mb->write0F(msg);
		}
		break;
		
		default:
		{
			if( dlog.debugging(Debug::WARN) )
				dlog[Debug::WARN] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE mfunc=" << (int)p->mbfunc << " ..." << endl;
			return false;
		}
		break;
	}
	
	return true;
}
// -----------------------------------------------------------------------------
bool RTUExchange::RTUDevice::checkRespond()
{
	bool prev = resp_state;
	if( resp_trTimeout.change(resp_real) )
	{	
		if( resp_real )
			resp_state = true;

		resp_ptTimeout.reset();
	}

	if( resp_state && !resp_real && resp_ptTimeout.checkTime() )
		resp_state = false; 
	
	// если ещё не инициализировали значение в SM
	// то возвращаем true, чтобы оно принудительно сохранилось
	if( !resp_init )
	{
		resp_init = true;
		return true;
	}

	return ( prev != resp_state );
}
// -----------------------------------------------------------------------------
void RTUExchange::updateSM()
{
	allNotRespond = true;
	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		
//		cout << "check respond addr=" << ModbusRTU::addr2str(d->mbaddr) 
//			<< " respond=" << d->resp_id 
//			<< " real=" << d->resp_real
//			<< " state=" << d->resp_state
//			<< endl;

		if( d->resp_real )
			allNotRespond = false;
				
		// update respond sensors...
		if( d->checkRespond() && d->resp_id != DefaultObjectId  )
		{
			try
			{
				bool set = d->resp_invert ? !d->resp_state : d->resp_state;
				shm->localSaveState(d->resp_dit,d->resp_id,set,getId());
			}
			catch(Exception& ex)
			{
				dlog[Debug::CRIT] << myname
					<< "(step): (respond) " << ex << std::endl;
			}
		}

//		cerr << "*********** allNotRespond=" << allNotRespond << endl;

		// update values...
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			try
			{
				if( d->dtype == dtRTU )
					updateRTU(it);
				else if( d->dtype == dtMTR )
					updateMTR(it);
				else if( d->dtype == dtRTU188 )
					updateRTU188(it);
			}
			catch(IOController_i::NameNotFound &ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM):(NameNotFound) " << ex.err << endl;
			}
			catch(IOController_i::IOBadParam& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM):(IOBadParam) " << ex.err << endl;
			}
			catch(IONotifyController_i::BadRange )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): (BadRange)..." << endl;
			}
			catch( Exception& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): " << ex << endl;
			}
			catch(CORBA::SystemException& ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): CORBA::SystemException: "
					<< ex.NP_minorString() << endl;
			}
			catch(...)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): catch ..." << endl;
			}
			
			if( it==d->regmap.end() )
				break;
		}
	}
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
			}
			break;
		
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
			if( rmap.empty() )
			{
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* rmap EMPTY! terminated... *************" << endl;
				raise(SIGTERM);
				return; 
			}

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << "(sysCommand): rmap size= " << rmap.size() << endl;

			if( !shm->isLocalwork() )
				initDeviceList();
		
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

			// начальная инициализация
			if( !force )
			{
				uniset_mutex_lock l(pollMutex,2000);
				force = true;
				poll();
				force = false;
			}
			askTimer(tmExchange,polltime);
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
				unideb.logFile(fname);
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}
			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname);
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

	if( force_out )
		return;

	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			if( !isWriteFunction(it->second->mbfunc) )
				continue;
		
			for( PList::iterator i=it->second->slst.begin(); i!=it->second->slst.end(); ++i )
			{
				try
				{
					shm->askSensor(i->si.id,cmd);
				}
				catch( UniSetTypes::Exception& ex )
				{
					dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
				}
				catch(...)
				{
					dlog[Debug::WARN] << myname << "(askSensors): catch..." << std::endl;
				}
			}
		}
	}
}
// ------------------------------------------------------------------------------------------
void RTUExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	if( force_out )
		return;

	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			if( !isWriteFunction(it->second->mbfunc) )
				continue;
		
			for( PList::iterator i=it->second->slst.begin(); i!=it->second->slst.end(); ++i )
			{
				if( sm->id == i->si.id && sm->node == i->si.node )
				{
					if( dlog.debugging(Debug::INFO) )
					{
						dlog[Debug::INFO] << myname<< "(sensorInfo): si.id=" << sm->id 
							<< " reg=" << ModbusRTU::dat2str(i->reg->mbreg)
							<< " val=" << sm->value << endl;
					}

					i->value = sm->value;
					updateRSProperty( &(*i),true);
					return;
				}
			}
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
		if( !shm->isLocalwork() )
			rtuQueryOptimization(rmap);
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
/*
	RSMap::iterator it=rsmap.begin();
	for( ; it!=rsmap.end(); ++it )
	{
//		if( it->stype!=UniversalIO::DigitalOutput && it->stype!=UniversalIO::AnalogOutput )
//			continue;
		
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
*/	
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
RTUExchange::RTUDevice* RTUExchange::addDev( RTUDeviceMap& mp, ModbusRTU::ModbusAddr a, UniXML_iterator& xmlit )
{
	RTUDeviceMap::iterator it = mp.find(a);
	if( it != mp.end() )
	{
		DeviceType dtype = getDeviceType(xmlit.getProp("mbtype"));
		if( it->second->dtype != dtype )
		{
			dlog[Debug::CRIT] << myname << "(addDev): OTHER mbtype=" << dtype << " for " << xmlit.getProp("name")
				<< ". Already used devtype=" <<  it->second->dtype 
				<< " for mbaddr=" << ModbusRTU::addr2str(it->second->mbaddr)
				<< endl;
			return 0;
		}

		dlog[Debug::INFO] << myname << "(addDev): device for addr=" << ModbusRTU::addr2str(a) 
			<< " already added. Ignore device params for " << xmlit.getProp("name") << " ..." << endl;
		return it->second;
	}

	RTUExchange::RTUDevice* d = new RTUExchange::RTUDevice();
	d->mbaddr = a;

	if( !initRTUDevice(d,xmlit) )
	{	
		delete d;
		return 0;
	}

	mp.insert(RTUDeviceMap::value_type(a,d));
	return d;
}
// ------------------------------------------------------------------------------------------
RTUExchange::RegInfo* RTUExchange::addReg( RegMap& mp, ModbusRTU::ModbusData r, 
											UniXML_iterator& xmlit, RTUExchange::RTUDevice* dev,
											RTUExchange::RegInfo* rcopy )
{
	RegMap::iterator it = mp.find(r);
	if( it != mp.end() )
	{
		if( !it->second->dev )
		{
			dlog[Debug::CRIT] << myname << "(addReg): for reg=" << ModbusRTU::dat2str(r) 
				<< " dev=0!!!! " << endl;
			return 0;
		}
	
		if( it->second->dev->dtype != dev->dtype )
		{
			dlog[Debug::CRIT] << myname << "(addReg): OTHER mbtype=" << dev->dtype << " for reg=" << ModbusRTU::dat2str(r) 
				<< ". Already used devtype=" <<  it->second->dev->dtype << " for " << it->second->dev << endl;
			return 0;
		}
	
		if( dlog.debugging(Debug::INFO) )
		{
			dlog[Debug::INFO] << myname << "(addReg): reg=" << ModbusRTU::dat2str(r) 
				<< " already added. Ignore register params for " << xmlit.getProp("name") << " ..." << endl;
		}

		it->second->rit = it;
		return it->second;
	}
	
	RTUExchange::RegInfo* ri;
	if( rcopy )
	{
		ri = new RTUExchange::RegInfo(*rcopy);
		ri->slst.clear();
		ri->mbreg = r;
	}
	else
	{
		ri = new RTUExchange::RegInfo();
		if( !initRegInfo(ri,xmlit,dev) )
		{
			delete ri;
			return 0;
		}
		ri->mbreg = r;
	}
	
	mp.insert(RegMap::value_type(r,ri));
	ri->rit = mp.find(r);
	
	return ri;
}
// ------------------------------------------------------------------------------------------
RTUExchange::RSProperty* RTUExchange::addProp( PList& plist, RSProperty& p )
{
	for( PList::iterator it=plist.begin(); it!=plist.end(); ++it )
	{
		if( it->si.id == p.si.id && it->si.node == p.si.node )
			return &(*it);
	}
	
	plist.push_back(p);
	PList::iterator it = plist.end();
	it--;
	return &(*it);
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::initRSProperty( RSProperty& p, UniXML_iterator& it )
{
	if( !IOBase::initItem(&p,it,shm,&dlog,myname) )
		return false;
	
	string sbit(it.getProp("nbit"));
	if( !sbit.empty() )
	{
		p.nbit = UniSetTypes::uni_atoi(sbit);
		if( p.nbit < 0 || p.nbit >= ModbusRTU::BitsPerData )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): BAD nbit=" << p.nbit 
				<< ". (0 >= nbit < " << ModbusRTU::BitsPerData <<")." << endl;
			return false;
		}
	}
	
	if( p.nbit > 0 && 
		( p.stype == UniversalIO::AnalogInput ||
			p.stype == UniversalIO::AnalogOutput ) )
	{
		dlog[Debug::WARN] << "(initRSProperty): (ignore) uncorrect param`s nbit>1 (" << p.nbit << ")"
			<< " but iotype=" << p.stype << " for " << it.getProp("name") << endl;
	}

	string sbyte(it.getProp("nbyte"));
	if( !sbyte.empty() )
	{
		p.nbyte = UniSetTypes::uni_atoi(sbyte);
		if( p.nbyte < 0 || p.nbyte > VTypes::Byte::bsize )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): BAD nbyte=" << p.nbyte 
				<< ". (0 >= nbyte < " << VTypes::Byte::bsize << ")." << endl;
			return false;
		}
	}
	
	string vt(it.getProp("vtype"));
	if( vt.empty() )
	{
		p.rnum = VTypes::wsize(VTypes::vtUnknown);
		p.vType = VTypes::vtUnknown;
	}
	else
	{
		VTypes::VType v(VTypes::str2type(vt));
		if( v == VTypes::vtUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): Unknown rtuVType=" << vt << " for " 
					<< it.getProp("name") 
					<< endl;

			return false;
		}

		p.vType = v;
		p.rnum = VTypes::wsize(v);
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::initRegInfo( RegInfo* r, UniXML_iterator& it,  RTUExchange::RTUDevice* dev )
{
	r->dev = dev;
	r->mbval = it.getIntProp("default");
	r->offset= it.getIntProp("mboffset");

	if( dev->dtype == RTUExchange::dtMTR )
	{
		// only for MTR
		if( !initMTRitem(it,r) )
			return false;
	}
	else if( dev->dtype == RTUExchange::dtRTU188 )
	{	// only for RTU188
		if( !initRTU188item(it,r) )
			return false;
	}
	else if( dev->dtype == RTUExchange::dtRTU )
	{
	}
	else
	{
		dlog[Debug::CRIT] << myname << "(initRegInfo): Unknown mbtype='" << dev->dtype
				<< "' for " << it.getProp("name") << endl;
		return false;
	}

	if( mbregFromID )
		r->mbreg = conf->getSensorID(it.getProp("name"));
	else if( dev->dtype != RTUExchange::dtRTU188 )
	{	
		string reg = it.getProp("mbreg");
		if( reg.empty() )
		{
			dlog[Debug::CRIT] << myname << "(initRegInfo): unknown mbreg for " << it.getProp("name") << endl;
			return false;
		}

		r->mbreg = ModbusRTU::str2mbData(reg);
	}
	else // if( dev->dtype == RTUExchange::dtRTU188 )
	{
		UniversalIO::IOTypes stype = UniSetTypes::getIOType(it.getProp("iotype"));
		r->mbreg = RTUStorage::getRegister(r->rtuJack,r->rtuChan,stype);
		if( r->mbreg == -1 )
		{
			dlog[Debug::CRIT] << myname << "(initRegInfo): (RTU188) unknown mbreg for " << it.getProp("name") << endl;
			return false;
		}
	}

	r->mbfunc 	= ModbusRTU::fnUnknown;
	string f = it.getProp("mbfunc");
	if( !f.empty() )
	{
		r->mbfunc = (ModbusRTU::SlaveFunctionCode)UniSetTypes::uni_atoi(f);
		if( r->mbfunc == ModbusRTU::fnUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initRegInfo): Unknown mbfunc ='" << f
					<< "' for " << it.getProp("name") << endl;
			return false;
		}
	}
	
	return true;
}
// ------------------------------------------------------------------------------------------
RTUExchange::DeviceType RTUExchange::getDeviceType( const std::string dtype )
{
	if( dtype.empty() )
		return dtUnknown;

	if( dtype == "mtr" || dtype == "MTR" )
		return dtMTR;
	
	if( dtype == "rtu" || dtype == "RTU" )
		return dtRTU;
	
	if ( dtype == "rtu188" || dtype == "RTU188" )
		return dtRTU188;

	return dtUnknown;

}
// ------------------------------------------------------------------------------------------
bool RTUExchange::initRTUDevice( RTUDevice* d, UniXML_iterator& it )
{
	d->dtype = getDeviceType(it.getProp("mbtype"));

	if( d->dtype == dtUnknown )
	{
		dlog[Debug::CRIT] << myname << "(initRTUDevice): Unknown mbtype=" << it.getProp("mbtype")
			<< ". Use: rtu | mtr | rtu188" 
			<< " for " << it.getProp("name") << endl;
		return false;
	}

	string addr = it.getProp("mbaddr");
	if( addr.empty() )
	{
		dlog[Debug::CRIT] << myname << "(initRTUDevice): Unknown mbaddr for " << it.getProp("name") << endl;
		return false;
	}

	d->speed = defSpeed;
	d->mbaddr = ModbusRTU::str2mbAddr(addr);
	return true;
}
// ------------------------------------------------------------------------------------------

bool RTUExchange::initItem( UniXML_iterator& it )
{
	RSProperty p;
	if( !initRSProperty(p,it) )
		return false;

	string addr = it.getProp("mbaddr");
	if( addr.empty() )
	{
		dlog[Debug::CRIT] << myname << "(initItem): Unknown mbaddr='" << addr << " for " << it.getProp("name") << endl;
		return false;
	}

	ModbusRTU::ModbusAddr mbaddr = ModbusRTU::str2mbAddr(addr);

	RTUDevice* dev = addDev(rmap,mbaddr,it);
	if( !dev )
	{
		dlog[Debug::CRIT] << myname << "(initItem): " << it.getProp("name") << " CAN`T ADD for polling!" << endl;
		return false;
	}

	ModbusRTU::ModbusData mbreg;

	if( mbregFromID )
		mbreg = p.si.id; // conf->getSensorID(it.getProp("name"));
	else if( dev->dtype != RTUExchange::dtRTU188 )
	{	
		string reg = it.getProp("mbreg");
		if( reg.empty() )
		{
			dlog[Debug::CRIT] << myname << "(initRegInfo): unknown mbreg for " << it.getProp("name") << endl;
			return false;
		}
		mbreg = ModbusRTU::str2mbData(reg);
	}
	else // if( dev->dtype == RTUExchange::dtRTU188 )
	{
		RegInfo rr;
		initRegInfo(&rr,it,dev);
		mbreg = RTUStorage::getRegister(rr.rtuJack,rr.rtuChan,p.stype);
		if( mbreg == -1 )
		{
			dlog[Debug::CRIT] << myname << "(initItem): unknown mbreg for " << it.getProp("name") << endl;
			return false;
		}
	}

	RegInfo* ri = addReg(dev->regmap,mbreg,it,dev);

	if( dev->dtype == dtMTR )
	{
		p.rnum = MTR::wsize(ri->mtrType);
		if( p.rnum <= 0 )
		{
			dlog[Debug::CRIT] << myname << "(initItem): unknown word size for " << it.getProp("name") << endl;
			return false;
		}
	}

	if( !ri )
		return false;
	ri->dev = dev;

	// п÷п═п·п▓п∙п═п п░!
	// п╣я│п╩п╦ я└я┐п╫п╨я├п╦я▐ п╫п╟ п╥п╟п©п╦я│я▄, я┌п╬ п╫п╟п╢п╬ п©я─п╬п╡п╣я─п╦я┌я▄
	// я┤я┌п╬ п╬п╢п╦п╫ п╦ я┌п╬я┌п╤п╣ я─п╣пЁп╦я│я┌я─ п╫п╣ п©п╣я─п╣п╥п╟п©п╦я┬я┐я┌ п╫п╣я│п╨п╬п╩я▄п╨п╬ п╢п╟я┌я┤п╦п╨п╬п╡
	// я█я┌п╬ п╡п╬п╥п╪п╬п╤п╫п╬ я┌п╬п╩я▄п╨п╬, п╣я│п╩п╦ п╬п╫п╦ п©п╦я┬я┐я┌ п╠п╦я┌я▀!!
	// п≤п╒п·п⌠:
	// п∙я│п╩п╦ п╢п╩я▐ я└я┐п╫п╨я├п╦п╧ п╥п╟п©п╦я│п╦ я│п©п╦я│п╬п╨ п╢п╟я┌я┤п╦п╨п╬п╡ п╫п╟ п╬п╢п╦п╫ я─п╣пЁп╦я│я┌я─ > 1
	// п╥п╫п╟я┤п╦я┌ п╡ я│п©п╦я│п╨п╣ п╪п╬пЁя┐я┌ п╠я▀я┌я▄ я┌п╬п╩я▄п╨п╬ п╠п╦я┌п╬п╡я▀п╣ п╢п╟я┌я┤п╦п╨п╦
	// п╦ п╣я│п╩п╦ п╦п╢я▒я┌ п©п╬п©я▀я┌п╨п╟ п╡п╫п╣я│я┌п╦ п╡ я│п©п╦я│п╬п╨ п╫п╣ п╠п╦я┌п╬п╡я▀п╧ п╢п╟я┌я┤п╦п╨ я┌п╬ п·п╗п≤п▒п п░!
	// п≤ п╫п╟п╬п╠п╬я─п╬я┌: п╣я│п╩п╦ п╦п╢я▒я┌ п©п╬п©я▀я┌п╨п╟ п╡п╫п╣я│я┌п╦ п╠п╦я┌п╬п╡я▀п╧ п╢п╟я┌я┤п╦п╨, п╟ п╡ я│п©п╦я│п╨п╣
	// я┐п╤п╣ я│п╦п╢п╦я┌ п╢п╟я┌я┤п╦п╨ п╥п╟п╫п╦п╪п╟я▌я┴п╦п╧ я├п╣п╩я▀п╧ я─п╣пЁп╦я│я┌я─, я┌п╬ я┌п╬п╤п╣ п·п╗п≤п▒п п░!
	if(	ri->mbfunc == ModbusRTU::fnWriteOutputRegisters ||
			ri->mbfunc == ModbusRTU::fnWriteOutputSingleRegister )
	{
		if( p.nbit<0 &&  ri->slst.size() > 1 )
		{
			dlog[Debug::CRIT] << myname << "(initItem): FAILED! Sharing SAVE (not bit saving) to "
				<< " mbreg=" << ModbusRTU::dat2str(ri->mbreg)
				<< " for " << it.getProp("name") << endl;

			abort(); 	// ABORT PROGRAM!!!!
			return false;
		}
		
		if( p.nbit >= 0 && ri->slst.size() == 1 )
		{
			PList::iterator it2 = ri->slst.begin();
			if( it2->nbit < 0 )
			{
					dlog[Debug::CRIT] << myname << "(initItem): FAILED! Sharing SAVE (mbreg=" 
						<< ModbusRTU::dat2str(ri->mbreg) << "  already used)!"
						<< " IGNORE --> " << it.getProp("name") << endl;
					abort(); 	// ABORT PROGRAM!!!!
					return false;
			}
		}
	}

	RSProperty* p1 = addProp(ri->slst,p);
	if( !p1 )
		return false;

	p1->reg = ri;

	if( p1->rnum > 1 )
	{
		for( int i=1; i<p1->rnum; i++ )
			addReg(dev->regmap,mbreg+i,it,dev,ri);
	}
	
	if( dev->dtype == dtRTU188 )
	{
		if( !dev->rtu )
			dev->rtu = new RTUStorage(mbaddr);
	}

	return true;
}

// ------------------------------------------------------------------------------------------
bool RTUExchange::initMTRitem( UniXML_iterator& it, RegInfo* p )
{
	p->mtrType = MTR::str2type(it.getProp("mtrtype"));
	if( p->mtrType == MTR::mtUnknown )
	{
		dlog[Debug::CRIT] << myname << "(readMTRItem): Unknown mtrtype '" 
					<< it.getProp("mtrtype")
					<< "' for " << it.getProp("name") << endl;

		return false;
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool RTUExchange::initRTU188item( UniXML_iterator& it, RegInfo* p )
{
	string jack = it.getProp("jack");
	string chan	= it.getProp("channel");
	
	if( jack.empty() )
	{
		dlog[Debug::CRIT] << myname << "(readRTU188Item): Unknown jack='' "
					<< " for " << it.getProp("name") << endl;
		return false;
	}
	p->rtuJack = RTUStorage::s2j(jack);
	if( p->rtuJack == RTUStorage::nUnknown )
	{
		dlog[Debug::CRIT] << myname << "(readRTU188Item): Unknown jack=" << jack
					<< " for " << it.getProp("name") << endl;
		return false;
	}

	if( chan.empty() )
	{
		dlog[Debug::CRIT] << myname << "(readRTU188Item): Unknown channel='' "
					<< " for " << it.getProp("name") << endl;
		return false;
	}
	
	p->rtuChan = UniSetTypes::uni_atoi(chan);

	if( dlog.debugging(Debug::LEVEL2) )
		dlog[Debug::LEVEL2] << myname << "(readRTU188Item): " << p << endl; 

	return true;
}
// -----------------------------------------------------------------------------
void RTUExchange::initIterators()
{
	shm->initAIterator(aitHeartBeat);

	for( RTUExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		shm->initDIterator(d->resp_dit);
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			for( PList::iterator it2=it->second->slst.begin();it2!=it->second->slst.end(); ++it2 )
			{
				shm->initDIterator(it2->dit);
				shm->initAIterator(it2->ait);
			}
		}
	}

}
// -----------------------------------------------------------------------------
void RTUExchange::help_print( int argc, const char* const* argv )
{
	cout << "--rs-polltime msec     - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--rs-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--rs-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--rs-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;    
	cout << "--rs-force             - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
	cout << "--rs-initPause		- Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--rs-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола RS: " << endl;
	cout << "--rs-dev devname  - файл устройства" << endl;
	cout << "--rs-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;
	cout << "--rs-my-addr      - адрес текущего узла" << endl;
	cout << "--rs-recv-timeout - Таймаут на ожидание ответа." << endl;
}
// -----------------------------------------------------------------------------
RTUExchange* RTUExchange::init_rtuexchange( int argc, const char* const* argv, UniSetTypes::ObjectId icID, SharedMemory* ic )
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
			os << "Unknown device type (" << (int)dt << ")";
		break;
	}
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const RTUExchange::RSProperty& p )
{
	os 	<< " (" << ModbusRTU::dat2str(p.reg->mbreg) << ")"
		<< " sid=" << p.si.id
		<< " stype=" << p.stype
	 	<< " nbit=" << p.nbit
	 	<< " nbyte=" << p.nbyte
		<< " rnum=" << p.rnum
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
void RTUExchange::initDeviceList()
{
	xmlNode* respNode = conf->findNode(cnode,"DeviceList");
	if( respNode )
	{
		UniXML_iterator it1(respNode);
		if( it1.goChildren() )
		{
			for(;it1.getCurrent(); it1.goNext() )
			{
				ModbusRTU::ModbusAddr a = ModbusRTU::str2mbAddr(it1.getProp("addr"));
				initDeviceInfo(rmap,a,it1);
			}
		}
		else
			dlog[Debug::WARN] << myname << "(init): <DeviceList> empty section..." << endl;
	}
	else
		dlog[Debug::WARN] << myname << "(init): <DeviceList> not found..." << endl;
}
// -----------------------------------------------------------------------------
bool RTUExchange::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it )
{
	RTUDeviceMap::iterator d = m.find(a);
	if( d == m.end() )
	{
		dlog[Debug::WARN] << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
		return false;
	}
	
	if( !it.getProp("respondSensor").empty() )
	{
		d->second->resp_id = conf->getSensorID(it.getProp("respondSensor"));
		if( d->second->resp_id == DefaultObjectId )
		{
			dlog[Debug::CRIT] << myname << "(initDeviceInfo): not found ID for noRespondSensor=" << it.getProp("respondSensor") << endl;
			return false;
		}
	}

	dlog[Debug::INFO] << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) << endl;
	int tout = it.getPIntProp("timeout", UniSetTimer::WaitUpTime);
	d->second->resp_ptTimeout.setTiming(tout);
	d->second->resp_invert = it.getIntProp("invert");

	string s = it.getProp("speed");
	if( !s.empty() )
	{
		d->second->speed = ComPort::getSpeed(s);
		if( d->second->speed == ComPort::ComSpeed0 )
		{
			d->second->speed = defSpeed;
			dlog[Debug::CRIT] << myname << "(initDeviceInfo): Unknown speed=" << s <<
				" for addr=" << ModbusRTU::addr2str(a) << endl;
			return false;
		}
	}

	return true;
}
// -----------------------------------------------------------------------------
void RTUExchange::printMap( RTUExchange::RTUDeviceMap& m )
{
	cout << "devices: num=" << m.size() << endl;
	for( RTUExchange::RTUDeviceMap::iterator it=m.begin(); it!=m.end(); ++it )
	{
		cout << "  " <<  *(it->second) << endl;
	}
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, RTUExchange::RTUDeviceMap& m )
{
	os << "devices: " << endl;
	for( RTUExchange::RTUDeviceMap::iterator it=m.begin(); it!=m.end(); ++it )
	{
		os << "  " <<  *(it->second) << endl;
	}
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, RTUExchange::RTUDevice& d )
{
  		os 	<< "addr=" << ModbusRTU::addr2str(d.mbaddr)
  			<< " type=" << d.dtype;

		os << " rtu=" << (d.rtu ? "yes" : "no" );

  		os	<< " respond_id=" << d.resp_id
  			<< " respond_timeout=" << d.resp_ptTimeout.getInterval()
  			<< " respond_state=" << d.resp_state
  			<< " respond_invert=" << d.resp_invert
  			<< endl;
  			

		os << "  regs: " << endl;
		for( RTUExchange::RegMap::iterator it=d.regmap.begin(); it!=d.regmap.end(); ++it )
			os << "     " << *(it->second) << endl;
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, RTUExchange::RegInfo& r )
{
	os << " mbreg=" << ModbusRTU::dat2str(r.mbreg)
		<< " mbfunc=" << r.mbfunc
	
		<< " mtrType=" << MTR::type2str(r.mtrType)
		<< " jack=" << RTUStorage::j2s(r.rtuJack)
		<< " chan=" << r.rtuChan
		<< " q_num=" << r.q_num
		<< " q_count=" << r.q_count
		<< " value=" << ModbusRTU::dat2str(r.mbval) << "(" << (int)r.mbval << ")"
		<< endl;

	for( RTUExchange::PList::iterator it=r.slst.begin(); it!=r.slst.end(); ++it )
		os << "         " << (*it) << endl;

	return os;
}
// -----------------------------------------------------------------------------
void RTUExchange::rtuQueryOptimization( RTUDeviceMap& m )
{
	if( noQueryOptimization )
		return;

	dlog[Debug::INFO] << myname << "(rtuQueryOptimization): optimization..." << endl;

	// MAXLEN/2 - я█я┌п╬ п╨п╬п╩п╦я┤п╣я│я┌п╡п╬ я│п╩п╬п╡ п╢п╟п╫п╫я▀я┘ п╡ п╬я┌п╡п╣я┌п╣
	// 10 - п╫п╟ п╡я│я▐п╨п╦п╣ я│п╩я┐п╤п╣п╠п╫я▀п╣ п╥п╟пЁп╬п╩п╬п╡п╨п╦
	int maxcount = ModbusRTU::MAXLENPACKET/2 - 10;

	for( RTUExchange::RTUDeviceMap::iterator it1=m.begin(); it1!=m.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( RTUExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			RTUExchange::RegMap::iterator beg = it;
			ModbusRTU::ModbusData reg = it->second->mbreg + it->second->offset;

			beg->second->q_num = 1;
			beg->second->q_count = 1;
			it++;
			for( ;it!=d->regmap.end(); ++it )
			{
				if( (it->second->mbreg + it->second->offset - reg) > 1 )
					break;
				
				if( beg->second->mbfunc != it->second->mbfunc )
					break;

				beg->second->q_count++;
				if( beg->second->q_count > maxcount )
					break;

				reg = it->second->mbreg + it->second->offset;
				it->second->q_num = beg->second->q_count;
				it->second->q_count = 0;
			}

			// check correct function...
			if( beg->second->q_count>1 && beg->second->mbfunc==ModbusRTU::fnWriteOutputSingleRegister )
			{
				dlog[Debug::WARN] << myname << "(rtuQueryOptimization): "
					<< " optimization change func=" << ModbusRTU::fnWriteOutputSingleRegister
					<< " <--> func=" << ModbusRTU::fnWriteOutputRegisters
					<< " for mbaddr=" << ModbusRTU::addr2str(d->mbaddr)
					<< " mbreg=" << ModbusRTU::dat2str(beg->second->mbreg);
				
				beg->second->mbfunc = ModbusRTU::fnWriteOutputRegisters;
			}
			else if( beg->second->q_count>1 && beg->second->mbfunc==ModbusRTU::fnForceSingleCoil )
			{
				dlog[Debug::WARN] << myname << "(rtuQueryOptimization): "
					<< " optimization change func=" << ModbusRTU::fnForceSingleCoil
					<< " <--> func=" << ModbusRTU::fnForceMultipleCoils
					<< " for mbaddr=" << ModbusRTU::addr2str(d->mbaddr)
					<< " mbreg=" << ModbusRTU::dat2str(beg->second->mbreg);
				
				beg->second->mbfunc = ModbusRTU::fnForceMultipleCoils;
			}
			
			if( it==d->regmap.end() )
				break;

			it--;
		}
	}
}
// -----------------------------------------------------------------------------
void RTUExchange::updateRTU( RegMap::iterator& rit )
{
	RegInfo* r(rit->second);
	for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
		updateRSProperty( &(*it),false );
}
// -----------------------------------------------------------------------------
void RTUExchange::updateRSProperty( RSProperty* p, bool write_only )
{
	using namespace ModbusRTU;
	RegInfo* r(p->reg->rit->second);

	bool save = isWriteFunction( r->mbfunc );
	
	if( !save && write_only )
		return;

		try
		{
			if( p->vType == VTypes::vtUnknown )
			{
				ModbusRTU::DataBits16 b(r->mbval);
				if( p->nbit >= 0 )
				{
					if( save )
					{
						bool set = IOBase::processingAsDO( p, shm, force_out );
						b.set(p->nbit,set);
						r->mbval = b.mdata();
					}
					else
					{
						bool set = b[p->nbit];
						IOBase::processingAsDI( p, set, shm, force );
					}
					return;
				}

				if( p->rnum <= 1 )
				{
					if( save )
					{
						if( p->stype == UniversalIO::DigitalInput ||
							p->stype == UniversalIO::DigitalOutput )
						{
							r->mbval = IOBase::processingAsDO( p, shm, force_out );
						}
						else  
							r->mbval = IOBase::processingAsAO( p, shm, force_out );
					}
					else
					{
						if( p->stype == UniversalIO::DigitalInput ||
							p->stype == UniversalIO::DigitalOutput )
						{
							IOBase::processingAsDI( p, r->mbval, shm, force );
						}
						else
							IOBase::processingAsAI( p, (unsigned short)(r->mbval), shm, force );
					}
					return;
				}

				dlog[Debug::CRIT] << myname << "(updateRSProperty): IGNORE item: rnum=" << p->rnum 
						<< " > 1 ?!! for id=" << p->si.id << endl;
				return;
			}
			else if( p->vType == VTypes::vtSigned )
			{
				if( save )
				{
					if( p->stype == UniversalIO::DigitalInput ||
						p->stype == UniversalIO::DigitalOutput )
					{
						r->mbval = (signed short)IOBase::processingAsDO( p, shm, force_out );
					}
					else  
						r->mbval = (signed short)IOBase::processingAsAO( p, shm, force_out );
				}
				else
				{
					if( p->stype == UniversalIO::DigitalInput ||
						p->stype == UniversalIO::DigitalOutput )
					{
						IOBase::processingAsDI( p, r->mbval, shm, force );
					}
					else
					{
						long val = (signed short)r->mbval;
						IOBase::processingAsAI( p, val, shm, force );
					}
				}
				return;
			}
			else if( p->vType == VTypes::vtUnsigned )
			{
				if( save )
				{
					if( p->stype == UniversalIO::DigitalInput ||
						p->stype == UniversalIO::DigitalOutput )
					{
						r->mbval = (unsigned short)IOBase::processingAsDO( p, shm, force_out );
					}
					else  
						r->mbval = (unsigned short)IOBase::processingAsAO( p, shm, force_out );
				}
				else
				{
					if( p->stype == UniversalIO::DigitalInput ||
						p->stype == UniversalIO::DigitalOutput )
					{
						IOBase::processingAsDI( p, r->mbval, shm, force );
					}
					else
					{
						long val = (unsigned short)r->mbval;
						IOBase::processingAsAI( p, val, shm, force );
					}
				}
				return;
			}
			else if( p->vType == VTypes::vtByte )
			{
				if( p->nbyte <= 0 || p->nbyte > VTypes::Byte::bsize )
				{
					dlog[Debug::CRIT] << myname << "(updateRSProperty): IGNORE item: reg=" << ModbusRTU::dat2str(r->mbreg) 
							<< " vtype=" << p->vType << " but nbyte=" << p->nbyte << endl;
					return;
				}

				if( save )
				{
					long v = IOBase::processingAsAO( p, shm, force_out );
					VTypes::Byte b(r->mbval);
					b.raw.b[p->nbyte-1] = v;
					r->mbval = b.raw.w;
				}
				else
				{
					VTypes::Byte b(r->mbval);
					IOBase::processingAsAI( p, b.raw.b[p->nbyte-1], shm, force );
				}
				
				return;
			}
			else if( p->vType == VTypes::vtF2 )
			{
				RegMap::iterator i(p->reg->rit);
				if( save )
				{
					float f = IOBase::processingFasAO( p, shm, force_out );
					VTypes::F2 f2(f);
					for( int k=0; k<VTypes::F2::wsize(); k++, i++ )
						i->second->mbval = f2.raw.v[k];
				}
				else
				{
					ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::F2::wsize()];
					for( int k=0; k<VTypes::F2::wsize(); k++, i++ )
						data[k] = i->second->mbval;
				
					VTypes::F2 f(data,VTypes::F2::wsize());
					delete[] data;
				
					IOBase::processingFasAI( p, (float)f, shm, force );
				}
			}
			else if( p->vType == VTypes::vtF4 )
			{
				RegMap::iterator i(p->reg->rit);
				if( save )
				{
					float f = IOBase::processingFasAO( p, shm, force_out );
					VTypes::F4 f4(f);
					for( int k=0; k<VTypes::F4::wsize(); k++, i++ )
						i->second->mbval = f4.raw.v[k];
				}
				else
				{
					ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::F4::wsize()];
					for( int k=0; k<VTypes::F4::wsize(); k++, i++ )
						data[k] = i->second->mbval;
				
					VTypes::F4 f(data,VTypes::F4::wsize());
					delete[] data;
				
					IOBase::processingFasAI( p, (float)f, shm, force );
				}
			}
		}
		catch(IOController_i::NameNotFound &ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty):(NameNotFound) " << ex.err << endl;
		}
		catch(IOController_i::IOBadParam& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty):(IOBadParam) " << ex.err << endl;
		}
		catch(IONotifyController_i::BadRange )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): (BadRange)..." << endl;
		}
		catch( Exception& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): " << ex << endl;
		}
		catch(CORBA::SystemException& ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): CORBA::SystemException: "
				<< ex.NP_minorString() << endl;
		}
		catch(...)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): catch ..." << endl;
		}
}
// -----------------------------------------------------------------------------

void RTUExchange::updateMTR( RegMap::iterator& rit )
{
	RegInfo* r(rit->second);
	using namespace ModbusRTU;
	bool save = isWriteFunction( r->mbfunc );

	{
		for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
		{
			try
			{
				if( r->mtrType == MTR::mtT1 )
				{
					if( save )
						r->mbval = IOBase::processingAsAO( &(*it), shm, force_out );
					else
					{
						MTR::T1 t(r->mbval);
						IOBase::processingAsAI( &(*it), t.val, shm, force );
					}
					continue;
				}
			
				if( r->mtrType == MTR::mtT2 )
				{
					if( save )
					{
						MTR::T2 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						r->mbval = t.val; 
					}
					else
					{
						MTR::T2 t(r->mbval);
						IOBase::processingAsAI( &(*it), t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT3 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T3 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T3::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T3::wsize()];
						for( int k=0; k<MTR::T3::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T3 t(data,MTR::T3::wsize());
						delete[] data;
						IOBase::processingAsAI( &(*it), (long)t, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT4 )
				{
					if( save )
						cerr << myname << "(updateMTR): write (T4) reg(" << dat2str(r->mbreg) << ") to MTR NOT YET!!!" << endl;
					else
					{
						MTR::T4 t(r->mbval);
						IOBase::processingAsAI( &(*it), uni_atoi(t.sval), shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT5 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T5 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T5::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T5::wsize()];
						for( int k=0; k<MTR::T5::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T5 t(data,MTR::T5::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT6 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T6 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T6::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T6::wsize()];
						for( int k=0; k<MTR::T6::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T6 t(data,MTR::T6::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT7 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T7 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T7::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T7::wsize()];
						for( int k=0; k<MTR::T7::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T7 t(data,MTR::T7::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtF1 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						float f = IOBase::processingFasAO( &(*it), shm, force_out );
						MTR::F1 f1(f);
						for( int k=0; k<MTR::F1::wsize(); k++, i++ )
							i->second->mbval = f1.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::F1::wsize()];
						for( int k=0; k<MTR::F1::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::F1 t(data,MTR::F1::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t, shm, force );
					}
					continue;
				}
			}
			catch(IOController_i::NameNotFound &ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR):(NameNotFound) " << ex.err << endl;
			}
			catch(IOController_i::IOBadParam& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR):(IOBadParam) " << ex.err << endl;
			}
			catch(IONotifyController_i::BadRange )
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): (BadRange)..." << endl;
			}
			catch( Exception& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): " << ex << endl;
			}
			catch(CORBA::SystemException& ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): CORBA::SystemException: "
					<< ex.NP_minorString() << endl;
			}
			catch(...)
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): catch ..." << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void RTUExchange::updateRTU188( RegMap::iterator& it )
{
	RegInfo* r(it->second);
	if( !r->dev->rtu )
		return;

	using namespace ModbusRTU;

//	bool save = false;
	if( isWriteFunction(r->mbfunc) )
	{
//		save = true;
		cerr << myname << "(updateRTU188): write reg(" << dat2str(r->mbreg) << ") to RTU188 NOT YET!!!" << endl;
		return;
	}

	for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
	{
		try
		{
			if( it->stype == UniversalIO::DigitalInput )
			{
				bool set = r->dev->rtu->getState(r->rtuJack,r->rtuChan,it->stype);
				IOBase::processingAsDI( &(*it), set, shm, force );
				continue;
			}
	
			if( it->stype == UniversalIO::AnalogInput )
			{
				long val = r->dev->rtu->getInt(r->rtuJack,r->rtuChan,it->stype);
				IOBase::processingAsAI( &(*it),val, shm, force );
				continue;
			}
		}
		catch(IOController_i::NameNotFound &ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateMTR):(NameNotFound) " << ex.err << endl;
		}
		catch(IOController_i::IOBadParam& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateMTR):(IOBadParam) " << ex.err << endl;
		}
		catch(IONotifyController_i::BadRange )
		{
			dlog[Debug::LEVEL3] << myname << "(updateMTR): (BadRange)..." << endl;
		}
		catch( Exception& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateMTR): " << ex << endl;
		}
		catch(CORBA::SystemException& ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateMTR): CORBA::SystemException: "
				<< ex.NP_minorString() << endl;
		}
		catch(...)
		{
			dlog[Debug::LEVEL3] << myname << "(updateMTR): catch ..." << endl;
		}
	}
}
// -----------------------------------------------------------------------------
