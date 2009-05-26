// $Id: MBMaster.cc,v 1.11 2009/03/03 10:33:27 pv Exp $
// -----------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extentions.h"
#include "MBMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtentions;
using namespace ModbusRTU;
// -----------------------------------------------------------------------------
MBMaster::MBMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic,
						std::string prefix):
UniSetObject_LT(objId),
mbmap(100),
maxItem(0),
mb(0),
shm(0),
initPause(0),
mbregFromID(false),
force(false),
force_out(false),
activated(false),
prefix(prefix)
{
	cout << "$Id: MBMaster.cc,v 1.11 2009/03/03 10:33:27 pv Exp $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBMaster): objId=-1?!! Use --mbtcp-name" );

	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBMaster): Not find conf-node for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dlog[Debug::INFO] << myname << "(init): read s_field='" << s_field
						<< "' s_fvalue='" << s_fvalue << "'" << endl;

	// ---------- init MBTCP ----------
	string pname("--" + prefix + "-iaddr");
	iaddr	= conf->getArgParam(pname,it.getProp("iaddr"));
	if( iaddr.empty() )
		throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet addr...(Use: " + pname +")" );

	string tmp("--" + prefix + "-port");
	port = atoi(conf->getArgParam(tmp,it.getProp("port")).c_str());
	if( port<=0 )
		throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet port...(Use: " + tmp +")" );

	recv_timeout = atoi(conf->getArgParam("--" + prefix + "-recv-timeout",it.getProp("recv_timeout")).c_str());
	if( recv_timeout <= 0 )
		recv_timeout = 2000;

	string saddr = conf->getArgParam("--" + prefix + "-my-addr",it.getProp("addr"));
	myaddr = ModbusRTU::str2mbAddr(saddr);
	if( saddr.empty() )
		myaddr = 0x00;

	mbregFromID = atoi(conf->getArgParam("--" + prefix + "-reg-from-id",it.getProp("reg_from_id")).c_str());
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	polltime = atoi(conf->getArgParam("--" + prefix + "-polltime",it.getProp("polltime")).c_str());
	if( !polltime )
		polltime = 100;

	initPause = atoi(conf->getArgParam("--" + prefix + "-initPause",it.getProp("initPause")).c_str());
	if( !initPause )
		initPause = 3000;

	force = atoi(conf->getArgParam("--" + prefix + "-force",it.getProp("force")).c_str());

	if( shm->isLocalwork() )
	{
		readConfiguration();
		mbmap.resize(maxItem);
		dlog[Debug::INFO] << myname << "(init): mbmap size = " << mbmap.size() << endl;
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&MBMaster::readItem) );

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix +"-heartbeat-id",it.getProp("heartbeat_id"));
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

		maxHeartBeat = atoi(conf->getArgParam("--" + prefix + "-heartbeat-max",it.getProp("heartbeat_max")).c_str());
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
			err << myname << ": test_id unknown. 'TestMode_S' not found...";
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	activateTimeout	= atoi(conf->getArgParam("--" + prefix + "-activate-timeout").c_str());
	if( activateTimeout<=0 )
		activateTimeout = 20000;

	int msec = atoi(conf->getArgParam("--" + prefix + "-timeout",it.getProp("timeout")).c_str());
	if( msec <=0 )
		msec = 3000;

	ptTimeout.setTiming(msec);

	dlog[Debug::INFO] << myname << "(init): " << prefix << "-timeout=" << msec << " msec" << endl;
}
// -----------------------------------------------------------------------------
MBMaster::~MBMaster()
{
	if( mb )
		mb->disconnect();
	delete mb;
}
// -----------------------------------------------------------------------------
void MBMaster::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = atoi(conf->getArgParam("--" + prefix + "-sm-ready-timeout","15000").c_str());
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
void MBMaster::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmExchange )
		step();
}
// -----------------------------------------------------------------------------
void MBMaster::step()
{
	{
		uniset_mutex_lock l(pollMutex,2000);
		try
		{
			poll();
		}
		catch(Exception& ex )
		{
			cerr << myname << "(step): " << ex << std::endl;
		}
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
void MBMaster::init_mb()
{
	if( mb )
		return;
	
	try
	{
		ost::Thread::setException(ost::Thread::throwException);
		ost::InetAddress ia(iaddr.c_str());
		mb = new ModbusTCPMaster();
		mb->connect(ia,port);
		mb->setTimeout(recv_timeout);
		mb->setLog(dlog);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << myname << ": create 'ModbusMaster' failed: " << ex << endl;
		throw;
	}

	dlog[Debug::INFO]  << myname << "(init): myaddr=" << ModbusRTU::addr2str(myaddr) 
			<< " iaddr=" << iaddr << " port=" << port << endl;
}
// -----------------------------------------------------------------------------
void MBMaster::poll()
{
	init_mb();

	for( MBMap::iterator it=mbmap.begin(); it!=mbmap.end(); ++it )
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
				long val = readReg(it);
				IOBase::processingAsAI( ib, val, shm, force );
			}
			else if( it->stype == UniversalIO::DigitalInput )
			{
				bool set = readReg(it) ? true : false;
				IOBase::processingAsDI( ib, set, shm, force );
			}
			else if( it->stype == UniversalIO::AnalogOutput )
			{
				long val = IOBase::processingAsAO(ib,shm,force_out);
				writeReg(it,val);
			}
			else if( it->stype == UniversalIO::DigitalOutput )
			{
				long val = IOBase::processingAsDO(ib,shm,force_out) ? 1 : 0;
				writeReg(it,val);
			}
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
	}
}
// -----------------------------------------------------------------------------
long MBMaster::readReg( MBMap::iterator& p )
{
	try
	{
		if( p->mbfunc == ModbusRTU::fnReadInputRegisters )
		{	
//			if( dlog.debugging(Debug::LEVEL3) )
//				dlog[Debug::LEVEL3] << " read from " << ModbusRTU::addr2str(p->mbaddr) << " reg=" << ModbusRTU::dat2str(p->mbreg) << endl;
			cerr << " read from " << ModbusRTU::addr2str(p->mbaddr) << " reg=" << ModbusRTU::dat2str(p->mbreg) << endl;
			ModbusRTU::ReadInputRetMessage ret = mb->read04(p->mbaddr,p->mbreg,1);
			return ret.data[0];
		}
	
		if( p->mbfunc == ModbusRTU::fnReadOutputRegisters )
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(p->mbaddr,p->mbreg,1);
			return ret.data[0];
		}

		cerr << myname << "(readReg): неподдерживаемая функция чтения " << (int)p->mbfunc << endl;
	}
	catch( ModbusRTU::mbException& ex )
	{
		dlog[Debug::CRIT] << "(readReg): " << ex << endl;
	}
	catch(SystemError& err)
	{
		dlog[Debug::CRIT] << "(readReg): " << err << endl;
	}
	catch(Exception& ex)
	{
		dlog[Debug::CRIT] << "(readReg): " << ex << endl;
	}
	catch( ost::SockException& e ) 
	{
		dlog[Debug::CRIT] << "(readReg): " << e.getString() << ": " << e.getSystemErrorString() << endl;
	}

	return 0;
}
// -----------------------------------------------------------------------------
bool MBMaster::writeReg( MBMap::iterator& p, long val )
{
	if( p->mbfunc == fnWriteOutputRegisters )
	{
		ModbusRTU::WriteOutputMessage msg(p->mbaddr,p->mbreg);
		msg.addData(val);
		ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		return true;
	}

	if( p->mbfunc == ModbusRTU::fnForceSingleCoil )
	{
		ModbusRTU::ForceSingleCoilRetMessage ret = mb->write05(p->mbaddr,p->mbreg,(bool)val);
		return false;
	}
	
	if( p->mbfunc == fnWriteOutputSingleRegister )
	{
		ModbusRTU::WriteSingleOutputRetMessage  ret = mb->write06(p->mbaddr,p->mbreg,val);
		return true;
	}
	
	if( p->mbfunc == fnForceMultipleCoils )
	{
		ModbusRTU::ForceCoilsMessage msg(p->mbaddr,p->mbreg);
		ModbusRTU::DataBits16 b(val);
		msg.addData(b);
		ModbusRTU::ForceCoilsRetMessage  ret = mb->write0F(msg);
		return true;
	}

	cerr << myname << "(writeReg): неподдерживаемая функция чтения " << (int)p->mbfunc << endl;
	return false;
}
// -----------------------------------------------------------------------------
void MBMaster::processingMessage(UniSetTypes::VoidMessage *msg)
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
void MBMaster::sysCommand(UniSetTypes::SystemMessage *sm)
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( mbmap.empty() )
			{
				dlog[Debug::CRIT] << myname << "(sysCommand): mbmap EMPTY! terminated..." << endl;
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
			// (т.е. MBMaster  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(MBMaster)
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
void MBMaster::initOutput()
{
}
// ------------------------------------------------------------------------------------------
void MBMaster::askSensors( UniversalIO::UIOCommand cmd )
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

	MBMap::iterator it=mbmap.begin();
	for( ; it!=mbmap.end(); ++it )
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
void MBMaster::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	MBMap::iterator it=mbmap.begin();
	for( ; it!=mbmap.end(); ++it )
	{
		if( it->stype != UniversalIO::DigitalOutput && it->stype!=UniversalIO::AnalogOutput )
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
bool MBMaster::activateObject()
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
void MBMaster::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
#warning Доделать...
	// выставление безопасного состояния на выходы....
	MBMap::iterator it=mbmap.begin();
	for( ; it!=mbmap.end(); ++it )
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
void MBMaster::readConfiguration()
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
bool MBMaster::check_item( UniXML_iterator& it )
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

bool MBMaster::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------
bool MBMaster::initItem( UniXML_iterator& it )
{
	MBProperty p;

	if( !IOBase::initItem( static_cast<IOBase*>(&p),it,shm,&dlog,myname) )
		return false;

	string addr = it.getProp("mbaddr");
	if( addr.empty() )
		return true;

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

	p.mbaddr 	= ModbusRTU::str2mbAddr(addr);

	string stype( it.getProp("mb_iotype") );
	if( stype.empty() )
		stype = it.getProp("iotype");

	if( stype == "AI" )
	{
		p.stype 	= UniversalIO::AnalogInput;
		p.mbfunc  	= ModbusRTU::fnReadInputRegisters;
	}
	else if ( stype == "DI" )
	{
		p.stype 	= UniversalIO::DigitalInput;
		p.mbfunc  	= ModbusRTU::fnReadInputRegisters;
	}
	else if ( stype == "AO" )
	{
		p.stype 	= UniversalIO::AnalogOutput;
		p.mbfunc  	= ModbusRTU::fnWriteOutputRegisters;
	}
	else if ( stype == "DO" )
	{
		p.stype 	= UniversalIO::DigitalOutput;
		p.mbfunc  	= ModbusRTU::fnWriteOutputRegisters;
	}

	string f = it.getProp("mbfunc");
	if( !f.empty() )
	{
		p.mbfunc = (ModbusRTU::SlaveFunctionCode)UniSetTypes::uni_atoi(f.c_str());
		if( p.mbfunc == ModbusRTU::fnUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initItem): Неверный mbfunc ='" << f
					<< "' для  датчика " << it.getProp("name") << endl;

			return false;
		}
	}

	if( p.mbfunc == ModbusRTU::fnReadCoilStatus ||
		p.mbfunc == ModbusRTU::fnReadInputStatus )
	{
		string nb = it.getProp("nbit");
		if( nb.empty() )
		{
			dlog[Debug::CRIT] << myname << "(initItem): Unknown nbit. for " 
					<< it.getProp("name") 
					<< " mbfunc=" << p.mbfunc
					<< endl;

			return false;
		}
		
		p.nbit = UniSetTypes::uni_atoi(nb.c_str());
	}

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;

	// если вектор уже заполнен
	// то увеличиваем его на 10 элементов (с запасом)
	// после инициализации делается resize
	// под текущее количество
	if( maxItem >= mbmap.size() )
		mbmap.resize(maxItem+10);
	
	mbmap[maxItem] = p;
	maxItem++;
	return true;
}
// ------------------------------------------------------------------------------------------
void MBMaster::initIterators()
{
	MBMap::iterator it=mbmap.begin();
	for( ; it!=mbmap.end(); it++ )
	{
		shm->initDIterator(it->dit);
		shm->initAIterator(it->ait);
	}

	shm->initAIterator(aitHeartBeat);
}
// -----------------------------------------------------------------------------
void MBMaster::help_print( int argc, char* argv[] )
{
	cout << "Default: prefix='mbtcp'" << endl;
	cout << "--prefix-polltime msec	- Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--prefix-heartbeat-id		- Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--prefix-heartbeat-max  	- Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--prefix-ready-timeout	- Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;    
	cout << "--prefix-force			- Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
	cout << "--prefix-initPause		- Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--prefix-notRespondSensor - датчик связи для данного процесса " << endl;
	cout << "--prefix-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола RS: " << endl;
	cout << "--prefix-iadrr ip  - IP" << endl;
	cout << "--prefix-port port    - Port." << endl;
	cout << "--prefix-my-addr      - адрес текущего узла" << endl;
	cout << "--prefix-recv-timeout - Таймаут на ожидание ответа." << endl;
}
// -----------------------------------------------------------------------------
MBMaster* MBMaster::init_mbmaster( int argc, char* argv[], UniSetTypes::ObjectId shmID, SharedMemory* ic,
							std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","MBTCPMaster1");
	if( name.empty() )
	{
		cerr << "(mbtcpexchange): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(mbtcpexchange): идентификатор '" << name 
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(mbtcpexchange): name = " << name << "(" << ID << ")" << endl;
	return new MBMaster(ID,shmID,ic,prefix);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBMaster::MBProperty& p )
{
	os << " mbaddr=(" << (int)p.mbaddr << ")" << ModbusRTU::addr2str(p.mbaddr)
				<< " reg=" << ModbusRTU::dat2str(p.mbreg)
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
