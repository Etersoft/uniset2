// -----------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMaster::MBTCPMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, 
							SharedMemory* ic, const std::string prefix ):
UniSetObject_LT(objId),
mb(0),
shm(0),
initPause(0),
force(false),
force_out(false),
mbregFromID(false),
activated(false),
noQueryOptimization(false),
force_disconnect(false),
allNotRespond(false),
prefix(prefix),
no_extimer(false),
poll_count(0)
{
//	cout << "$ $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMaster): objId=-1?!! Use --" + prefix + "-name" );

//	xmlNode* cnode = conf->getNode(myname);
	string conf_name = conf->getArgParam("--" + prefix + "-confnode",myname);

	cnode = conf->getNode(conf_name);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBTCPMaster): Not found node <" + conf_name + " ...> for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	stat_time = conf->getArgPInt("--" + prefix + "-statistic-sec",it.getProp("statistic_sec"),0);
	if( stat_time > 0 )
		ptStatistic.setTiming(stat_time*1000);

	// ---------- init MBTCP ----------
	string pname("--" + prefix + "-gateway-iaddr");
	iaddr	= conf->getArgParam(pname,it.getProp("gateway_iaddr"));
	if( iaddr.empty() )
		throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet addr...(Use: " + pname +")" );

	string tmp("--" + prefix + "-gateway-port");
	port = conf->getArgInt(tmp,it.getProp("gateway_port"));
	if( port <= 0 )
		throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet port...(Use: " + tmp +")" );


	recv_timeout = conf->getArgPInt("--" + prefix + "-recv-timeout",it.getProp("recv_timeout"), 50);

	int alltout = conf->getArgPInt("--" + prefix + "-all-timeout",it.getProp("all_timeout"), 2000);
	ptAllNotRespond.setTiming(alltout);

	noQueryOptimization = conf->getArgInt("--" + prefix + "-no-query-optimization",it.getProp("no_query_optimization"));

	mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id",it.getProp("reg_from_id"));
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	polltime = conf->getArgPInt("--" + prefix + "-polltime",it.getProp("polltime"), 100);

	initPause = conf->getArgPInt("--" + prefix + "-initPause",it.getProp("initPause"), 3000);

	force = conf->getArgInt("--" + prefix + "-force",it.getProp("force"));
	force_out = conf->getArgInt("--" + prefix + "-force-out",it.getProp("force_out"));
	force_disconnect = conf->getArgInt("--" + prefix + "-force-disconnect",it.getProp("force_disconnect"));

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&MBTCPMaster::readItem) );

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id",it.getProp("heartbeat_id"));
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

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max",it.getProp("heartbeat_max"), 10);
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

	activateTimeout	= conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

//	initMB(false);

	printMap(rmap);
//	abort();

	poll_count = -1;
}
// -----------------------------------------------------------------------------
MBTCPMaster::~MBTCPMaster()
{
	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
			delete it->second;

		delete it1->second;
	}
	
	delete mb;
	delete shm;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::initMB( bool reopen )
{
	if( mb )
	{
		if( !reopen )
			return;
		
		delete mb;
		mb = 0;
	}

	try
	{
		ost::Thread::setException(ost::Thread::throwException);
		mb = new ModbusTCPMaster();
	
		ost::InetAddress ia(iaddr.c_str());
		mb->connect(ia,port);
		mb->setForceDisconnect(force_disconnect);

		if( recv_timeout > 0 )
			mb->setTimeout(recv_timeout);

		dlog[Debug::INFO] << myname << "(init): ipaddr=" << iaddr << " port=" << port << endl;
		
		if( dlog.debugging(Debug::LEVEL9) )
			mb->setLog(dlog);
	}
	catch( ModbusRTU::mbException& ex )
	{
		cerr << "(init): " << ex << endl;
	}
	catch(...)
	{
		if( mb )
			delete mb;
		mb = 0;
	}
}
// -----------------------------------------------------------------------------
void MBTCPMaster::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--mbm-sm-ready-timeout","15000");
	if( ready_timeout == 0 )
		ready_timeout = 15000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout, 50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): failed waiting SharedMemory " << ready_timeout << " msec";
		dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}
}
// -----------------------------------------------------------------------------
void MBTCPMaster::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmExchange )
	{
		if( no_extimer )
			askTimer(tm->id,0);
		else
			step();
	}
}
// -----------------------------------------------------------------------------
void MBTCPMaster::step()
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
void MBTCPMaster::poll()
{
	if( trAllNotRespond.hi(allNotRespond) )
		ptAllNotRespond.reset();
	
	if( allNotRespond && mb && ptAllNotRespond.checkTime() )
	{
		ptAllNotRespond.reset();
//		initMB(true);
	}
	
	if( !mb )
	{
		initMB(false);
		if( !mb )
		{
			for( MBTCPMaster::RTUDeviceMap::iterator it=rmap.begin(); it!=rmap.end(); ++it )
				it->second->resp_real = false;
		}
		updateSM();
		return;
	}

		
	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);

		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr) 
				<< " regs=" << d->regmap.size() << endl;

		d->resp_real = true;
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			try
			{
				if( d->dtype==MBTCPMaster::dtRTU || d->dtype==MBTCPMaster::dtMTR )
				{
					pollRTU(d,it);
				}
			}
			catch( ModbusRTU::mbException& ex )
			{ 
				if( d->resp_real )
				{
					dlog[Debug::CRIT] << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr) 
						<< " reg=" << ModbusRTU::dat2str(it->second->mbreg)
						<< " -> " << ex << endl;
				}

				d->resp_real = false;
				if( !d->ask_every_reg )
					break;
			}

			if( it==d->regmap.end() )
				break;
		}

		if( stat_time > 0 )
		{
			poll_count++;
			if( ptStatistic.checkTime() )
			{
				cout << endl << "(poll statistic): number of calls is " << poll_count << " (poll time: " << stat_time << " sec)" << endl << endl;
				ptStatistic.reset();
				poll_count=0;
			}
		}

//			mb->disconnect();
	}

	// update SharedMemory...
	updateSM();
	
	// check thresholds
	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			RegInfo* r(it->second);
			for( PList::iterator i=r->slst.begin(); i!=r->slst.end(); ++i )
				IOBase::processingThreshold( &(*i),shm,force);
		}
	}
	
//	printMap(rmap);
}
// -----------------------------------------------------------------------------
bool MBTCPMaster::pollRTU( RTUDevice* dev, RegMap::iterator& it )
{
	RegInfo* p(it->second);

	if( dlog.debugging(Debug::LEVEL3)  )
	{
		dlog[Debug::LEVEL3] << myname << "(pollRTU): poll "
			<< " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
			<< " mbreg=" << ModbusRTU::dat2str(p->mbreg)
			<< " mboffset=" << p->offset
			<< " mbfunc=" << p->mbfunc
			<< " q_count=" << p->q_count
			<< " mb_init=" << p->mb_init
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
			
			if( !p->mb_init )
			{
//				cerr << "******* mb_init: mbreg=" << ModbusRTU::dat2str(p->mbreg) << endl;
				ModbusRTU::ReadInputRetMessage ret1 = mb->read04(dev->mbaddr,p->mb_init_mbreg,1);
				p->mbval = ret1.data[0];
				p->sm_init = true;
				return true;
			}

//			cerr << "**** mbreg=" << ModbusRTU::dat2str(p->mbreg) << " val=" << ModbusRTU::dat2str(p->mbval) << endl;
			ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06(dev->mbaddr,p->mbreg+p->offset,p->mbval);
		}
		break;

		case ModbusRTU::fnWriteOutputRegisters:
		{
			ModbusRTU::WriteOutputMessage msg(dev->mbaddr,p->mbreg+p->offset);
			for( int i=0; i<p->q_count; i++,it++ )
			{
				if( !it->second->mb_init )
				{
//					cerr << "******* mb_init: mbreg=" << ModbusRTU::dat2str(it->second->mbreg) 
//						<< " mb_init mbreg=" << ModbusRTU::dat2str(it->second->mb_init_mbreg) << endl;
					ModbusRTU::ReadOutputRetMessage ret1 = mb->read03(dev->mbaddr,it->second->mb_init_mbreg,1);
//					cerr << "******* mb_init: mbreg=" << ModbusRTU::dat2str(it->second->mbreg) 
//						<< " mb_init mbreg=" << ModbusRTU::dat2str(it->second->mb_init_mbreg)
//						<< " mbval=" << ret1.data[0] << endl;
					it->second->mbval = ret1.data[0];
					it->second->sm_init = true;
				}
			
				msg.addData(it->second->mbval);
			}
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

			if( !p->mb_init )
			{
//				cerr << "******* mb_init: mbreg=" << ModbusRTU::dat2str(p->mbreg)
//						<< " init mbreg=" << ModbusRTU::dat2str(p->mb_init_mbreg) << endl;
				ModbusRTU::ReadInputStatusRetMessage ret1 = mb->read02(dev->mbaddr,p->mb_init_mbreg,1);
				ModbusRTU::DataBits b(ret1.data[0]);
//				cerr << "******* mb_init_mbreg=" << ModbusRTU::dat2str(p->mb_init_mbreg) 
//						<< " read val=" << (int)b[0] << endl;
				p->mbval = b[0];
				p->sm_init = true;
				return true;
			}

//			cerr << "****(coil) mbreg=" << ModbusRTU::dat2str(p->mbreg) << " val=" << ModbusRTU::dat2str(p->mbval) << endl;
			ModbusRTU::ForceSingleCoilRetMessage ret = mb->write05(dev->mbaddr,p->mbreg+p->offset,p->mbval);
		}
		break;

		case ModbusRTU::fnForceMultipleCoils:
		{
			if( !p->mb_init )
			{
				// every register ask... (for mb_init_mbreg no some)
				for( int i=0; i<p->q_count; i++,it++ )
				{
					ModbusRTU::ReadInputStatusRetMessage ret1 = mb->read02(dev->mbaddr,it->second->mb_init_mbreg,1);
					ModbusRTU::DataBits b(ret1.data[0]);
					it->second->mbval = b[0] ? 1 : 0;
					it->second->sm_init = true;
				}
/*
				// alone query for all register (if mb_init_mbreg ++ )
				ModbusRTU::ReadInputStatusRetMessage ret1 = mb->read02(dev->mbaddr,p->mb_init_mbreg,p->q_count);
				int m=0;
				for( int i=0; i<ret1.bcnt; i++ )
				{
					ModbusRTU::DataBits b(ret1.data[i]);
					for( int k=0;k<ModbusRTU::BitsPerByte && m<p->q_count; k++,it++,m++ )
					{
						it->second->mbval = b[k] ? 1 : 0;
						it->second->sm_init = true;
					}
				}
*/
				p->sm_init = true;
				it--;
				return true;
			}

			ModbusRTU::ForceCoilsMessage msg(dev->mbaddr,p->mbreg+p->offset);
			for( int i=0; i<p->q_count; i++,it++ )
				msg.addBit( (it->second->mbval ? true : false) );

			it--;
//			cerr << "*********** (write multiple): " << msg << endl;
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
bool MBTCPMaster::RTUDevice::checkRespond()
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
void MBTCPMaster::updateSM()
{
	allNotRespond = true;
	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		
		if( dlog.debugging(Debug::LEVEL4) )
		{
			dlog[Debug::LEVEL4] << "check respond addr=" << ModbusRTU::addr2str(d->mbaddr) 
				<< " respond=" << d->resp_id 
				<< " real=" << d->resp_real
				<< " state=" << d->resp_state
				<< endl;
		}
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
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			try
			{
				if( d->dtype == dtRTU )
					updateRTU(it);
				else if( d->dtype == dtMTR )
					updateMTR(it);
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
void MBTCPMaster::processingMessage(UniSetTypes::VoidMessage *msg)
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
void MBTCPMaster::sysCommand( UniSetTypes::SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( rmap.empty() )
			{
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* ITEM MAP EMPTY! terminated... *************" << endl;
				raise(SIGTERM);
				return; 
			}

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << "(sysCommand): device map size= " << rmap.size() << endl;

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
			// (т.е. MBTCPMaster  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(MBTCPMaster)
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
void MBTCPMaster::initOutput()
{
}
// ------------------------------------------------------------------------------------------
void MBTCPMaster::askSensors( UniversalIO::UIOCommand cmd )
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

	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
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
void MBTCPMaster::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	if( force_out )
		return;

	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
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
							<< " val=" << sm->value 
							<< " mb_init=" << i->reg->mb_init << endl;
					}

					if( !i->reg->mb_init )
						continue;

					i->value = sm->value;
					updateRSProperty( &(*i),true);
					return;
				}
			}
		}
	}
}
// ------------------------------------------------------------------------------------------
bool MBTCPMaster::activateObject()
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
void MBTCPMaster::sigterm( int signo )
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
void MBTCPMaster::readConfiguration()
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
bool MBTCPMaster::check_item( UniXML_iterator& it )
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

bool MBTCPMaster::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------
MBTCPMaster::RTUDevice* MBTCPMaster::addDev( RTUDeviceMap& mp, ModbusRTU::ModbusAddr a, UniXML_iterator& xmlit )
{
	RTUDeviceMap::iterator it = mp.find(a);
	if( it != mp.end() )
	{
		DeviceType dtype = getDeviceType(xmlit.getProp("tcp_mbtype"));
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

	MBTCPMaster::RTUDevice* d = new MBTCPMaster::RTUDevice();
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
MBTCPMaster::RegInfo* MBTCPMaster::addReg( RegMap& mp, ModbusRTU::ModbusData r, 
											UniXML_iterator& xmlit, MBTCPMaster::RTUDevice* dev,
											MBTCPMaster::RegInfo* rcopy )
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
	
	MBTCPMaster::RegInfo* ri;
	if( rcopy )
	{
		ri = new MBTCPMaster::RegInfo(*rcopy);
		ri->slst.clear();
		ri->mbreg = r;
	}
	else
	{
		ri = new MBTCPMaster::RegInfo();
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
MBTCPMaster::RSProperty* MBTCPMaster::addProp( PList& plist, RSProperty& p )
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
bool MBTCPMaster::initRSProperty( RSProperty& p, UniXML_iterator& it )
{
	if( !IOBase::initItem(&p,it,shm,&dlog,myname) )
		return false;

	if( it.getIntProp("tcp_rawdata") )
	{
		p.cal.minRaw = 0;
		p.cal.maxRaw = 0;
		p.cal.minCal = 0;
		p.cal.maxCal = 0;
		p.cal.precision = 0;
		p.cdiagram = 0;
	}

	string stype( it.getProp("tcp_iotype") );
	if( !stype.empty() )
	{
		p.stype = UniSetTypes::getIOType(stype);
		if( p.stype == UniversalIO::UnknownIOType )
		{
			if( dlog )
				dlog[Debug::CRIT] << myname << "(IOBase::readItem): неизвестный iotype=: " 
					<< stype << " for " << it.getProp("name") << endl;
			return false;
		}
	}
	
	string sbit(it.getProp("tcp_nbit"));
	if( !sbit.empty() )
	{
		p.nbit = UniSetTypes::uni_atoi(sbit.c_str());
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

	string sbyte(it.getProp("tcp_nbyte"));
	if( !sbyte.empty() )
	{
		p.nbyte = UniSetTypes::uni_atoi(sbyte.c_str());
		if( p.nbyte < 0 || p.nbyte > VTypes::Byte::bsize )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): BAD nbyte=" << p.nbyte 
				<< ". (0 >= nbyte < " << VTypes::Byte::bsize << ")." << endl;
			return false;
		}
	}
	
	string vt(it.getProp("tcp_vtype"));
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
bool MBTCPMaster::initRegInfo( RegInfo* r, UniXML_iterator& it,  MBTCPMaster::RTUDevice* dev )
{
	r->dev = dev;
	r->mbval = it.getIntProp("default");
	r->offset = it.getIntProp("tcp_mboffset");
	r->mb_init = it.getIntProp("tcp_mbinit");
	
	if( dev->dtype == MBTCPMaster::dtRTU )
	{
//		dlog[Debug::INFO] << myname << "(initRegInfo): init RTU.." 
	}
	else if( dev->dtype == MBTCPMaster::dtMTR )
	{
		// only for MTR
		if( !initMTRitem(it,r) )
			return false;
	}
	else
	{
		dlog[Debug::CRIT] << myname << "(initRegInfo): Unknown mbtype='" << dev->dtype
				<< "' for " << it.getProp("name") << endl;
		return false;
	}
	
	if( mbregFromID )
	{
		if( it.getProp("id").empty() )
			r->mbreg = conf->getSensorID(it.getProp("name"));
		else
			r->mbreg = it.getIntProp("id");
	}
	else
	{
		string sr = it.getProp("tcp_mbreg");
		if( sr.empty() )
		{
			dlog[Debug::CRIT] << myname << "(initItem): Unknown 'mbreg' for " << it.getProp("name") << endl;
			return false;
		}
		r->mbreg = ModbusRTU::str2mbData(sr);
	}

	{
		string sr = it.getProp("tcp_init_mbreg");
		if( sr == "-1" )
		{
			r->mb_init = true; // OFF mb_init
			r->sm_init = true;
		}
		else if( sr.empty() )
			r->mb_init_mbreg = r->mbreg;
		else
			r->mb_init_mbreg = ModbusRTU::str2mbData(sr);
	}

	r->mbfunc 	= ModbusRTU::fnUnknown;
	string f = it.getProp("tcp_mbfunc");
	if( !f.empty() )
	{
		r->mbfunc = (ModbusRTU::SlaveFunctionCode)UniSetTypes::uni_atoi(f.c_str());
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
MBTCPMaster::DeviceType MBTCPMaster::getDeviceType( const std::string dtype )
{
	if( dtype.empty() )
		return dtUnknown;
	
	if( dtype == "mtr" || dtype == "MTR" )
		return dtMTR;
	
	if( dtype == "rtu" || dtype == "RTU" )
		return dtRTU;
	
	return dtUnknown;
}
// ------------------------------------------------------------------------------------------
bool MBTCPMaster::initRTUDevice( RTUDevice* d, UniXML_iterator& it )
{
	d->dtype = getDeviceType(it.getProp("tcp_mbtype"));

	if( d->dtype == dtUnknown )
	{
		dlog[Debug::CRIT] << myname << "(initRTUDevice): Unknown tcp_mbtype=" << it.getProp("tcp_mbtype")
			<< ". Use: rtu " 
			<< " for " << it.getProp("name") << endl;
		return false;
	}

	string addr = it.getProp("tcp_mbaddr");
	if( addr.empty() )
	{
		dlog[Debug::CRIT] << myname << "(initRTUDevice): Unknown mbaddr for " << it.getProp("name") << endl;
		return false;
	}

	d->mbaddr = ModbusRTU::str2mbAddr(addr);
	return true;
}
// ------------------------------------------------------------------------------------------

bool MBTCPMaster::initItem( UniXML_iterator& it )
{
	RSProperty p;
	if( !initRSProperty(p,it) )
		return false;

	string addr = it.getProp("tcp_mbaddr");
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
	else
	{
		string reg = it.getProp("tcp_mbreg");
		if( reg.empty() )
		{	
			dlog[Debug::CRIT] << myname << "(initItem): unknown mbreg for " << it.getProp("name") << endl;
			return false;
		}
		mbreg = ModbusRTU::str2mbData(reg);
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

	// п÷п═п·п▓п•п═п п░!
	// п╣я│п╩п╦ я└я┐п╫п╨я├п╦я▐ п╫п╟ п╥п╟п©п╦я│я▄, я┌п╬ п╫п╟п╢п╬ п©я─п╬п╡п╣я─п╦я┌я▄
	// я┤я┌п╬ п╬п╢п╦п╫ п╦ я┌п╬я┌п╤п╣ я─п╣пЁп╦я│я┌я─ п╫п╣ п©п╣я─п╣п╥п╟п©п╦я┬я┐я┌ п╫п╣я│п╨п╬п╩я▄п╨п╬ п╢п╟я┌я┤п╦п╨п╬п╡
	// я█я┌п╬ п╡п╬п╥п╪п╬п╤п╫п╬ я┌п╬п╩я▄п╨п╬, п╣я│п╩п╦ п╬п╫п╦ п©п╦я┬я┐я┌ п╠п╦я┌я▀!!
	// п≤п╒п·п⌠:
	// п•я│п╩п╦ п╢п╩я▐ я└я┐п╫п╨я├п╦п╧ п╥п╟п©п╦я│п╦ я│п©п╦я│п╬п╨ п╢п╟я┌я┤п╦п╨п╬п╡ п╫п╟ п╬п╢п╦п╫ я─п╣пЁп╦я│я┌я─ > 1
	// п╥п╫п╟я┤п╦я┌ п╡ я│п©п╦я│п╨п╣ п╪п╬пЁя┐я┌ п╠я▀я┌я▄ я┌п╬п╩я▄п╨п╬ п╠п╦я┌п╬п╡я▀п╣ п╢п╟я┌я┤п╦п╨п╦
	// п╦ п╣я│п╩п╦ п╦п╢я▒я┌ п©п╬п©я▀я┌п╨п╟ п╡п╫п╣я│я┌п╦ п╡ я│п©п╦я│п╬п╨ п╫п╣ п╠п╦я┌п╬п╡я▀п╧ п╢п╟я┌я┤п╦п╨ я┌п╬ п·п╗п≤п▒п п░!
	// п≤ п╫п╟п╬п╠п╬я─п╬я┌: п╣я│п╩п╦ п╦п╢я▒я┌ п©п╬п©я▀я┌п╨п╟ п╡п╫п╣я│я┌п╦ п╠п╦я┌п╬п╡я▀п╧ п╢п╟я┌я┤п╦п╨, п╟ п╡ я│п©п╦я│п╨п╣
	// я┐п╤п╣ я│п╦п╢п╦я┌ п╢п╟я┌я┤п╦п╨ п╥п╟п╫п╦п╪п╟я▌я┴п╦п╧ я├п╣п╩я▀п╧ я─п╣пЁп╦я│я┌я─, я┌п╬ я┌п╬п╤п╣ п·п╗п≤п▒п п░!
	if(	ModbusRTU::isWriteFunction(ri->mbfunc) )
	{
		if( p.nbit<0 &&  ri->slst.size() > 1 )
		{
			dlog[Debug::CRIT] << myname << "(initItem): FAILED! Sharing SAVE (not bit saving) to "
				<< " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg)
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
		{
			MBTCPMaster::RegInfo* ri1 = addReg(dev->regmap,mbreg+i,it,dev,ri);
			ri1->mb_init_mbreg = ri->mb_init_mbreg+i;
		}
	}
	
	return true;
}
// ------------------------------------------------------------------------------------------
bool MBTCPMaster::initMTRitem( UniXML_iterator& it, RegInfo* p )
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
void MBTCPMaster::initIterators()
{
	shm->initAIterator(aitHeartBeat);

	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		shm->initDIterator(d->resp_dit);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
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
void MBTCPMaster::help_print( int argc, const char* const* argv )
{
	cout << "--mbm-polltime msec     - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--mbm-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--mbm-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--mbm-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;    
	cout << "--mbm-force             - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
	cout << "--mbm-initPause         - Задержка перед инициализацией (время на активизация процесса)" << endl;
//	cout << "--mbm-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола TCP: " << endl;
	cout << "--mbm-recv-timeout - Таймаут на ожидание ответа." << endl;
}
// -----------------------------------------------------------------------------
MBTCPMaster* MBTCPMaster::init_mbmaster( int argc, const char* const* argv, UniSetTypes::ObjectId icID, SharedMemory* ic, 
											const std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","MBTCPMaster1");
	if( name.empty() )
	{
		cerr << "(MBTCPMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(MBTCPMaster): идентификатор '" << name 
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(MBTCPMaster): name = " << name << "(" << ID << ")" << endl;
	return new MBTCPMaster(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const MBTCPMaster::DeviceType& dt )
{
	switch(dt)
	{
		case MBTCPMaster::dtRTU:
			os << "RTU";
		break;
		
		case MBTCPMaster::dtMTR:
			os << "MTR";
		break;
		
		default:
			os << "Unknown device type (" << (int)dt << ")";
		break;
	}
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const MBTCPMaster::RSProperty& p )
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
		os 	<< p.cal
			<< " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
	}		
	
	return os;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::initDeviceList()
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
bool MBTCPMaster::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it )
{
	RTUDeviceMap::iterator d = m.find(a);
	if( d == m.end() )
	{
		dlog[Debug::WARN] << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
		return false;
	}

	d->second->ask_every_reg = it.getIntProp("ask_every_reg");

	dlog[Debug::INFO] << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) 
			<< " ask_every_reg=" << d->second->ask_every_reg << endl;

	string s(it.getProp("respondSensor"));
	if( !s.empty() )
	{
		d->second->resp_id = conf->getSensorID(s);
		if( d->second->resp_id == DefaultObjectId )
		{
			dlog[Debug::CRIT] << myname << "(initDeviceInfo): not found ID for noRespondSensor=" << s << endl;
			return false;
		}
    }
    
	dlog[Debug::INFO] << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) << endl;
	int tout = it.getPIntProp("timeout", UniSetTimer::WaitUpTime);
	d->second->resp_ptTimeout.setTiming(tout);
	d->second->resp_invert = it.getIntProp("invert");
//	d->second->no_clean_input = it.getIntProp("no_clean_input");
	
//	dlog[Debug::INFO] << myname << "(initDeviceInfo): add " << d->second << endl;
	
	return true;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::printMap( MBTCPMaster::RTUDeviceMap& m )
{
	cout << "devices: " << endl;
	for( MBTCPMaster::RTUDeviceMap::iterator it=m.begin(); it!=m.end(); ++it )
	{
		cout << "  " <<  *(it->second) << endl;
	}
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBTCPMaster::RTUDeviceMap& m )
{
	os << "devices: " << endl;
	for( MBTCPMaster::RTUDeviceMap::iterator it=m.begin(); it!=m.end(); ++it )
	{
		os << "  " <<  *(it->second) << endl;
	}
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBTCPMaster::RTUDevice& d )
{
  		os 	<< "addr=" << ModbusRTU::addr2str(d.mbaddr)
  			<< " type=" << d.dtype
  			<< " respond_id=" << d.resp_id
  			<< " respond_timeout=" << d.resp_ptTimeout.getInterval()
  			<< " respond_state=" << d.resp_state
  			<< " respond_invert=" << d.resp_invert
  			<< endl;
  			

		os << "  regs: " << endl;
		for( MBTCPMaster::RegMap::iterator it=d.regmap.begin(); it!=d.regmap.end(); ++it )
			os << "     " << *(it->second) << endl;
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBTCPMaster::RegInfo& r )
{
	os << " mbreg=" << ModbusRTU::dat2str(r.mbreg)
		<< " mbfunc=" << r.mbfunc
		<< " q_num=" << r.q_num
		<< " q_count=" << r.q_count
		<< " value=" << ModbusRTU::dat2str(r.mbval) << "(" << (int)r.mbval << ")"
		<< " mtrType=" << MTR::type2str(r.mtrType)
		<< endl;

	for( MBTCPMaster::PList::iterator it=r.slst.begin(); it!=r.slst.end(); ++it )
		os << "         " << (*it) << endl;

	return os;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::rtuQueryOptimization( RTUDeviceMap& m )
{
	if( noQueryOptimization )
		return;

	dlog[Debug::INFO] << myname << "(rtuQueryOptimization): optimization..." << endl;

	// MAXLEN/2 - я█я┌п╬ п╨п╬п╩п╦я┤п╣я│я┌п╡п╬ я│п╩п╬п╡ п╢п╟п╫п╫я▀я┘ п╡ п╬я┌п╡п╣я┌п╣
	// 10 - п╫п╟ п╡я│я▐п╨п╦п╣ я│п╩я┐п╤п╣п╠п╫я▀п╣ п╥п╟пЁп╬п╩п╬п╡п╨п╦
	int maxcount = ModbusRTU::MAXLENPACKET/2 - 10;

	for( MBTCPMaster::RTUDeviceMap::iterator it1=m.begin(); it1!=m.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			MBTCPMaster::RegMap::iterator beg = it;
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
void MBTCPMaster::updateRTU( RegMap::iterator& rit )
{
	RegInfo* r(rit->second);
	for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
		updateRSProperty( &(*it),false );

	if( r->sm_init )
		r->mb_init = true;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::updateRSProperty( RSProperty* p, bool write_only )
{
	using namespace ModbusRTU;
	RegInfo* r(p->reg->rit->second);

	bool save = isWriteFunction( r->mbfunc );
	
	if( !save && write_only )
		return;

	if( dlog.debugging(Debug::LEVEL3) )
		dlog[Debug::LEVEL3] << "updateP: sid=" << p->si.id 
				<< " mbval=" << r->mbval 
				<< " vtype=" << p->vType
				<< " rnum=" << p->rnum
				<< " nbit=" << p->nbit
				<< " save=" << save
				<< " ioype=" << p->stype
				<< " mb_init=" << r->mb_init
				<< endl;

		try
		{
			if( p->vType == VTypes::vtUnknown )
			{
				ModbusRTU::DataBits16 b(r->mbval);
				if( p->nbit >= 0 )
				{
					if( save && r->mb_init )
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
					if( save && r->mb_init )
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
				if( save && r->mb_init )
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
						IOBase::processingAsAI( p, (signed short)(r->mbval), shm, force );
					}
				}
				return;
			}
			else if( p->vType == VTypes::vtUnsigned )
			{
				if( save && r->mb_init )
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
						IOBase::processingAsAI( p, (unsigned short)r->mbval, shm, force );
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

				if( save && r->mb_init )
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
				if( save && r->mb_init )
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
				if( save && r->mb_init )
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
void MBTCPMaster::updateMTR( RegMap::iterator& rit )
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
				
				if( r->mtrType == MTR::mtT16 )
				{
					if( save )
					{
						MTR::T16 t(IOBase::processingFasAO( &(*it), shm, force_out ));
						r->mbval = t.val;
					}
					else
					{
						MTR::T16 t(r->mbval);
						IOBase::processingFasAI( &(*it), t.fval, shm, force );
					}
					continue;
				}

				if( r->mtrType == MTR::mtT17 )
				{
					if( save )
					{
						MTR::T17 t(IOBase::processingFasAO( &(*it), shm, force_out ));
						r->mbval = t.val;
					}
					else
					{
						MTR::T17 t(r->mbval);
						IOBase::processingFasAI( &(*it), t.fval, shm, force );
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
void MBTCPMaster::execute()
{
	no_extimer = true;

	try
	{
		askTimer(tmExchange,0);
	}
	catch(...){}
	
	initMB(false);

	while(1)
	{
		try
		{
			step();
		}
		catch( Exception& ex )
		{
			dlog[Debug::CRIT] << myname << "(execute): " << ex << std::endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(execute): catch ..." << endl;
		}

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
