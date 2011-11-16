// -----------------------------------------------------------------------------
#include <cmath>
#include <sstream>
#include "Extensions.h"
#include "RTUExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
RTUExchange::RTUExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic,
						  const std::string prefix_ ):
MBExchange(objId,shmId,ic,prefix_),
mbrtu(0),
defSpeed(ComPort::ComSpeed0),
use485F(false),
transmitCtl(false),
rs_pre_clean(false)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(RTUExchange): objId=-1?!! Use --" + prefix + "-name" );

	// префикс для "свойств" - по умолчанию
	prop_prefix = "";
	// если задано поле для "фильтрации"
	// то в качестве префикса используем его
	if( !s_field.empty() )
		prop_prefix = s_field + "_";
	// если "принудительно" задан префикс
	// используем его.
	{
		string p("--" + prefix + "-set-prop-prefix"); 
		string v = conf->getArgParam(p,"");
		if( !v.empty() )
			prop_prefix = v;
		// если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
		else if( findArgParam(p,conf->getArgc(),conf->getArgv()) !=-1 );
			prop_prefix = "";
	}

	dlog[Debug::INFO] << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML_iterator it(cnode);

	// ---------- init RS ----------
	devname	= conf->getArgParam("--"+prefix+"-dev",it.getProp("device"));
	if( devname.empty() )
		throw UniSetTypes::SystemError(myname+"(RTUExchange): Unknown device..." );

	string speed = conf->getArgParam("--"+prefix+"-speed",it.getProp("speed"));
	if( speed.empty() )
		speed = "38400";

	use485F = conf->getArgInt("--"+prefix+"-use485F",it.getProp("use485F"));
	transmitCtl = conf->getArgInt("--"+prefix+"-transmit-ctl",it.getProp("transmitCtl"));
	defSpeed = ComPort::getSpeed(speed);

	sleepPause_usec = conf->getArgPInt("--" + prefix + "-sleepPause-usec",it.getProp("slepePause"), 100);

	rs_pre_clean = conf->getArgInt("--"+prefix+"-pre-clean",it.getProp("pre_clean"));
	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&RTUExchange::readItem) );


	initMB(false);

	if( dlog.debugging(Debug::INFO) )
		printMap(rmap);
}
// -----------------------------------------------------------------------------
void RTUExchange::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='rs'" << endl;
	MBExchange::help_print(argc,argv);
//	cout << " Настройки протокола RS: " << endl;
	cout << "--prefix-dev devname  - файл устройства" << endl;
	cout << "--prefix-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;
	cout << "--prefix-my-addr      - адрес текущего узла" << endl;
	cout << "--prefix-recv-timeout - Таймаут на ожидание ответа." << endl;
	cout << "--prefix-pre-clean    - Очищать буфер перед каждым запросом" << endl;
	cout << "--prefix-sleepPause-usec - Таймаут на ожидание очередного байта" << endl;
}
// -----------------------------------------------------------------------------
RTUExchange::~RTUExchange()
{
	delete mbrtu;
}
// -----------------------------------------------------------------------------
ModbusClient* RTUExchange::initMB( bool reopen )
{
	if( !file_exist(devname) )
	{
		if( mbrtu )
		{
			delete mbrtu;
			mb = 0;
			mbrtu = 0;
		}
		return mbrtu;
	}

	if( mbrtu )
	{
		if( !reopen )
			return mbrtu;
		
		delete mbrtu;
		mbrtu = 0;
		mb = 0;
	}

	try
	{
		mbrtu = new ModbusRTUMaster(devname,use485F,transmitCtl);

		if( defSpeed != ComPort::ComSpeed0 )
			mbrtu->setSpeed(defSpeed);
	
//		mb->setLog(dlog);

		if( recv_timeout > 0 )
			mbrtu->setTimeout(recv_timeout);
		
		mbrtu->setSleepPause(sleepPause_usec);

		dlog[Debug::INFO] << myname << "(init): dev=" << devname << " speed=" << ComPort::getSpeed(defSpeed) << endl;
	}
	catch(...)
	{
		if( mbrtu )
			delete mbrtu;
		mbrtu = 0;
	}

	mb = mbrtu;
	return mbrtu;
}
// -----------------------------------------------------------------------------
void RTUExchange::step()
{
	try
	{
		if( sidExchangeMode != DefaultObjectId && force_out )
			exchangeMode = shm->localGetValue(aitExchangeMode,sidExchangeMode);
	}
	catch(...){}

	{
		uniset_mutex_lock l(pollMutex,2000);
		poll();
	}

	MBExchange::step();
}
// -----------------------------------------------------------------------------
void RTUExchange::poll()
{
	if( trAllNotRespond.hi(allNotRespond) )
		ptTimeout.reset();
	
	if( allNotRespond && mb && ptTimeout.checkTime() )
	{
		ptTimeout.reset();
		initMB(true);
	}
	
	if( !mb )
	{
		initMB(false);
		if( !mb )
		{
			for( MBExchange::RTUDeviceMap::iterator it=rmap.begin(); it!=rmap.end(); ++it )
				it->second->resp_real = false;
		}
		updateSM();
		return;
	}

	ComPort::Speed s = mbrtu->getSpeed();
	
	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
	
		if( d->speed != s )
		{
			s = d->speed;
			mbrtu->setSpeed(d->speed);
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

				d->rtu->poll(mbrtu);
				d->resp_real = true;
			}
			catch( ModbusRTU::mbException& ex )
			{ 
				if( d->resp_real )
				{
					if( dlog.debugging(Debug::LEVEL3) )
					{
  						dlog[Debug::CRIT] << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
							<< " -> " << ex << endl;
					}					
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
//					if( d->resp_real )
//					{
						if( dlog.debugging(Debug::LEVEL3) )
						{
							 dlog[Debug::LEVEL3] << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
								<< " reg=" << ModbusRTU::dat2str(it->second->mbreg)
								<< " for sensors: "; print_plist(dlog(Debug::LEVEL3), it->second->slst);
								dlog(Debug::LEVEL3) << " err: " << ex << endl;
						}
				//		d->resp_real = false;
//					}
				}

				if( it==d->regmap.end() )
					break;
			}
		}
	}

	// update SharedMemory...
	updateSM();
	
	// check thresholds
	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
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
RTUExchange* RTUExchange::init_rtuexchange( int argc, const char* const* argv, UniSetTypes::ObjectId icID,
											SharedMemory* ic, const std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","RTUExchange1");
	if( name.empty() )
	{
		cerr << "(rtuexchange): Unknown 'name'. Use --" << prefix << "-name" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(rtuexchange): Not found ID for '" << name
			<< "'!"
			<< " in section <" << conf->getObjectsSection() << ">" << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(rtuexchange): name = " << name << "(" << ID << ")" << endl;
	return new RTUExchange(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
bool RTUExchange::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it )
{
	if( !MBExchange::initDeviceInfo(m,a,it) )
		return false;
	
	RTUDeviceMap::iterator d = m.find(a);
	if( d == m.end() )
	{
		dlog[Debug::WARN] << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
		return false;
	}
	
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
