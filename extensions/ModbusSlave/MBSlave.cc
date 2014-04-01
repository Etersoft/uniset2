#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "MBSlave.h"
#include "modbus/ModbusRTUSlaveSlot.h"
#include "modbus/ModbusTCPServerSlot.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
using namespace ModbusRTU;
// -----------------------------------------------------------------------------
MBSlave::MBSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, 
					SharedMemory* ic, string prefix ):
UniSetObject_LT(objId),
mbslot(0),
shm(0),
initPause(0),
test_id(DefaultObjectId),
askcount_id(DefaultObjectId),
respond_id(DefaultObjectId),
respond_invert(false),
askCount(0),
activated(false),
activateTimeout(500),
pingOK(true),
force(false),
mbregFromID(false),
prefix(prefix)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBSlave): objId=-1?!! Use --mbs-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBSlave): Not found conf-node for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dlog[Debug::INFO] << myname << "(init): read s_field='" << s_field
						<< "' s_fvalue='" << s_fvalue << "'" << endl;

	force = conf->getArgInt("--" + prefix + "-force",it.getProp("force"));

	// int recv_timeout = conf->getArgParam("--" + prefix + "-recv-timeout",it.getProp("recv_timeout")));

	string saddr = conf->getArgParam("--" + prefix + "-my-addr",it.getProp("addr"));

	if( saddr.empty() )
		addr = 0x01;
	else
		addr = ModbusRTU::str2mbAddr(saddr);

	mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id",it.getProp("reg_from_id"));
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	respond_id = conf->getSensorID(conf->getArgParam("--" + prefix + "-respond-id",it.getProp("respond_id")));
	respond_invert = conf->getArgInt("--" + prefix + "-respond-invert",it.getProp("respond_invert"));

	string stype = conf->getArgParam("--" + prefix + "-type",it.getProp("type"));
	
	if( stype == "RTU" )
	{
		// ---------- init RS ----------
		string dev	= conf->getArgParam("--" + prefix + "-dev",it.getProp("device"));
		if( dev.empty() )
			throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown device...");

		string speed 	= conf->getArgParam("--" + prefix + "-speed",it.getProp("speed"));
		if( speed.empty() )
			speed = "38400";

		bool use485F = conf->getArgInt("--rs-use485F",it.getProp("use485F"));
		bool transmitCtl = conf->getArgInt("--rs-transmit-ctl",it.getProp("transmitCtl"));

		ModbusRTUSlaveSlot* rs = new ModbusRTUSlaveSlot(dev,use485F,transmitCtl);
		rs->setSpeed(speed);
		rs->setRecvTimeout(2000);
//		rs->setAfterSendPause(afterSend);
//		rs->setReplyTimeout(replyTimeout);
		rs->setLog(dlog);

		mbslot = rs;
		thr = new ThreadCreator<MBSlave>(this,&MBSlave::execute_rtu);

		dlog[Debug::INFO] << myname << "(init): type=RTU myaddr=" << ModbusRTU::addr2str(addr) 
			<< " dev=" << dev << " speed=" << speed << endl;
	}
	else if( stype == "TCP" )
	{
		string iaddr = conf->getArgParam("--" + prefix + "-inet-addr",it.getProp("iaddr"));
		if( iaddr.empty() )
			throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown TCP server address. Use: --prefix-inet-addr [ XXX.XXX.XXX.XXX| hostname ]");
		
		int port = conf->getArgPInt("--" + prefix + "-inet-port",it.getProp("iport"), 502);

		dlog[Debug::INFO] << myname << "(init): type=TCP myaddr=" << ModbusRTU::addr2str(addr) 
			<< " inet=" << iaddr << " port=" << port << endl;
	
		ost::InetAddress ia(iaddr.c_str());
		mbslot	= new ModbusTCPServerSlot(ia,port);
		thr = new ThreadCreator<MBSlave>(this,&MBSlave::execute_tcp);

		dlog[Debug::INFO] << myname << "(init): init TCP connection ok. " << " inet=" << iaddr << " port=" << port << endl;
	}
	else
		throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown slave type. Use: --mbs-type [RTU|TCP]");

//	mbslot->connectReadCoil( sigc::mem_fun(this, &MBSlave::readCoilStatus) );
	mbslot->connectReadInputStatus( sigc::mem_fun(this, &MBSlave::readInputStatus) );
	mbslot->connectReadOutput( sigc::mem_fun(this, &MBSlave::readOutputRegisters) );
	mbslot->connectReadInput( sigc::mem_fun(this, &MBSlave::readInputRegisters) );
	mbslot->connectForceSingleCoil( sigc::mem_fun(this, &MBSlave::forceSingleCoil) );
	mbslot->connectForceCoils( sigc::mem_fun(this, &MBSlave::forceMultipleCoils) );
	mbslot->connectWriteOutput( sigc::mem_fun(this, &MBSlave::writeOutputRegisters) );
	mbslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBSlave::writeOutputSingleRegister) );
	mbslot->connectMEIRDI( sigc::mem_fun(this, &MBSlave::read4314) );

	if( findArgParam("--" + prefix + "-allow-setdatetime",conf->getArgc(),conf->getArgv())!=-1 )
		mbslot->connectSetDateTime( sigc::mem_fun(this, &MBSlave::setDateTime) );

	mbslot->connectDiagnostics( sigc::mem_fun(this, &MBSlave::diagnostics) );	
	mbslot->connectFileTransfer( sigc::mem_fun(this, &MBSlave::fileTransfer) );	

//	mbslot->connectJournalCommand( sigc::mem_fun(this, &MBSlave::journalCommand) );
//	mbslot->connectRemoteService( sigc::mem_fun(this, &MBSlave::remoteService) );
	// -------------------------------

	initPause = conf->getArgPInt("--" + prefix + "-initPause",it.getProp("initPause"), 3000);

	if( shm->isLocalwork() )
	{
		readConfiguration();
		dlog[Debug::INFO] << myname << "(init): iomap size = " << iomap.size() << endl;
	}
	else
	{
		ic->addReadItem( sigc::mem_fun(this,&MBSlave::readItem) );
		// при работе с SM через указатель принудительно включаем force
		force = true;
	}

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id",it.getProp("heartbeat_id"));
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

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max",it.getProp("heartbeat_max"), 10);
		test_id = sidHeartBeat;
	}
	else
	{
		test_id = conf->getSensorID("TestMode_S");
		if( test_id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": test_id unknown. 'TestMode_S' not found...";
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	askcount_id = conf->getSensorID(conf->getArgParam("--" + prefix + "-askcount-id",it.getProp("askcount_id")));
	dlog[Debug::INFO] << myname << ": init askcount_id=" << askcount_id << endl;

	dlog[Debug::INFO] << myname << ": init test_id=" << test_id << endl;

	wait_msec = getHeartBeatTime() - 100;
	if( wait_msec < 500 )
		wait_msec = 500;

	activateTimeout	= conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

	timeout_t msec = conf->getArgPInt("--" + prefix + "-timeout",it.getProp("timeout"), 3000);
	ptTimeout.setTiming(msec);

	dlog[Debug::INFO] << myname << "(init): rs-timeout=" << msec << " msec" << endl;


	// build file list...
	xmlNode* fnode = 0;
	UniXML* xml = conf->getConfXML();
	if( xml )
		fnode = xml->extFindNode(cnode,1,1,"filelist");

	if( fnode )
	{
		UniXML_iterator fit(fnode);
		if( fit.goChildren() )
		{
			for( ;fit.getCurrent(); fit.goNext() )
			{
				std::string nm = fit.getProp("name");
				if( nm.empty() )
				{
					dlog[Debug::WARN] << myname << "(build file list): ignore empty name... " << endl;
					continue;
				}
				int id = fit.getIntProp("id");
				if( id == 0 )
				{
					dlog[Debug::WARN] << myname << "(build file list): FAILED ID for " << nm << "... ignore..." << endl;
					continue;
				}
			
				std::string dir = fit.getProp("directory");
				if( !dir.empty() )
				{
					if( dir == "ConfDir" )
						nm = conf->getConfDir() + nm;
					else if( dir == "DataDir" )
						nm = conf->getDataDir() + nm;
					else
						nm = dir + nm;
				}

				dlog[Debug::INFO] << myname << "(init):       add to filelist: "
						<< "id=" << id
						<< " file=" << nm 
						<< endl;

				flist[id] = nm;
			}
		}
		else
			dlog[Debug::INFO] << myname << "(init): <filelist> empty..." << endl;
	}
	else
		dlog[Debug::INFO] << myname << "(init): <filelist> not found..." << endl;


	// Формирование "карты" ответов на запрос 0x2B(43)/0x0E(14)
	xmlNode* mnode = 0;
	if( xml )
		mnode = xml->extFindNode(cnode,1,1,"MEI");

	if( mnode )
	{
		// Считывается структура для формирования ответов на запрос 0x2B(43)/0x0E(14)
//		<MEI>
// 			  <device id="">
// 			     <objects id="">
//                      <string id="" value=""/>
//                      <string id="" value=""/>
//                      <string id="" value=""/>
//                        ...
// 			     </objects>
// 			  </device>
// 			  <device devID="">
// 				...
// 			  </device>
//      </MEI>
		UniXML_iterator dit(mnode);
		if( dit.goChildren() )
		{
			// Device ID list..
			for( ;dit.getCurrent(); dit.goNext() )
			{
				if( dit.getProp("id").empty() )
				{
					dlog[Debug::WARN] << myname << "(init): read <MEI>. Unknown <device id=''>. Ignore.." << endl; 
					continue;
				}

				int devID = dit.getIntProp("id");

				UniXML_iterator oit(dit);
				if( oit.goChildren() )
				{
					if( dlog.debugging(Debug::INFO) )
						dlog[Debug::INFO] << myname << "(init): MEI: read dev='" << devID << "'" << endl;
					MEIObjIDMap meiomap;
					// Object ID list..
					for( ;oit.getCurrent(); oit.goNext() )
					{
						if( dit.getProp("id").empty() )
						{
							dlog[Debug::WARN] << myname 
								<< "(init): read <MEI>. Unknown <object id='' (for device id='"
								<< devID << "'). Ignore.." 
								<< endl;
							
							continue;
						}

						int objID = oit.getIntProp("id");
						UniXML_iterator sit(oit);
						if( sit.goChildren() )
						{
							if( dlog.debugging(Debug::INFO) )
								dlog[Debug::INFO] << myname << "(init): MEI: read obj='" << objID << "'" << endl;

							MEIValMap meivmap;
							// request (string) list..
							for( ;sit.getCurrent(); sit.goNext() )
							{
								int vid = objID;
								if( sit.getProp("id").empty() )
								{
									if( dlog.debugging(Debug::WARN) )
										dlog[Debug::INFO] << myname << "(init): MEI: dev='" << devID 
											<< "' obj='" << objID << "'"
											<< ". Unknown id='' for value='" << sit.getProp("value") << "'" 
											<< ". Set objID='" << objID << "'"
											<< endl;
								}
								else 
									vid = sit.getIntProp("id");
								
								meivmap[vid] = sit.getProp("value");
							}

							if( !meivmap.empty() )
								meiomap[objID] = meivmap;
						}
					}

					if( !meiomap.empty() )
						meidev[devID] = meiomap;
				}
			}
		}

		if( !meidev.empty() && dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(init): <MEI> init ok." << endl;
	}
	else
		dlog[Debug::INFO] << myname << "(init): <MEI> empty..." << endl;

}
// -----------------------------------------------------------------------------
MBSlave::~MBSlave()
{
	delete mbslot;
	delete shm;
	delete thr;
}
// -----------------------------------------------------------------------------
void MBSlave::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--" + prefix + "-sm-ready-timeout","15000");
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
void MBSlave::execute_rtu()
{
	ModbusRTUSlaveSlot* rscomm = dynamic_cast<ModbusRTUSlaveSlot*>(mbslot);
	
	ModbusRTU::mbErrCode prev = erNoError;
	while(1)
	{
		try
		{
			ModbusRTU::mbErrCode res = rscomm->receive( addr, wait_msec );

			if( res!=ModbusRTU::erTimeOut )
				ptTimeout.reset();
	
			// собираем статистику обмена
			if( prev!=ModbusRTU::erTimeOut )
			{
				//  с проверкой на переполнение
				askCount = askCount>=numeric_limits<long>::max() ? 0 : askCount+1;
				if( res!=ModbusRTU::erNoError )
					errmap[res]++;
			}

			prev = res;

			if( res!=ModbusRTU::erNoError && res!=ModbusRTU::erTimeOut )
				dlog[Debug::WARN] << myname << "(execute_rtu): " << ModbusRTU::mbErr2Str(res) << endl;

			if( !activated )
				continue;

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
						<< "(execute_rtu): (hb) " << ex << std::endl;
				}
			}

			if( respond_id != DefaultObjectId )
			{
				bool state = ptTimeout.checkTime() ? false : true;
				if( respond_invert )
					state ^= true;

				try
				{
					shm->localSaveState(ditRespond,respond_id,state,getId());
				}
				catch(Exception& ex)
				{
					dlog[Debug::CRIT] << myname
						<< "(execute_rtu): (respond) " << ex << std::endl;
				}
			}

			if( askcount_id!=DefaultObjectId )
			{
				try
				{
					shm->localSaveValue(aitAskCount,askcount_id,askCount,getId());
				}
				catch(Exception& ex)
				{
					dlog[Debug::CRIT] << myname
						<< "(execute_rtu): (askCount) " << ex << std::endl;
				}
			}
		
			for( IOMap::iterator it=iomap.begin(); it!=iomap.end(); ++it )
				IOBase::processingThreshold(&it->second,shm,force);
		}
		catch(...){}
	}
}
// -------------------------------------------------------------------------
void MBSlave::execute_tcp()
{
	ModbusTCPServerSlot* sslot = dynamic_cast<ModbusTCPServerSlot*>(mbslot);

    ModbusRTU::mbErrCode prev = erNoError;

	while(1)
	{
		try
		{
			ModbusRTU::mbErrCode res = sslot->receive( addr, wait_msec );

			if( res!=ModbusRTU::erTimeOut )
				ptTimeout.reset();
	
			// собираем статистику обмена
			if( prev!=ModbusRTU::erTimeOut )
			{
				//  с проверкой на переполнение
				askCount = askCount>=numeric_limits<long>::max() ? 0 : askCount+1;
				if( res!=ModbusRTU::erNoError )
					errmap[res]++;
			}

			prev = res;
			
			if( res!=ModbusRTU::erNoError && res!=ModbusRTU::erTimeOut )
				dlog[Debug::WARN] << myname << "(execute_tcp): " << ModbusRTU::mbErr2Str(res) << endl;

			if( !activated )
				continue;

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
						<< "(execute_tcp): (hb) " << ex << std::endl;
				}
			}

			if( respond_id != DefaultObjectId )
			{
				bool state = ptTimeout.checkTime() ? false : true;
				if( respond_invert )
					state ^= true;
				try
				{
					shm->localSaveState(ditRespond,respond_id,state,getId());
				}
				catch(Exception& ex)
				{
					dlog[Debug::CRIT] << myname
						<< "(execute_rtu): (respond) " << ex << std::endl;
				}
			}

			if( askcount_id!=DefaultObjectId )
			{
				try
				{
					shm->localSaveValue(aitAskCount,askcount_id,askCount,getId());
				}
				catch(Exception& ex)
				{
					dlog[Debug::CRIT] << myname
						<< "(execute_rtu): (askCount) " << ex << std::endl;
				}
			}

			for( IOMap::iterator it=iomap.begin(); it!=iomap.end(); ++it )
				IOBase::processingThreshold(&it->second,shm,force);
		}
		catch(...){}
	}
}
// -------------------------------------------------------------------------

void MBSlave::processingMessage(UniSetTypes::VoidMessage *msg)
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
		raise(SIGTERM);
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
void MBSlave::sysCommand(UniSetTypes::SystemMessage *sm)
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( iomap.empty() )
			{
				dlog[Debug::CRIT] << myname << "(sysCommand): iomap EMPTY! terminated..." << endl;
				raise(SIGTERM);
				return; 
			}
		
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
			else 
			{
				UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
				askSensors(UniversalIO::UIONotify);
				thr->start();
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
			// (т.е. MBSlave  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(MBSlave)
			if( shm->isLocalwork() )
				break;

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
void MBSlave::askSensors( UniversalIO::UIOCommand cmd )
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

	if( force )
		return;

	IOMap::iterator it=iomap.begin();
	for( ; it!=iomap.end(); ++it )
	{
		IOProperty* p(&it->second);
		
//		if( p->stype != UniversalIO::DigitalOutput && p->stype != UniversalIO::AnalogOutput )
//			continue;

//		if( p->safety == NoSafetyState )
//			continue;
		try
		{
			shm->askSensor(p->si.id,cmd);
		}
		catch( UniSetTypes::Exception& ex )
		{
			dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
		}
		catch(...){}
	}
}
// ------------------------------------------------------------------------------------------
void MBSlave::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	IOMap::iterator it=iomap.begin();
	for( ; it!=iomap.end(); ++it )
	{
		if( it->second.si.id == sm->id )
		{
			IOProperty* p(&it->second);
			if( p->stype == UniversalIO::DigitalOutput ||
				p->stype == UniversalIO::DigitalInput )
			{
				uniset_spin_lock lock(p->val_lock);
				p->value = sm->state ? 1 : 0;
			}
			else if( p->stype == UniversalIO::AnalogOutput ||
					p->stype == UniversalIO::AnalogInput )
			{
				uniset_spin_lock lock(p->val_lock);
				p->value = sm->value;
			}

			int sz = VTypes::wsize(p->vtype);
			if( sz < 1 )
				return;

			// если размер больше одного слова
			// то надо обновить значение "везде"
			// они если "всё верно инициализировано" идут подряд
			int i=0;
			for( ;i<sz && it!=iomap.end(); i++,it++ )
			{
				p  = &it->second;
				if( p->si.id == sm->id )
					p->value = sm->value; 
			}

			if( dlog.debugging(Debug::CRIT) )
			{
				// вообще этого не может случиться
				// потому-что корректность проверяется при загрузке
				if( i != sz )
					dlog[Debug::CRIT] << myname << "(sensorInfo): update failed for sid=" << sm->id
						<< " (i=" << i << " sz=" << sz << ")" << endl;
			}
			return;
		}
	}
}
// ------------------------------------------------------------------------------------------
bool MBSlave::activateObject()
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
void MBSlave::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
	try
	{
		if( mbslot )
			mbslot->sigterm(signo);
	}
	catch(...){}
	
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void MBSlave::readConfiguration()
{
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
		if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
			initItem(it);
	}
	
//	readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool MBSlave::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------
bool MBSlave::initItem( UniXML_iterator& it )
{
	IOProperty p;

	if( !IOBase::initItem( static_cast<IOBase*>(&p),it,shm,&dlog,myname) )
		return false;

	if( mbregFromID )
		p.mbreg = p.si.id;
	else
	{
		string r = it.getProp("mbreg");
		if( r.empty() )
		{
			dlog[Debug::CRIT] << myname << "(initItem): Unknown 'mbreg' for " << it.getProp("name") << endl;
			return false;
		}
		
		p.mbreg = ModbusRTU::str2mbData(r);
	}
	
	string stype( it.getProp("mb_iotype") );
	if( stype.empty() )
		stype = it.getProp("iotype");

	p.stype = UniSetTypes::getIOType(stype);
	if( p.stype == UniversalIO::UnknownIOType )
	{
		dlog[Debug::CRIT] << myname << "(initItem): Unknown 'iotype' or 'mb_iotype' for " << it.getProp("name") << endl;
		return false;
	}

	p.amode = MBSlave::amRW;
	string am(it.getProp("mb_accessmode"));
	if( am == "ro" )
		p.amode = MBSlave::amRO;
	else if( am == "rw" )
		p.amode = MBSlave::amRW;

	string vt(it.getProp("mb_vtype"));
	if( vt.empty() )
	{
		p.vtype = VTypes::vtUnknown;
		p.wnum = 0;
		iomap[p.mbreg] = p;
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;

	}
	else
	{
		VTypes::VType v(VTypes::str2type(vt));
		if( v == VTypes::vtUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initItem): Unknown rtuVType=" << vt << " for " 
					<< it.getProp("name") 
					<< endl;

			return false;
		}
		p.vtype = v;
		p.wnum = 0;
		for( int i=0; i<VTypes::wsize(p.vtype); i++ )
		{
			p.mbreg += i;
			p.wnum+= i;
			iomap[p.mbreg] = p;
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;
		}
	}
	
	return true;
}
// ------------------------------------------------------------------------------------------
void MBSlave::initIterators()
{
	IOMap::iterator it=iomap.begin();
	for( ; it!=iomap.end(); it++ )
	{
		shm->initDIterator(it->second.dit);
		shm->initAIterator(it->second.ait);
	}

	shm->initAIterator(aitHeartBeat);
	shm->initAIterator(aitAskCount);
	shm->initDIterator(ditRespond);
}
// -----------------------------------------------------------------------------
void MBSlave::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbs'" << endl;
	cout << "--prefix-reg-from-id 0,1   - Использовать в качестве регистра sensor ID" << endl;
	cout << "--prefix-filter-field name - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
	cout << "--prefix-filter-value val  - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
	cout << "--prefix-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--prefix-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--prefix-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
	cout << "--prefix-initPause         - Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--prefix-force 1           - Читать данные из SM каждый раз, а не по изменению." << endl;
	cout << "--prefix-respond-id - respond sensor id" << endl;
	cout << "--prefix-respond-invert [0|1] - invert respond logic" << endl;
	cout << "--prefix-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << "--prefix-timeout msec - timeout for check link" << endl;
	cout << "--prefix-allow-setdatetime - On set date and time (0x50) modbus function" << endl;
	cout << "--prefix-my-addr      - адрес текущего узла" << endl;
	cout << "--prefix-type [RTU|TCP] - modbus server type." << endl;

	cout << " Настройки протокола RTU: " << endl;
	cout << "--prefix-dev devname  - файл устройства" << endl;
	cout << "--prefix-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;

	cout << " Настройки протокола TCP: " << endl;
	cout << "--prefix-inet-addr [xxx.xxx.xxx.xxx | hostname ]  - this modbus server address" << endl;
	cout << "--prefix-inet-port num - this modbus server port. Default: 502" << endl;
}
// -----------------------------------------------------------------------------
MBSlave* MBSlave::init_mbslave( int argc, const char* const* argv, UniSetTypes::ObjectId icID, SharedMemory* ic,
								string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","MBSlave1");
	if( name.empty() )
	{
		cerr << "(mbslave): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(mbslave): идентификатор '" << name 
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(mbslave): name = " << name << "(" << ID << ")" << endl;
	return new MBSlave(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBSlave::IOProperty& p )
{
	os 	<< " reg=" << ModbusRTU::dat2str(p.mbreg)
		<< " sid=" << p.si.id
		<< " stype=" << p.stype
		<< " wnum=" << p.wnum
		<< " safety=" << p.safety
		<< " invert=" << p.invert;

	if( p.stype == UniversalIO::AnalogInput || p.stype == UniversalIO::AnalogOutput )
	{
		os 	<< p.cal
			<< " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
	}		
	
	return os;
}
// -----------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readOutputRegisters( ModbusRTU::ReadOutputMessage& query, 
													ModbusRTU::ReadOutputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(readOutputRegisters): " << query << endl;

	if( query.count <= 1 )
	{
		ModbusRTU::ModbusData d = 0;
		ModbusRTU::mbErrCode ret = real_read(query.start,d);
		if( ret == ModbusRTU::erNoError )
			reply.addData(d);
		else
			reply.addData(0);
		return ret;
	}

	// Фомирование ответа:
	ModbusRTU::mbErrCode ret = much_real_read(query.start,buf,query.count);
	if( ret == ModbusRTU::erNoError )
	{
		for( int i=0; i<query.count; i++ )
			reply.addData( buf[i] );
	}
	
	return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
											ModbusRTU::WriteOutputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(writeOutputRegisters): " << query << endl;

	// Формирование ответа:
	ModbusRTU::mbErrCode ret = much_real_write(query.start,query.data,query.quant);
	if( ret == ModbusRTU::erNoError )
        	reply.set(query.start,query.quant); 
	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
											ModbusRTU::WriteSingleOutputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(writeOutputSingleRegisters): " << query << endl;

	ModbusRTU::mbErrCode ret = real_write(query.start,query.data);
	if( ret == ModbusRTU::erNoError )
		reply.set(query.start,query.data);

	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::much_real_write( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, 
						int count )
{	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(much_real_write): read mbID=" 
			<< ModbusRTU::dat2str(reg) << " count=" << count << endl;
	}
	
	int i=0;
	IOMap::iterator it = iomap.end();
	for( ; i<count; i++ )
	{
		it = iomap.find(reg+i);
		if( it != iomap.end() )
		{
			reg += i;
			break;
		}
	}

	if( it == iomap.end() )
		return ModbusRTU::erBadDataAddress;

	for( ; (it!=iomap.end()) && (i<count); i++,reg++ )
	{
		if( it->first == reg )
		{
			real_write_it(it,dat[i]);
			it++;
		}
	}
	
	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_write( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData mbval )
{
	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(write): save mbID=" 
			<< ModbusRTU::dat2str(reg) 
			<< " data=" << ModbusRTU::dat2str(mbval)
			<< "(" << (int)mbval << ")" << endl;
	}

	IOMap::iterator it = iomap.find(reg);
	return real_write_it(it,mbval);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_write_it( IOMap::iterator& it, ModbusRTU::ModbusData& mbval )
{
	if( it == iomap.end() )
		return ModbusRTU::erBadDataAddress;

	try
	{
		IOProperty* p(&it->second);

		if( p->amode == MBSlave::amRO )
			return ModbusRTU::erBadDataAddress;

		if( p->vtype == VTypes::vtUnknown )
		{
			if( p->stype == UniversalIO::DigitalInput ||
				p->stype == UniversalIO::DigitalOutput )
			{
				IOBase::processingAsDI( p, mbval, shm, force );
			}
			else
			{
				long val = (signed short)(mbval);
				IOBase::processingAsAI( p, val, shm, force );
			}
			return erNoError;
		}
		else if( p->vtype == VTypes::vtUnsigned )
		{
			long val = (unsigned short)(mbval);
			IOBase::processingAsAI( p, val, shm, force );
		}
		else if( p->vtype == VTypes::vtSigned )
		{
			long val = (signed short)(mbval);
			IOBase::processingAsAI( p, val, shm, force );
		}
/*		
		else if( p->vtype == VTypes::vtByte )
		{
			VTypes::Byte b(r->mbval);
			IOBase::processingAsAI( p, b.raw.b[p->nbyte-1], shm, force );
			return;
		}
		else if( p->vtype == VTypes::vtF2 )
		{
			RegMap::iterator i(p->reg->rit);
			ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::F2::wsize()];
				for( int k=0; k<VTypes::F2::wsize(); k++, i++ )
					data[k] = i->second->mbval;
				
				VTypes::F2 f(data,VTypes::F2::wsize());
				delete[] data;
			
				IOBase::processingFasAI( p, (float)f, shm, force );
			}
			else if( p->vtype == VTypes::vtF4 )
			{
				RegMap::iterator i(p->reg->rit);

				ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::F4::wsize()];
				for( int k=0; k<VTypes::F4::wsize(); k++, i++ )
					data[k] = i->second->mbval;
				
				VTypes::F4 f(data,VTypes::F4::wsize());
				delete[] data;
				
				IOBase::processingFasAI( p, (float)f, shm, force );
			}
*/

/*
		if( p->stype == UniversalIO::DigitalInput ||
			p->stype == UniversalIO::DigitalOutput )
		{
			bool set = val ? true : false;
			IOBase::processingAsDI(p,set,shm,force);
		}
		else if( p->stype == UniversalIO::AnalogInput ||
				 p->stype == UniversalIO::AnalogOutput )
		{
			IOBase::processingAsAI( p, val, shm, force );
		}
*/
		pingOK = true;
		return ModbusRTU::erNoError;
	}
	catch( UniSetTypes::NameNotFound& ex )
	{
		dlog[Debug::WARN] << myname << "(write): " << ex << endl;
		return ModbusRTU::erBadDataAddress;
	}
	catch( UniSetTypes::OutOfRange& ex )
	{
		dlog[Debug::WARN] << myname << "(write): " << ex << endl;
		return ModbusRTU::erBadDataValue;
	}
	catch( Exception& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(write): " << ex << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(write): СORBA::SystemException: "
				<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(write) catch ..." << endl;
	}
	
	pingOK = false;
	return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::much_real_read( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, 
						int count )
{
	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(much_real_read): read mbID=" 
			<< ModbusRTU::dat2str(reg) << " count=" << count << endl;
	}
	
	IOMap::iterator it = iomap.end();
	int i=0;
	for( ; i<count; i++ )
	{
		it = iomap.find(reg+i);
		if( it != iomap.end() )
		{
			reg += i;
			break;
		}

		dat[i] = 0;
	}

	if( it == iomap.end() )
		return ModbusRTU::erBadDataAddress;
	
	ModbusRTU::ModbusData val=0;
	for( ; (it!=iomap.end()) && (i<count); i++,reg++ )
	{
		val=0;
		// если регистры идут не подряд, то просто вернём ноль
		if( it->first == reg )
		{
			real_read_it(it,val);
			it++;
		}
		dat[i] = val;
	}

	// добиваем нулями "ответ"
	// чтобы он был такой длинны, которую запрашивали
	if( i<count )
	{
		for( ; i<count; i++ )
			dat[i] = 0;
	}	

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_read( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData& val )
{
	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(real_read): read mbID=" 
			<< ModbusRTU::dat2str(reg) << endl;
	}

	IOMap::iterator it = iomap.find(reg);
	return real_read_it(it,val);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::real_read_it( IOMap::iterator& it, ModbusRTU::ModbusData& val )
{
	if( it == iomap.end() )
		return ModbusRTU::erBadDataAddress;

	try
	{
		if( dlog.debugging(Debug::INFO) )
		{
			dlog[Debug::INFO] << myname << "(real_read_it): read mbID="
				<< ModbusRTU::dat2str(it->first) << endl;
		}
				
		IOProperty* p(&it->second);
		val = 0;

		if( p->amode == MBSlave::amWO )
			return ModbusRTU::erBadDataAddress;
		
		if( p->stype == UniversalIO::DigitalInput || 
			p->stype == UniversalIO::DigitalOutput )
		{
			val = IOBase::processingAsDO(p,shm,force) ? 1 : 0;
		}
		else if( p->stype == UniversalIO::AnalogInput ||
				p->stype == UniversalIO::AnalogOutput )
		{
			if( p->vtype == VTypes::vtUnknown )
			{
				val = IOBase::processingAsAO(p,shm,force);
			}		
			else if( p->vtype == VTypes::vtF2 )
			{
				float f = IOBase::processingFasAO(p,shm,force);
				VTypes::F2 f2(f);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < f4.wsize()
				val = f2.raw.v[p->wnum];
			}
			else if( p->vtype == VTypes::vtF2r )
			{
				float f = IOBase::processingFasAO(p,shm,force);
				VTypes::F2r f2(f);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < f4.wsize()
				val = f2.raw.v[p->wnum];
			}
			else if( p->vtype == VTypes::vtF4 )
			{
				float f = IOBase::processingFasAO(p,shm,force);
				VTypes::F4 f4(f);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < f4.wsize()
				val = f4.raw.v[p->wnum];
			}
			else if( p->vtype == VTypes::vtI2 )
			{
				long v = IOBase::processingAsAO(p,shm,force);
				VTypes::I2 i2(v);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < i2.wsize()
				val = i2.raw.v[p->wnum];
			}
			else if( p->vtype == VTypes::vtI2r )
			{
				long v = IOBase::processingAsAO(p,shm,force);
				VTypes::I2r i2(v);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < i2.wsize()
				val = i2.raw.v[p->wnum];
			}
			else if( p->vtype == VTypes::vtU2 )
			{
				unsigned long v = IOBase::processingAsAO(p,shm,force);
				VTypes::U2 u2(v);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < u2.wsize()
				val = u2.raw.v[p->wnum];
			}
			else if( p->vtype == VTypes::vtU2r )
			{
				unsigned long v = IOBase::processingAsAO(p,shm,force);
				VTypes::U2r u2(v);
				// оптимизируем и проверку не делаем
				// считая, что при "загрузке" всё было правильно
				// инициализировано
				// if( p->wnum >=0 && p->wnum < u2.wsize()
				val = u2.raw.v[p->wnum];
			}
			else
				val = IOBase::processingAsAO(p,shm,force);
		}
		else
			return ModbusRTU::erBadDataAddress;

		pingOK = true;
		return ModbusRTU::erNoError;
	}
	catch( UniSetTypes::NameNotFound& ex )
	{
		dlog[Debug::WARN] << myname << "(real_read_it): " << ex << endl;
		return ModbusRTU::erBadDataAddress;
	}
	catch( UniSetTypes::OutOfRange& ex )
	{
		dlog[Debug::WARN] << myname << "(real_read_it): " << ex << endl;
		return ModbusRTU::erBadDataValue;
	}
	catch( Exception& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(real_read_it): " << ex << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(real_read_it): CORBA::SystemException: "
				<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(real_read_it) catch ..." << endl;
	}
	
	pingOK = false;
	return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------

mbErrCode MBSlave::readInputRegisters( ReadInputMessage& query, ReadInputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(readInputRegisters): " << query << endl;

	if( query.count <= 1 )
	{
		ModbusRTU::ModbusData d = 0;
		ModbusRTU::mbErrCode ret = real_read(query.start,d);
		if( ret == ModbusRTU::erNoError )
			reply.addData(d);
		else
			reply.addData(0);
		
		return ret;
	}

	// Фомирование ответа:
	ModbusRTU::mbErrCode ret = much_real_read(query.start,buf,query.count);
	if( ret == ModbusRTU::erNoError )
	{
		for( int i=0; i<query.count; i++ )
			reply.addData( buf[i] );
	}
	
	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::setDateTime( ModbusRTU::SetDateTimeMessage& query, 
									ModbusRTU::SetDateTimeRetMessage& reply )
{
	return ModbusServer::replySetDateTime(query,reply,&dlog);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::remoteService( ModbusRTU::RemoteServiceMessage& query, 
									ModbusRTU::RemoteServiceRetMessage& reply )
{
//	cerr << "(remoteService): " << query << endl;
	return ModbusRTU::erOperationFailed;
}									
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::fileTransfer( ModbusRTU::FileTransferMessage& query, 
									ModbusRTU::FileTransferRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(fileTransfer): " << query << endl;

	FileList::iterator it = flist.find(query.numfile);
	if( it == flist.end() )
		return ModbusRTU::erBadDataValue;

	std::string fname(it->second);
	return ModbusServer::replyFileTransfer( fname,query,reply,&dlog );
}									
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readCoilStatus( ReadCoilMessage& query, 
												ReadCoilRetMessage& reply )
{
//	cout << "(readInputStatus): " << query << endl;
	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readInputStatus( ReadInputStatusMessage& query, 
												ReadInputStatusRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(readInputStatus): " << query << endl;

	try
	{
		if( query.count <= 1 )
		{
			ModbusRTU::ModbusData d = 0;
			ModbusRTU::mbErrCode ret = real_read(query.start,d);
			reply.addData(0);
			if( ret == ModbusRTU::erNoError )
				reply.setBit(0,0,d);
			else
				reply.setBit(0,0,0);
			
			pingOK = true;
			return ret;
		}

		// Фомирование ответа:
		much_real_read(query.start,buf,query.count);
		int bnum = 0;
		int i=0;
		while( i<query.count )
		{
			reply.addData(0);
			for( int nbit=0; nbit<BitsPerByte && i<query.count; nbit++,i++ )
				reply.setBit(bnum,nbit,buf[i]);
			bnum++;
		}

		pingOK = true;
		return ModbusRTU::erNoError;
	}
	catch( UniSetTypes::NameNotFound& ex )
	{
		dlog[Debug::WARN] << myname << "(readInputStatus): " << ex << endl;
		return ModbusRTU::erBadDataAddress;
	}
	catch( Exception& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readInputStatus): " << ex << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readInputStatus): СORBA::SystemException: "
				<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readInputStatus): catch ..." << endl;
	}
	
	pingOK = false;
	return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
													ModbusRTU::ForceCoilsRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(forceMultipleCoils): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	int nbit = 0;
	for( int i = 0; i<query.bcnt; i++ )
	{
		ModbusRTU::DataBits b(query.data[i]);
		for( int k=0; k<ModbusRTU::BitsPerByte && nbit<query.quant; k++, nbit++ )
		{
			// ModbusRTU::mbErrCode ret = 	
			real_write(query.start+nbit, (b[k] ? 1 : 0) );
			//if( ret == ModbusRTU::erNoError )
		}
	}
	
	//if( ret == ModbusRTU::erNoError )
	if( nbit == query.quant )
		reply.set(query.start,query.quant);
	
	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
											ModbusRTU::ForceSingleCoilRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(forceSingleCoil): " << query << endl;

	ModbusRTU::mbErrCode ret = real_write(query.start, (query.cmd() ? 1 : 0) );
	if( ret == ModbusRTU::erNoError )
		reply.set(query.start,query.data);

	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::diagnostics( ModbusRTU::DiagnosticMessage& query, 
											ModbusRTU::DiagnosticRetMessage& reply )
{
	if( query.subf == ModbusRTU::subEcho )
	{
		reply = query;
		return ModbusRTU::erNoError;
	}
	
	if( query.subf == ModbusRTU::dgBusErrCount )
	{
		reply = query;
		reply.data[0] = errmap[ModbusRTU::erBadCheckSum];
		return ModbusRTU::erNoError;
	}
	
	if( query.subf == ModbusRTU::dgMsgSlaveCount || query.subf == ModbusRTU::dgBusMsgCount )
	{
		reply = query;
		reply.data[0] = askCount;
		return ModbusRTU::erNoError;
	}

	if( query.subf == ModbusRTU::dgSlaveNAKCount )
	{
		reply = query;
		reply.data[0] = errmap[erOperationFailed];
		return ModbusRTU::erNoError;
	}

	if( query.subf == ModbusRTU::dgClearCounters )
	{
		askCount = 0;
		errmap[erOperationFailed] = 0;
		errmap[ModbusRTU::erBadCheckSum] = 0;
		// другие счётчики пока не сбрасываем..
		reply = query;
		return ModbusRTU::erNoError;
	}	

	return ModbusRTU::erOperationFailed; 
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::read4314( ModbusRTU::MEIMessageRDI& query, 
								ModbusRTU::MEIMessageRetRDI& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(read4314): " << query << endl;

//	if( query.devID <= rdevMinNum || query.devID >= rdevMaxNum )
//		return erOperationFailed;

	MEIDevIDMap::iterator dit = meidev.find(query.devID);
	if( dit == meidev.end() )
		return ModbusRTU::erBadDataAddress;

	MEIObjIDMap::iterator oit = dit->second.find(query.objID);
	if( oit == dit->second.end() )
		return ModbusRTU::erBadDataAddress;
	
	reply.mf = 0xFF;
	reply.conformity = query.devID;
	for( MEIValMap::iterator i=oit->second.begin(); i!=oit->second.end(); i++ )
		reply.addData( i->first, i->second );
	
	return erNoError;
}
// -------------------------------------------------------------------------


