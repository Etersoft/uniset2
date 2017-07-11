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
#include <cmath>
#include <sstream>
#include "Extensions.h"
#include "RTUExchange.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
RTUExchange::RTUExchange(uniset::ObjectId objId, uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
						 const std::string& prefix_ ):
	MBExchange(objId, shmId, ic, prefix_),
	mbrtu(0),
	defSpeed(ComPort::ComSpeed38400),
	use485F(false),
	transmitCtl(false),
	rs_pre_clean(false)
{
	if( objId == DefaultObjectId )
		throw uniset::SystemError("(RTUExchange): objId=-1?!! Use --" + prefix + "-name" );

	auto conf = uniset_conf();

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
		string v = conf->getArgParam(p, "");

		if( !v.empty() && v[0] != '-' )
			prop_prefix = v;
		// если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
		else if( findArgParam(p, conf->getArgc(), conf->getArgv()) != -1 )
			prop_prefix = "";
	}

	mbinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML::iterator it(cnode);

	// ---------- init RS ----------
	devname    = conf->getArgParam("--" + prefix + "-dev", it.getProp("device"));

	if( devname.empty() )
		throw uniset::SystemError(myname + "(RTUExchange): Unknown device..." );

	string speed = conf->getArgParam("--" + prefix + "-speed", it.getProp("speed"));

	if( speed.empty() )
		speed = "38400";

	use485F = conf->getArgInt("--" + prefix + "-use485F", it.getProp("use485F"));
	transmitCtl = conf->getArgInt("--" + prefix + "-transmit-ctl", it.getProp("transmitCtl"));
	defSpeed = ComPort::getSpeed(speed);

	sleepPause_msec = conf->getArgPInt("--" + prefix + "-sleepPause-usec", it.getProp("slepePause"), 100);

	rs_pre_clean = conf->getArgInt("--" + prefix + "-pre-clean", it.getProp("pre_clean"));

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(devices);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this, &RTUExchange::readItem) );

	initMB(false);

	if( dlog()->is_info() )
		printMap(devices);
}
// -----------------------------------------------------------------------------
void RTUExchange::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='rs'" << endl;
	MBExchange::help_print(argc, argv);
	//    cout << " Настройки протокола RS: " << endl;
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
	//    delete mbrtu;
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> RTUExchange::initMB( bool reopen )
{
	if( !file_exist(devname) )
	{
		if( mbrtu )
		{
			//          delete mbrtu;
			mb = 0;
			mbrtu = 0;
		}

		return mbrtu;
	}

	if( mbrtu )
	{
		if( !reopen )
			return mbrtu;

		//        delete mbrtu;
		mbrtu = 0;
		mb = 0;
	}

	try
	{
		mbrtu = std::make_shared<ModbusRTUMaster>(devname, use485F, transmitCtl);

		if( defSpeed != ComPort::ComSpeed0 )
			mbrtu->setSpeed(defSpeed);

		auto l = loga->create(myname + "-exchangelog");
		mbrtu->setLog(l);

		if( ic )
			ic->logAgregator()->add(loga);

		if( recv_timeout > 0 )
			mbrtu->setTimeout(recv_timeout);

		mbrtu->setSleepPause(sleepPause_msec);
		mbrtu->setAfterSendPause(aftersend_pause);

		mbinfo << myname << "(init): dev=" << devname << " speed=" << ComPort::getSpeed( mbrtu->getSpeed() ) << endl;
	}
	catch( const uniset::Exception& ex )
	{
		//if( mbrtu )
		//    delete mbrtu;
		mbrtu = 0;

		mbwarn << myname << "(init): " << ex << endl;
	}
	catch(...)
	{
		//        if( mbrtu )
		//            delete mbrtu;
		mbrtu = 0;

		mbinfo << myname << "(init): catch...." << endl;
	}

	mb = mbrtu;
	return mbrtu;
}
// -----------------------------------------------------------------------------
void RTUExchange::step()
{
	try
	{
		if( sidExchangeMode != DefaultObjectId && force )
			exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);
	}
	catch(...) {}

	poll();

	MBExchange::step();
}
// -----------------------------------------------------------------------------
bool RTUExchange::poll()
{
	if( !mb )
	{
		mb = initMB(false);

		if( !isProcActive() )
			return false;

		updateSM();
		allInitOK = false;
		return false;
	}

	if( !allInitOK )
		firstInitRegisters();

	if( !isProcActive() )
		return false;

	ncycle++;
	bool allNotRespond = true;
	ComPort::Speed s = mbrtu->getSpeed();

	for( auto it1 : devices )
	{
		auto d = it1.second;

		if( d->mode_id != DefaultObjectId && d->mode == emSkipExchange )
			continue;

		if( d->speed != s )
		{
			s = d->speed;
			mbrtu->setSpeed(d->speed);
		}

		d->prev_numreply.store(d->numreply);

		if( d->dtype == MBExchange::dtRTU188 )
		{
			if( !d->rtu188 )
				continue;

			dlog3 << myname << "(pollRTU188): poll RTU188 "
				  << " mbaddr=" << ModbusRTU::addr2str(d->mbaddr)
				  << endl;

			try
			{
				if( rs_pre_clean )
					mb->cleanupChannel();

				d->rtu188->poll(mbrtu);
				d->numreply++;
				allNotRespond = false;
			}
			catch( ModbusRTU::mbException& ex )
			{
				if( d->numreply != d->prev_numreply )
				{
					dlog3 << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
						  << " -> " << ex << endl;
				}
			}
		}
		else
		{
			dlog3 << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr)
				  << " regs=" << d->pollmap.size() << endl;

			for( auto && m : d->pollmap )
			{
				if( m.first != 0 && (ncycle % m.first) != 0 )
					continue;

				auto rmap = m.second;

				for( auto && it = rmap->begin(); it != rmap->end(); ++it )
				{
					try
					{
						if( d->dtype == RTUExchange::dtRTU || d->dtype == RTUExchange::dtMTR )
						{
							if( rs_pre_clean )
								mb->cleanupChannel();

							if( pollRTU(d, it) )
							{
								d->numreply++;
								allNotRespond = false;
							}
						}
					}
					catch( ModbusRTU::mbException& ex )
					{
						dlog3 << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
							  << " reg=" << ModbusRTU::dat2str(it->second->mbreg)
							  << " for sensors: ";
						print_plist(dlog()->level3(), it->second->slst);
						dlog()->level3() << " err: " << ex << endl;
					}

					if( it == rmap->end() )
						break;

					if( !isProcActive() )
						return false;
				}
			}
		}
	}

	// update SharedMemory...
	updateSM();

	// check thresholds
	for( auto && t : thrlist )
	{
		if( !isProcActive() )
			return false;

		IOBase::processingThreshold(&t, shm, force);
	}

	if( trReopen.hi(allNotRespond) )
		ptReopen.reset();

	if( allNotRespond && ptReopen.checkTime() )
	{
		mbwarn << myname << ": REOPEN timeout..(" << ptReopen.getInterval() << ")" << endl;

		mb = initMB(true);
		ptReopen.reset();
	}

	//    printMap(rmap);
	return !allNotRespond;
}
// -----------------------------------------------------------------------------
std::shared_ptr<RTUExchange> RTUExchange::init_rtuexchange(int argc, const char* const* argv, uniset::ObjectId icID,
		const std::shared_ptr<SharedMemory>& ic, const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "RTUExchange1");

	if( name.empty() )
	{
		cerr << "(rtuexchange): Unknown 'name'. Use --" << prefix << "-name" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == uniset::DefaultObjectId )
	{
		cerr << "(rtuexchange): Not found ID for '" << name
			 << "'!"
			 << " in section <" << conf->getObjectsSection() << ">" << endl;
		return 0;
	}

	dinfo << "(rtuexchange): name = " << name << "(" << ID << ")" << endl;
	return make_shared<RTUExchange>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
bool RTUExchange::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML::iterator& it )
{
	if( !MBExchange::initDeviceInfo(m, a, it) )
		return false;

	auto d = m.find(a);

	if( d == m.end() )
	{
		mbwarn << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
		return false;
	}

	string s = it.getProp("speed");

	if( !s.empty() )
	{
		d->second->speed = ComPort::getSpeed(s);

		if( d->second->speed == ComPort::ComSpeed0 )
		{
			d->second->speed = defSpeed;
			mbcrit << myname << "(initDeviceInfo): Unknown speed=" << s <<
				   " for addr=" << ModbusRTU::addr2str(a) << endl;
			return false;
		}
	}
	else
		d->second->speed = defSpeed;

	return true;
}
// -----------------------------------------------------------------------------
