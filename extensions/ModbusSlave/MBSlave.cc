// $Id: MBSlave.cc,v 1.1 2009/01/11 19:08:45 vpashka Exp $
// -----------------------------------------------------------------------------
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
askCount(0),
activated(false),
activateTimeout(500),
pingOK(true),
force(false),
mbregFromID(false),
prefix(prefix)
{
	cout << "$Id: MBSlave.cc,v 1.1 2009/01/11 19:08:45 vpashka Exp $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBSlave): objId=-1?!! Use --mbs-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBSlave): Not find conf-node for " + myname );

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

		ModbusRTUSlaveSlot* rs = new ModbusRTUSlaveSlot(dev);
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
			throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown TCP server address. Use: --mbs-inet-addr [ XXX.XXX.XXX.XXX| hostname ]");
		
		int port = conf->getArgPInt("--" + prefix + "-inet-port",it.getProp("iport"), 502);
	
		ost::InetAddress ia(iaddr.c_str());
		mbslot	= new ModbusTCPServerSlot(ia,port);
		thr = new ThreadCreator<MBSlave>(this,&MBSlave::execute_tcp);

		dlog[Debug::INFO] << myname << "(init): type=TCP myaddr=" << ModbusRTU::addr2str(addr) 
			<< " inet=" << iaddr << " port=" << port << endl;
	}
	else
		throw UniSetTypes::SystemError(myname+"(MBSlave): Unknown slave type. Use: --mbs-type [RTU|TCP]");

//	mbslot->connectReadCoil( sigc::mem_fun(this, &MBSlave::readCoilStatus) );
//	mbslot->connectReadInputStatus( sigc::mem_fun(this, &MBSlave::readInputStatus) );
	mbslot->connectReadOutput( sigc::mem_fun(this, &MBSlave::readOutputRegisters) );
	mbslot->connectReadInput( sigc::mem_fun(this, &MBSlave::readInputRegisters) );
//	mbslot->connectForceSingleCoil( sigc::mem_fun(this, &MBSlave::forceSingleCoil) );
//	mbslot->connectForceCoils( sigc::mem_fun(this, &MBSlave::forceMultipleCoils) );
	mbslot->connectWriteOutput( sigc::mem_fun(this, &MBSlave::writeOutputRegisters) );
	mbslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBSlave::writeOutputSingleRegister) );

	if( findArgParam("--mbs-allow-setdatetime",conf->getArgc(),conf->getArgv())!=-1 )
		mbslot->connectSetDateTime( sigc::mem_fun(this, &MBSlave::setDateTime) );
	
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
		ic->addReadItem( sigc::mem_fun(this,&MBSlave::readItem) );

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
	xmlNode* fnode = conf->findNode(cnode,"filelist");
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
	
				prev = res;
			}
			
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
	
				prev = res;
			}
			
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
			break;
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
		if( check_item(it) )
			initItem(it);
	}
	
//	readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool MBSlave::check_item( UniXML_iterator& it )
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

bool MBSlave::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
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

	if( stype == "AI" )
		p.stype 	= UniversalIO::AnalogInput;
	else if ( stype == "DI" )
		p.stype 	= UniversalIO::DigitalInput;
	else if ( stype == "AO" )
		p.stype 	= UniversalIO::AnalogOutput;
	else if ( stype == "DO" )
		p.stype 	= UniversalIO::DigitalOutput;

	p.amode = MBSlave::amRW;
	string am(it.getProp("mb_accessmode"));
	if( am == "ro" )
		p.amode = MBSlave::amRO;
	else if( am == "rw" )
		p.amode = MBSlave::amRW;

	iomap[p.mbreg] = p;
	
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;
	
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
}
// -----------------------------------------------------------------------------
void MBSlave::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbtcp'" << endl;
	cout << "--prefix-heartbeat-id		- Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--prefix-heartbeat-max  	- Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--prefix-ready-timeout	- Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;    
	cout << "--prefix-initPause		- Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--prefix-notRespondSensor - датчик связи для данного процесса " << endl;
	cout << "--prefix-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << "--prefix-recv-timeout - Таймаут на ожидание ответа." << endl;
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
	return new MBSlave(ID,icID,ic);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBSlave::IOProperty& p )
{
	os 	<< " reg=" << ModbusRTU::dat2str(p.mbreg)
		<< " sid=" << p.si.id
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
ModbusRTU::mbErrCode MBSlave::readOutputRegisters( ModbusRTU::ReadOutputMessage& query, 
													ModbusRTU::ReadOutputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(readOutputRegisters): " << query << endl;

	try
	{
		if( query.count <= 1 )
		{
			ModbusRTU::ModbusData d = 0;
			ModbusRTU::mbErrCode ret = real_read(query.start,d);
			if( ret == ModbusRTU::erNoError )
				reply.addData(d);
			else
				reply.addData(0);

			pingOK = true;
			return ret;
		}

		// Фомирование ответа:
		int num=0; // добавленное количество данных
		ModbusRTU::ModbusData d = 0;
		ModbusRTU::ModbusData reg = query.start;
		for( ; num<query.count; num++, reg++ )
		{
			ModbusRTU::mbErrCode ret = real_read(reg,d);
			if( ret == ModbusRTU::erNoError )
				reply.addData(d);
			else
				reply.addData(0);
		}

		// Если мы в начале проверили, что запрос входит в разрешёный диапазон
		// то теоретически этой ситуации возникнуть не может...
		if( reply.count < query.count )
		{
			dlog[Debug::WARN] << myname 
				<< "(readInputRegisters): query.count=" << query.count 
					<< " > reply.count=" << reply.count << endl;
		}

		pingOK = true;
		return ModbusRTU::erNoError;
	}
	catch( UniSetTypes::NameNotFound& ex )
	{
		dlog[Debug::WARN] << myname << "(readOutputRegisters): " << ex << endl;
		return ModbusRTU::erBadDataAddress;
	}
	catch( Exception& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readOutputRegisters): " << ex << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readOutputRegisters): СORBA::SystemException: "
				<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readOutputRegisters): catch ..." << endl;
	}
	
	pingOK = false;
	return ModbusRTU::erTimeOut;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
											ModbusRTU::WriteOutputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(writeOutputRegisters): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;

	// Формирование ответа:
	int write_ok = 0;
	ModbusRTU::ModbusData reg = query.start;
	for( int num=0; num<query.quant; num++,reg++ )
	{
		ret = real_write( reg, query.data[num]);
		if( ret == ModbusRTU::erNoError )
			write_ok++;
	}			

	// формируем ответ
	if( write_ok > 0 )
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
ModbusRTU::mbErrCode MBSlave::real_write( ModbusRTU::ModbusData reg, 
											ModbusRTU::ModbusData val )
{
	try
	{
		if( dlog.debugging(Debug::INFO) )
		{
			dlog[Debug::INFO] << myname << "(write): save mbID=" 
				<< ModbusRTU::dat2str(reg) 
				<< " data=" << ModbusRTU::dat2str(val)
				<< "(" << (int)val << ")" << endl;
		}

		IOMap::iterator it = iomap.find(reg);
		if( it == iomap.end() )
			return ModbusRTU::erBadDataAddress;
				
		IOProperty* p(&it->second);

		if( p->amode == MBSlave::amRO )
			return ModbusRTU::erBadDataAddress;

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
ModbusRTU::mbErrCode MBSlave::real_read( ModbusRTU::ModbusData reg, 
											ModbusRTU::ModbusData& val )
{
	try
	{
		if( dlog.debugging(Debug::INFO) )
		{
			dlog[Debug::INFO] << myname << "(real_read): read mbID=" 
				<< ModbusRTU::dat2str(reg) << endl;
		}

		IOMap::iterator it = iomap.find(reg);
		if( it == iomap.end() )
			return ModbusRTU::erBadDataAddress;
				
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
			val = IOBase::processingAsAO(p,shm,force);
		}
		else
			return ModbusRTU::erBadDataAddress;

		pingOK = true;
		return ModbusRTU::erNoError;
	}
	catch( UniSetTypes::NameNotFound& ex )
	{
		dlog[Debug::WARN] << myname << "(real_read): " << ex << endl;
		return ModbusRTU::erBadDataAddress;
	}
	catch( UniSetTypes::OutOfRange& ex )
	{
		dlog[Debug::WARN] << myname << "(real_read): " << ex << endl;
		return ModbusRTU::erBadDataValue;
	}
	catch( Exception& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(real_read): " << ex << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(real_read): CORBA::SystemException: "
				<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(real_read) catch ..." << endl;
	}
	
	pingOK = false;
	return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------

mbErrCode MBSlave::readInputRegisters( ReadInputMessage& query, 
										ReadInputRetMessage& reply )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(readInputRegisters): " << query << endl;

	try
	{
		if( query.count <= 1 )
		{
			ModbusRTU::ModbusData d = 0;
			ModbusRTU::mbErrCode ret = real_read(query.start,d);
			if( ret == ModbusRTU::erNoError )
				reply.addData(d);
			else
				reply.addData(0);
			
			
			pingOK = true;
			return ret;
		}

		// Фомирование ответа:
		int num=0; // добавленное количество данных
		ModbusRTU::ModbusData d = 0;
		ModbusRTU::ModbusData reg = query.start;
		for( ; num<query.count; num++, reg++ )
		{
			ModbusRTU::mbErrCode ret = real_read(reg,d);
			if( ret == ModbusRTU::erNoError )
				reply.addData(d);
			else
				reply.addData(0);
		}

		// Если мы в начале проверили, что запрос входит в разрешёный диапазон
		// то теоретически этой ситуации возникнуть не может...
		if( reply.count < query.count )
		{
			dlog[Debug::WARN] << myname 
				<< "(readInputRegisters): query.count=" << query.count 
					<< " > reply.count=" << reply.count << endl;
		}

		pingOK = true;
		return ModbusRTU::erNoError;
	}
	catch( UniSetTypes::NameNotFound& ex )
	{
		dlog[Debug::WARN] << myname << "(readInputRegisters): " << ex << endl;
		return ModbusRTU::erBadDataAddress;
	}
	catch( Exception& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readInputRegisters): " << ex << endl;
	}
	catch( CORBA::SystemException& ex )
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readInputRegisters): СORBA::SystemException: "
				<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		if( pingOK )
			dlog[Debug::CRIT] << myname << "(readInputRegisters): catch ..." << endl;
	}
	
	pingOK = false;
	return ModbusRTU::erTimeOut;
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
//	cout << "(readInputStatus): " << query << endl;
	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
													ModbusRTU::ForceCoilsRetMessage& reply )
{
//	cout << "(forceMultipleCoils): " << query << endl;
	return ModbusRTU::erOperationFailed;
}													
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
											ModbusRTU::ForceSingleCoilRetMessage& reply )
{
//	cout << "(forceSingleCoil): " << query << endl;
	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
