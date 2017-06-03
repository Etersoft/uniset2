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
#include <sstream>
#include "ORepHelpers.h"
#include "UniSetTypes.h"
#include "Extensions.h"
#include "IOControl.h"
#include "IOLogSugar.h"
// -----------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	using namespace std;
	using namespace extensions;
	// -----------------------------------------------------------------------------
	std::ostream& operator<<( std::ostream& os, IOControl::IOInfo& inf )
	{
		os << "(" << inf.si.id << ")" << uniset_conf()->oind->getMapName(inf.si.id)
		   << " card=" << inf.ncard << " channel=" << inf.channel << " subdev=" << inf.subdev
		   << " aref=" << inf.aref << " range=" << inf.range
		   << " default=" << inf.defval << " safety=" << inf.safety;

		if( inf.cal.minRaw != inf.cal.maxRaw )
			os << inf.cal;

		return os;
	}
	// -----------------------------------------------------------------------------

	IOControl::IOControl(uniset::ObjectId id, uniset::ObjectId icID,
						 const std::shared_ptr<SharedMemory>& ic, int numcards, const std::string& prefix_ ):
		UniSetObject(id),
		polltime(150),
		cards(11),
		noCards(true),
		iomap(100),
		maxItem(0),
		filterT(0),
		myid(id),
		prefix(prefix_),
		blink_state(true),
		blink2_state(true),
		blink3_state(true),
		testLamp_s(uniset::DefaultObjectId),
		isTestLamp(false),
		sidHeartBeat(uniset::DefaultObjectId),
		force(false),
		force_out(false),
		maxCardNum(10),
		activated(false),
		readconf_ok(false),
		term(false),
		testMode_as(uniset::DefaultObjectId),
		testmode(tmNone),
		prev_testmode(tmNone)
	{
		auto conf = uniset_conf();

		string cname = conf->getArgParam("--" + prefix + "-confnode", myname);
		confnode = conf->getNode(cname);

		if( confnode == NULL )
			throw SystemError("Not found conf-node " + cname + " for " + myname);

		iolog = make_shared<DebugStream>();
		iolog->setLogName(myname);
		conf->initLogStream(iolog, prefix + "-log");

		loga = make_shared<LogAgregator>(myname + "-loga");
		loga->add(iolog);
		loga->add(ulog());
		loga->add(dlog());

		defCardNum = conf->getArgInt("--" + prefix + "-default-cardnum", "-1");
		maxCardNum = conf->getArgInt("--" + prefix + "-max-cardnum", "10");
		cards.resize(maxCardNum + 1);

		ioinfo << myname << "(init): numcards=" << numcards << endl;

		UniXML::iterator it(confnode);

		logserv = make_shared<LogServer>(loga);
		logserv->init( prefix + "-logserver", confnode );

		if( findArgParam("--" + prefix + "-run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
		{
			logserv_host = conf->getArg2Param("--" + prefix + "-logserver-host", it.getProp("logserverHost"), "localhost");
			logserv_port = conf->getArgPInt("--" + prefix + "-logserver-port", it.getProp("logserverPort"), getId());
		}

		if( ic )
			ic->logAgregator()->add(loga);

		noCards = true;

		for( unsigned int i = 1; i < cards.size(); i++ )
			cards[i] = NULL;

		buildCardsList();

		for( unsigned int i = 1; i < cards.size(); i++ )
		{
			stringstream s1;
			s1 << "--" << prefix << "-dev" << i;
			stringstream s2;
			s2 << "iodev" << i;

			string iodev = conf->getArgParam(s1.str(), it.getProp(s2.str()));

			if( iodev.empty() )
				continue;

			if( iodev == "/dev/null" )
			{
				if( cards[i] == NULL )
				{
					iolog3 << myname << "(init): Card N" << i
						   << " DISABLED! dev='"
						   << iodev << "'" << endl;
				}
			}
			else
			{
				noCards = false;
				cards[i] = new ComediInterface(iodev);
				iolog3 << myname << "(init): ADD card" << i  << " dev=" << iodev << endl;
			}

			if( cards[i] != NULL )
			{
				for( unsigned int s = 1; s <= 4; s++ )
				{
					stringstream t1;
					t1 << s1.str() << "-subdev" << s << "-type";

					string stype( conf->getArgParam(t1.str()) );

					if( !stype.empty() )
					{
						ComediInterface::SubdevType st = ComediInterface::str2type(stype.c_str());

						if( !stype.empty() && st == ComediInterface::Unknown )
						{
							ostringstream err;
							err << "Unknown subdev type '" << stype << " for " << t1.str();
							throw SystemError(err.str());
						}

						if( !stype.empty() )
						{
							ioinfo << myname
								   << "(init): card" << i
								   << " subdev" << s << " set type " << stype << endl;

							cards[i]->configureSubdev(s - 1, st);
						}
					}
				}
			}
		}

		ioinfo << myname << "(init): result numcards=" << cards.size() << endl;

		polltime = conf->getArgInt("--" + prefix + "-polltime", it.getProp("polltime"));

		if( !polltime )
			polltime = 100;

		force         = conf->getArgInt("--" + prefix + "-force", it.getProp("force"));
		force_out     = conf->getArgInt("--" + prefix + "-force-out", it.getProp("force_out"));

		filtersize = conf->getArgPInt("--" + prefix + "-filtersize", it.getProp("filtersize"), 1);

		filterT = atof(conf->getArgParam("--" + prefix + "-filterT", it.getProp("filterT")).c_str());

		string testlamp = conf->getArgParam("--" + prefix + "-test-lamp", it.getProp("testlamp_s"));

		if( !testlamp.empty() )
		{
			testLamp_s = conf->getSensorID(testlamp);

			if( testLamp_s == DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": Unkown ID for " << testlamp;
				iocrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}

			ioinfo << myname << "(init): testLamp_S='" << testlamp << "'" << endl;
		}

		string tmode = conf->getArgParam("--" + prefix + "-test-mode", it.getProp("testmode_as"));

		if( !tmode.empty() )
		{
			testMode_as = conf->getSensorID(tmode);

			if( testMode_as == DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": Unknown ID for " << tmode;
				iocrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}

			ioinfo << myname << "(init): testMode_as='" << testmode << "'" << endl;
		}

		shm = make_shared<SMInterface>(icID, ui, myid, ic);

		// определяем фильтр
		s_field = conf->getArgParam("--" + prefix + "-s-filter-field");
		s_fvalue = conf->getArgParam("--" + prefix + "-s-filter-value");

		ioinfo << myname << "(init): read s_field='" << s_field
			   << "' s_fvalue='" << s_fvalue << "'" << endl;

		int blink_msec = conf->getArgPInt("--" + prefix + "-blink-time", it.getProp("blink-time"), 300);
		ptBlink.setTiming(blink_msec);

		int blink2_msec = conf->getArgPInt("--" + prefix + "-blink2-time", it.getProp("blink2-time"), 150);
		ptBlink2.setTiming(blink2_msec);

		int blink3_msec = conf->getArgPInt("--" + prefix + "-blink3-time", it.getProp("blink3-time"), 100);
		ptBlink3.setTiming(blink3_msec);

		int sm_tout = conf->getArgInt("--" + prefix + "-sm-ready-timeout", it.getProp("ready_timeout"));

		if( sm_tout == 0 )
			smReadyTimeout = conf->getNCReadyTimeout();
		else if( sm_tout < 0 )
			smReadyTimeout = UniSetTimer::WaitUpTime;
		else
			smReadyTimeout = sm_tout;

		string sm_ready_sid = conf->getArgParam("--" + prefix + "-sm-ready-test-sid", it.getProp("sm_ready_test_sid"));
		sidTestSMReady = conf->getSensorID(sm_ready_sid);

		if( sidTestSMReady == DefaultObjectId )
		{
			sidTestSMReady = conf->getSensorID("TestMode_S");
			iowarn << myname
				   << "(init): Unknown ID for sm-ready-test-sid (--" << prefix << "-sm-ready-test-sid)."
				   << " Use 'TestMode_S'" << endl;
		}
		else
			ioinfo << myname << "(init): test-sid: " << sm_ready_sid << endl;


		// -----------------------
		string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", it.getProp("heartbeat_id"));

		if( !heart.empty() )
		{
			sidHeartBeat = conf->getSensorID(heart);

			if( sidHeartBeat == DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": Not found ID for 'HeartBeat' " << heart;
				iocrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}

			int heartbeatTime = conf->getArgInt("--heartbeat-check-time", "1000");

			if( heartbeatTime )
				ptHeartBeat.setTiming(heartbeatTime);
			else
				ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

			maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", it.getProp("heartbeat_max"), 10);
		}

		activateTimeout    = conf->getArgPInt("--" + prefix + "-activate-timeout", 25000);

		if( !shm->isLocalwork() ) // ic
			ic->addReadItem( sigc::mem_fun(this, &IOControl::readItem) );
	}

	// --------------------------------------------------------------------------------

	IOControl::~IOControl()
	{
		// здесь бы ещё пройтись по списку с сделать delete для
		// всех cdiagram созданных через new
		//
		for( unsigned int i = 0; i < cards.size(); i++ )
			delete cards[i];
	}

	// --------------------------------------------------------------------------------
	void IOControl::execute()
	{
		//    set_signals(true);
		UniXML::iterator it(confnode);

		waitSM(); // необходимо дождаться, чтобы нормально инициализировать итераторы

		PassiveTimer pt(UniSetTimer::WaitUpTime);

		if( shm->isLocalwork() )
		{
			maxItem = 0;
			readConfiguration();
			iomap.resize(maxItem);
			cerr << "************************** readConfiguration: " << pt.getCurrent() << " msec " << endl;
		}
		else
		{
			iomap.resize(maxItem);

			// init iterators
			for( auto& it : iomap )
			{
				shm->initIterator(it.ioit);
				shm->initIterator(it.t_ait);
			}

			readconf_ok = true; // т.к. waitSM() уже был...
		}

		maxHalf = maxItem / 2;
		ioinfo << myname << "(init): iomap size = " << iomap.size() << endl;

		//cerr << myname << "(iomap size): " << iomap.size() << endl;

		// чтение параметров по входам-выходам
		initIOCard();

		bool skip_iout = uniset_conf()->getArgInt("--" + prefix + "-skip-init-output");

		if( !skip_iout )
			initOutputs();

		shm->initIterator(itHeartBeat);
		shm->initIterator(itTestLamp);
		shm->initIterator(itTestMode);

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
			iocrit << myname << "(execute): ************* don`t activate?! ************" << endl;

		try
		{
			// init first time....
			if( !force && !noCards )
			{
				std::lock_guard<std::mutex> l(iopollMutex);
				force = true;
				iopoll();
				force = false;
			}
		}
		catch(...) {}

		while( !term )
		{
			try
			{

				if( !noCards )
				{
					check_testmode();
					check_testlamp();

					if( ptBlink.checkTime() )
					{
						ptBlink.reset();

						try
						{
							blink(lstBlink, blink_state);
						}
						catch(...) {}
					}

					if( ptBlink2.checkTime() )
					{
						ptBlink2.reset();

						try
						{
							blink(lstBlink2, blink2_state);
						}
						catch(...) {}
					}

					if( ptBlink3.checkTime() )
					{
						ptBlink3.reset();

						try
						{
							blink(lstBlink3, blink3_state);
						}
						catch(...) {}
					}

					{
						std::lock_guard<std::mutex> l(iopollMutex);
						iopoll();
					}
				}

				if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
				{
					shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, myid);
					ptHeartBeat.reset();
				}
			}
			catch( const uniset::Exception& ex )
			{
				iolog3 << myname << "(execute): " << ex << endl;
			}
			catch( const CORBA::SystemException& ex )
			{
				iolog3 << myname << "(execute): CORBA::SystemException: "
					   << ex.NP_minorString() << endl;
			}
			catch(...)
			{
				iolog3 << myname << "(execute): catch ..." << endl;
			}

			if( term )
				break;

			msleep( polltime );
		}

		term = false;
	}
	// --------------------------------------------------------------------------------
	void IOControl::iopoll()
	{
		if( testmode == tmOffPoll )
			return;

		// Опрос приоритетной очереди
		for( auto it : pmap )
		{
			if( it.priority > 0 )
			{
				ioread( &(iomap[it.index]) );
				IOBase::processingThreshold((IOBase*) & (iomap[it.index]), shm, force);
			}
		}

		bool prior = false;
		unsigned int i = 0;

		for( auto it = iomap.begin(); it != iomap.end(); ++it, i++ )
		{
			if( it->ignore )
				continue;

			IOBase::processingThreshold((IOBase*) & (*it), shm, force);

			ioread( (IOInfo*) & (*it) );

			// на середине
			// опять опросим приоритетные
			if( !prior && i > maxHalf )
			{
				for( auto& p : pmap )
				{
					if( p.priority > 1 )
					{
						ioread( &(iomap[p.index]) );
						IOBase::processingThreshold((IOBase*) & (iomap[p.index]), shm, force);
					}
				}

				prior = true;
			}
		}

		// Опрос приоритетной очереди
		for( auto& it : pmap )
		{
			if( it.priority > 2 )
			{
				ioread( &(iomap[it.index]) );
				IOBase::processingThreshold((IOBase*) & (iomap[it.index]), shm, force);
			}
		}
	}
	// --------------------------------------------------------------------------------
	void IOControl::ioread( IOInfo* it )
	{
		//    cout  << conf->oind->getMapName(it->si.id)  << " ignore=" << it->ignore << " ncard=" << it->ncard << endl;

		if( it->ignore || it->ncard == defCardNum )
			return;

		if( testmode != tmNone )
		{
			if( testmode == tmConfigEnable && !it->enable_testmode )
				return;

			if( testmode == tmConfigDisable && it->disable_testmode )
				return;

			if( testmode == tmOnlyInputs &&
					it->stype != UniversalIO::AI &&
					it->stype != UniversalIO::DI )
				return;

			if( testmode == tmOnlyOutputs &&
					it->stype != UniversalIO::AO &&
					it->stype != UniversalIO::DO )
				return;
		}

		ComediInterface* card = cards.getCard(it->ncard);

		if( card == NULL || it->subdev == DefaultSubdev || it->channel == DefaultChannel )
			return;

		if( it->si.id == DefaultObjectId )
		{
			iolog3 << myname << "(iopoll): sid=DefaultObjectId?!" << endl;
			return;
		}

		IOBase* ib = static_cast<IOBase*>(it);

		try
		{
			if( it->stype == UniversalIO::AI )
			{
				int val = card->getAnalogChannel(it->subdev, it->channel, it->range, it->aref);

				iolog3 << myname << "(iopoll): read AI "
					   << " sid=" << it->si.id
					   << " subdev=" << it->subdev
					   << " chan=" << it->channel
					   << " val=" << val
					   << endl;

				IOBase::processingAsAI( ib, val, shm, force );
			}
			else if( it->stype == UniversalIO::DI )
			{
				bool set = card->getDigitalChannel(it->subdev, it->channel);
				IOBase::processingAsDI( ib, set, shm, force );

				// немного оптимизации
				// сразу выставляем.сбрасываем флаг тестирования
				if( it->si.id == testLamp_s )
					isTestLamp = set;
			}
			else if( it->stype == UniversalIO::AO )
			{
				if( !it->lamp )
				{
					long rval = IOBase::processingAsAO( ib, shm, force_out );
					card->setAnalogChannel(it->subdev, it->channel, rval, it->range, it->aref);
				}
				else // управление лампочками
				{
					uniset_rwmutex_wrlock lock(it->val_lock);
					long prev_val = it->value;

					if( force_out )
						it->value = shm->localGetValue(it->ioit, it->si.id);

					switch( it->value )
					{
						case lmpOFF:
						{
							if( force_out && (prev_val == lmpBLINK
											  || prev_val == lmpBLINK2
											  || prev_val == lmpBLINK3) )
							{
								delBlink(it, lstBlink);
								delBlink(it, lstBlink2);
								delBlink(it, lstBlink3);
							}

							if( it->no_testlamp || !isTestLamp )
								card->setDigitalChannel(it->subdev, it->channel, 0);
						}
						break;

						case lmpON:
						{
							if( force_out && (prev_val == lmpBLINK
											  || prev_val == lmpBLINK2
											  || prev_val == lmpBLINK3) )
							{
								delBlink(it, lstBlink);
								delBlink(it, lstBlink2);
								delBlink(it, lstBlink3);
							}

							if( it->no_testlamp || !isTestLamp )
								card->setDigitalChannel(it->subdev, it->channel, 1);
						}
						break;

						case lmpBLINK:
						{
							if( force_out && prev_val != lmpBLINK )
							{
								delBlink(it, lstBlink2);
								delBlink(it, lstBlink3);
								addBlink(it, lstBlink);
								// и сразу зажигаем, чтобы не было паузы (так комфортнее выглядит для оператора)
								card->setDigitalChannel(it->subdev, it->channel, 1);
							}
						}
						break;

						case lmpBLINK2:
						{
							if( force_out && prev_val != lmpBLINK2 )
							{
								delBlink(it, lstBlink);
								delBlink(it, lstBlink3);
								addBlink(it, lstBlink2);
								// и сразу зажигаем, чтобы не было паузы (так комфортнее выглядит для оператора)
								card->setDigitalChannel(it->subdev, it->channel, 1);
							}
						}
						break;

						case lmpBLINK3:
						{
							if( force_out && prev_val != lmpBLINK3 )
							{
								delBlink(it, lstBlink);
								delBlink(it, lstBlink2);
								addBlink(it, lstBlink3);
								// и сразу зажигаем, чтобы не было паузы (так комфортнее выглядит для оператора)
								card->setDigitalChannel(it->subdev, it->channel, 1);
							}
						}
						break;

						default:
							return;
					}
				}
			}
			else if( it->stype == UniversalIO::DO )
			{
				bool set = IOBase::processingAsDO(ib, shm, force_out);

				if( !it->lamp || !isTestLamp )
					card->setDigitalChannel(it->subdev, it->channel, set);
			}
		}
		catch( const IOController_i::NameNotFound& ex )
		{
			iolog3 << myname << "(iopoll):(NameNotFound) " << ex.err << endl;
		}
		catch( const IOController_i::IOBadParam& ex )
		{
			iolog3 << myname << "(iopoll):(IOBadParam) " << ex.err << endl;
		}
		catch( const IONotifyController_i::BadRange& ex )
		{
			iolog3 << myname << "(iopoll): (BadRange)..." << endl;
		}
		catch( const uniset::Exception& ex )
		{
			iolog3 << myname << "(iopoll): " << ex << endl;
		}
		catch( const CORBA::SystemException& ex )
		{
			iolog3 << myname << "(iopoll): СORBA::SystemException: "
				   << ex.NP_minorString() << endl;
		}
	}
	// --------------------------------------------------------------------------------
	void IOControl::readConfiguration()
	{
		readconf_ok = false;

		xmlNode* root = uniset_conf()->getXMLSensorsSection();

		if(!root)
		{
			ostringstream err;
			err << myname << "(readConfiguration): section <sensors> not found";
			throw SystemError(err.str());
		}

		UniXML::iterator it(root);

		if( !it.goChildren() )
		{
			iowarn << myname << "(readConfiguration): section <sensors> empty?!!\n";
			return;
		}

		for( ; it.getCurrent(); it.goNext() )
		{
			if( uniset::check_filter(it, s_field, s_fvalue) )
				initIOItem(it);
		}

		readconf_ok = true;
	}
	// ------------------------------------------------------------------------------------------
	bool IOControl::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
	{
		if( uniset::check_filter(it, s_field, s_fvalue) )
			initIOItem(it);

		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool IOControl::initIOItem( UniXML::iterator& it )
	{
		IOInfo inf;

		string c(it.getProp("card"));

		inf.ncard = uni_atoi( c );

		if( c.empty() || inf.ncard < 0 || inf.ncard >= (int)cards.size() )
		{
			iolog3 << myname
				   << "(initIOItem): Unknown or bad card number ("
				   << inf.ncard << ") for " << it.getProp("name")
				   << " set default=" << defCardNum << endl;
			inf.ncard = defCardNum;
		}

		inf.subdev = it.getIntProp("subdev");

		if( inf.subdev < 0 )
			inf.subdev = DefaultSubdev;

		string jack = it.getProp("jack");

		if( !jack.empty() )
		{
			if( jack == "J1" )
				inf.subdev = 0;
			else if( jack == "J2" )
				inf.subdev = 1;
			else if( jack == "J3" )
				inf.subdev = 2;
			else if( jack == "J4" )
				inf.subdev = 3;
			else if( jack == "J5" )
				inf.subdev = 4;
			else
				inf.subdev = DefaultSubdev;
		}

		std::string prop_prefix( prefix + "_" );

		if( !IOBase::initItem(&inf, it, shm, prop_prefix, false, iolog, myname, filtersize, filterT) )
			return false;

		// если вектор уже заполнен
		// то увеличиваем его на 30 элементов (с запасом)
		// после инициализации делается resize
		// под реальное количество
		if( maxItem >= iomap.size() )
			iomap.resize(maxItem + 30);

		int prior = IOBase::initIntProp(it, "iopriority", prop_prefix, false);

		if( prior > 0 )
		{
			pmap.emplace_back(prior, maxItem);
			iolog3 << myname << "(readItem): add to priority list: " <<
				   it.getProp("name")
				   << " priority=" << prior << endl;
		}

		// значит это пороговый датчик..
		if( inf.t_ai != DefaultObjectId )
		{
			iomap[maxItem++] = std::move(inf);
			iolog3 << myname << "(readItem): add threshold '" << it.getProp("name")
				   << " for '" << uniset_conf()->oind->getNameById(inf.t_ai) << endl;
			return true;
		}

		inf.channel = IOBase::initIntProp(it, "channel", prop_prefix, false);

		if( inf.channel < 0 || inf.channel > 32 )
		{
			iowarn << myname << "(readItem): Unknown channel: " << inf.channel
				   << " for " << it.getProp("name") << endl;
			return false;
		}


		inf.lamp = IOBase::initIntProp(it, "lamp", prop_prefix, false);
		inf.no_testlamp = IOBase::initIntProp(it, "no_iotestlamp", prop_prefix, false);
		inf.enable_testmode = IOBase::initIntProp(it, "enable_testmode", prop_prefix, false);
		inf.disable_testmode = IOBase::initIntProp(it, "disable_testmode", prop_prefix, false);
		inf.aref = 0;
		inf.range = 0;

		if( inf.stype == UniversalIO::AI || inf.stype == UniversalIO::AO )
		{
			inf.range = IOBase::initIntProp(it, "range", prop_prefix, false);

			if( inf.range < 0 || inf.range > 3 )
			{
				iocrit << myname << "(readItem): Unknown 'range': " << inf.range
					   << " for " << it.getProp("name")
					   << " Must be range=[0..3]" << endl;
				return false;
			}

			inf.aref = IOBase::initIntProp(it, "aref", prop_prefix, false);

			if( inf.aref < 0 || inf.aref > 3 )
			{
				iocrit << myname << "(readItem): Unknown 'aref': " << inf.aref
					   << " for " << it.getProp("name")
					   << ". Must be aref=[0..3]" << endl;
				return false;
			}
		}

		iolog3 << myname << "(readItem): add: " << inf.stype << " " << inf << endl;

		iomap[maxItem++] = std::move(inf);
		return true;
	}
	// ------------------------------------------------------------------------------------------

	bool IOControl::activateObject()
	{
		// блокирование обработки Startup
		// пока не пройдёт инициализация датчиков
		// см. sysCommand()
		{
			activated = false;
			UniSetObject::activateObject();
			activated = true;
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	void IOControl::sigterm( int signo )
	{
		term = true;

		if( noCards )
			return;

		// выставляем безопасные состояния
		for( auto& it : iomap )
		{
			if( it.ignore )
				continue;

			ComediInterface* card = cards.getCard(it.ncard);

			if( card == NULL )
				continue;

			try
			{
				if( it.subdev == DefaultSubdev || it.safety == NoSafety )
					continue;

				if( it.stype == UniversalIO::DO || it.lamp )
				{
					bool set = it.invert ? !((bool)it.safety) : (bool)it.safety;
					card->setDigitalChannel(it.subdev, it.channel, set);
				}
				else if( it.stype == UniversalIO::AO )
				{
					card->setAnalogChannel(it.subdev, it.channel, it.safety, it.range, it.aref);
				}
			}
			catch( const std::exception& ex )
			{
				iolog3 << myname << "(sigterm): " << ex.what() << endl;
			}
		}

		while( term ) {}
	}
	// -----------------------------------------------------------------------------
	void IOControl::initOutputs()
	{
		if( noCards )
			return;

		// выставляем значение по умолчанию
		for( auto& it : iomap )
		{
			if( it.ignore )
				continue;

			ComediInterface* card = cards.getCard(it.ncard);

			if( card == NULL || it.subdev == DefaultSubdev || it.channel == DefaultChannel )
				continue;

			try
			{
				if( it.lamp )
					card->setDigitalChannel(it.subdev, it.channel, (bool)it.defval);
				else if( it.stype == UniversalIO::DO )
					card->setDigitalChannel(it.subdev, it.channel, (bool)it.defval);
				else if( it.stype == UniversalIO::AO )
					card->setAnalogChannel(it.subdev, it.channel, it.defval, it.range, it.aref);
			}
			catch( const uniset::Exception& ex )
			{
				iolog3 << myname << "(initOutput): " << ex << endl;
			}
		}
	}
	// ------------------------------------------------------------------------------
	void IOControl::initIOCard()
	{
		if( noCards )
			return;

		for( auto& it : iomap )
		{
			if( it.subdev == DefaultSubdev )
				continue;

			ComediInterface* card = cards.getCard(it.ncard);

			if( card == NULL || it.subdev == DefaultSubdev || it.channel == DefaultChannel )
				continue;

			try
			{
				// конфигурировать необходимо только дискретные входы/выходы
				// или "лампочки" (т.к. они фиктивные аналоговые датчики)
				if( it.lamp )
					card->configureChannel(it.subdev, it.channel, ComediInterface::DO);
				else if( it.stype == UniversalIO::DI )
					card->configureChannel(it.subdev, it.channel, ComediInterface::DI);
				else if( it.stype == UniversalIO::DO )
					card->configureChannel(it.subdev, it.channel, ComediInterface::DO);
				else if( it.stype == UniversalIO::AI )
				{
					card->configureChannel(it.subdev, it.channel, ComediInterface::AI);
					it.df.init( card->getAnalogChannel(it.subdev, it.channel, it.range, it.aref) );
				}
				else if( it.stype == UniversalIO::AO )
					card->configureChannel(it.subdev, it.channel, ComediInterface::AO);

			}
			catch( const uniset::Exception& ex)
			{
				iocrit << myname << "(initIOCard): sid=" << it.si.id << " " << ex << endl;
			}
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::blink( BlinkList& lst, bool& bstate )
	{
		if( lst.empty() )
			return;

		for( auto& io : lst )
		{
			if( io->subdev == DefaultSubdev || io->channel == DefaultChannel )
				continue;

			ComediInterface* card = cards.getCard(io->ncard);

			if( card == NULL )
				continue;

			try
			{
				card->setDigitalChannel(io->subdev, io->channel, bstate);
			}
			catch( const uniset::Exception& ex )
			{
				iocrit << myname << "(blink): " << ex << endl;
			}
		}

		bstate ^= true;
	}
	// -----------------------------------------------------------------------------

	void IOControl::addBlink( IOInfo* io, BlinkList& lst )
	{
		for( auto& it : lst )
		{
			if( it == io )
				return;
		}

		lst.push_back(io);
	}
	// -----------------------------------------------------------------------------
	void IOControl::delBlink( IOInfo* io, BlinkList& lst )
	{
		for( auto it = lst.begin(); it != lst.end(); ++it )
		{
			if( (*it) == io )
			{
				lst.erase(it);
				return;
			}
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::check_testmode()
	{
		if( testMode_as == DefaultObjectId )
			return;

		try
		{
			if( force_out )
				testmode = shm->localGetValue( itTestMode, testMode_as );

			if( prev_testmode == testmode )
				return;

			prev_testmode = testmode;

			// если режим "выключено всё"
			// то гасим все выходы
			if( testmode == tmOffPoll ||
					testmode == tmConfigEnable ||
					testmode == tmConfigDisable )
			{
				// выставляем безопасные состояния
				for( auto& it : iomap )
				{
					if( it.ignore )
						continue;

					ComediInterface* card = cards.getCard(it.ncard);

					if( card == NULL )
						continue;

					if( testmode == tmConfigEnable && !it.enable_testmode )
						return;

					if( testmode == tmConfigDisable && it.disable_testmode )
						return;

					try
					{
						if( it.subdev == DefaultSubdev || it.safety == NoSafety )
							continue;

						if( it.stype == UniversalIO::DO || it.lamp )
						{
							bool set = it.invert ? !((bool)it.safety) : (bool)it.safety;
							card->setDigitalChannel(it.subdev, it.channel, set);
						}
						else if( it.stype == UniversalIO::AO )
						{
							card->setAnalogChannel(it.subdev, it.channel, it.safety, it.range, it.aref);
						}
					}
					catch( const uniset::Exception& ex )
					{
						iolog3 << myname << "(sigterm): " << ex << endl;
					}
					catch(...) {}
				}
			}

		}
		catch( const uniset::Exception& ex)
		{
			iocrit << myname << "(check_testmode): " << ex << endl;
		}
		catch(...)
		{
			iocrit << myname << "(check_testmode): catch ..." << endl;
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::check_testlamp()
	{
		if( testLamp_s == DefaultObjectId )
			return;

		try
		{
			if( force_out )
				isTestLamp = shm->localGetValue( itTestLamp, testLamp_s );

			if( !trTestLamp.change(isTestLamp) )
				return; // если состояние не менялось, то продолжаем работу...

			if( isTestLamp )
				blink_state = true; // первый такт всегда зажигаем...

			//        cout << myname << "(check_test_lamp): ************* test lamp "
			//            << isTestLamp << " *************" << endl;

			// проходим по списку и формируем список мигающих выходов...
			for( auto& it : iomap )
			{
				if( !it.lamp || it.no_testlamp )
					continue;

				if(  it.stype == UniversalIO::AO )
				{
					if( isTestLamp )
					{
						addBlink( &it, lstBlink);
						delBlink( &it, lstBlink2);
						delBlink( &it, lstBlink3);
					}
					else if( it.value == lmpBLINK )
						addBlink( &it, lstBlink);
					else if( it.value == lmpBLINK2 )
						addBlink( &it, lstBlink2);
					else if( it.value == lmpBLINK3 )
						addBlink( &it, lstBlink3);
					else
					{
						delBlink(&it, lstBlink);
						delBlink(&it, lstBlink2);
						delBlink(&it, lstBlink3);
					}
				}
				else if( it.stype == UniversalIO::DO )
				{
					if( isTestLamp )
						addBlink(&it, lstBlink);
					else
						delBlink(&it, lstBlink);
				}
			}
		}
		catch( const uniset::Exception& ex)
		{
			iocrit << myname << "(check_testlamp): " << ex << endl;
		}
		catch( const std::exception& ex )
		{
			iocrit << myname << "(check_testlamp): catch ..." << endl;
		}
	}

	// -----------------------------------------------------------------------------
	std::shared_ptr<IOControl> IOControl::init_iocontrol(int argc, const char* const* argv,
			uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
			const std::string& prefix )
	{
		auto conf = uniset_conf();
		string name = conf->getArgParam("--" + prefix + "-name", "IOControl1");

		if( name.empty() )
		{
			cerr << "(iocontrol): Unknown name. Use --" << prefix << "-name " << endl;
			return 0;
		}

		ObjectId ID = conf->getObjectID(name);

		if( ID == uniset::DefaultObjectId )
		{
			cerr << "(iocontrol): Unknown ID for " << name
				 << "' Not found in <objects>" << endl;
			return 0;
		}

		int numcards = conf->getArgPInt("--" + prefix + "-numcards", 1);

		dinfo << "(iocontrol): name = " << name << "(" << ID << ")" << endl;
		return make_shared<IOControl>(ID, icID, ic, numcards, prefix);
	}
	// -----------------------------------------------------------------------------
	void IOControl::help_print( int argc, const char* const* argv )
	{
		cout << "--prefix-confnode name   - Использовать для настройки указанный xml-узел" << endl;
		cout << "--prefix-name name       - ID процесса. По умолчанию IOController1." << endl;
		cout << "--prefix-numcards        - Количество кард в/в. По умолчанию 1." << endl;
		cout << "--prefix-devX dev        - Использовать для card='X' указанный файл comedi-устройства." << endl;
		cout << "                           'X'  не должен быть больше --prefix-numcards" << endl;
		cout << "--prefix-devX-subdevX-type name  - Настройка типа подустройства для UNIO." << endl ;
		cout << "                                   Разрешены: TBI0_24,TBI24_0,TBI16_8" << endl;

		cout << "--prefix-default_cardnum   - Номер карты по умолчанию. По умолчанию -1." << endl;
		cout << "                             Если задать, то он будет использоватся для датчиков" << endl;
		cout << "                             у которых не задано поле 'card'." << endl;

		cout << "--prefix-test-lamp         - Для данного узла в качестве датчика кнопки 'ТестЛамп' использовать указанный датчик." << endl;
		cout << "--prefix-conf-field fname  - Считывать из конф. файла все датчики с полем fname='1'" << endl;
		cout << "--prefix-polltime msec     - Пауза между опросом карт. По умолчанию 200 мсек." << endl;
		cout << "--prefix-filtersize val    - Размерность фильтра для аналоговых входов." << endl;
		cout << "--prefix-filterT val       - Постоянная времени фильтра." << endl;
		cout << "--prefix-s-filter-field    - Идентификатор в configure.xml по которому считывается список относящихся к это процессу датчиков" << endl;
		cout << "--prefix-s-filter-value    - Значение идентификатора по которому считывается список относящихся к это процессу датчиков" << endl;
		cout << "--prefix-blink-time msec   - Частота мигания, мсек. По умолчанию в configure.xml" << endl;
		cout << "--prefix-blink2-time msec  - Вторая частота мигания (lmpBLINK2), мсек. По умолчанию в configure.xml" << endl;
		cout << "--prefix-blink3-time msec  - Вторая частота мигания (lmpBLINK3), мсек. По умолчанию в configure.xml" << endl;
		cout << "--prefix-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
		cout << "--prefix-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
		cout << "--prefix-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
		cout << "--prefix-force             - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
		cout << "--prefix-force-out         - Обновлять выходы принудительно (не по заказу)" << endl;
		cout << "--prefix-skip-init-output  - Не инициализировать 'выходы' при старте" << endl;
		cout << "--prefix-sm-ready-test-sid - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;
		cout << endl;
		cout << " Logs: " << endl;
		cout << "--prefix-log-...            - log control" << endl;
		cout << "             add-levels ..." << endl;
		cout << "             del-levels ..." << endl;
		cout << "             set-levels ..." << endl;
		cout << "             logfile filaname" << endl;
		cout << "             no-debug " << endl;
		cout << " LogServer: " << endl;
		cout << "--prefix-run-logserver       - run logserver. Default: localhost:id" << endl;
		cout << "--prefix-logserver-host ip   - listen ip. Default: localhost" << endl;
		cout << "--prefix-logserver-port num  - listen port. Default: ID" << endl;
		cout << LogServer::help_print("prefix-logserver") << endl;
	}
	// -----------------------------------------------------------------------------
	void IOControl::sysCommand( const SystemMessage* sm )
	{
		switch( sm->command )
		{
			case SystemMessage::StartUp:
			{
				if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
				{
					ioinfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
					logserv->async_run(logserv_host, logserv_port);
				}

				PassiveTimer ptAct(activateTimeout);

				while( !activated && !ptAct.checkTime() )
				{
					ioinfo << myname << "(sysCommand): wait activate..." << endl;
					msleep(300);

					if( activated )
						break;
				}

				if( !activated )
					iocrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl;

				askSensors(UniversalIO::UIONotify);
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
				// (т.е. IOControl  запущен в одном процессе с SharedMemory2)
				// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
				// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(IOControl)
				if( shm->isLocalwork() )
					break;

				askSensors(UniversalIO::UIONotify);

				// принудительно обновляем состояния...
				try
				{
					if( !force )
					{
						force = true;
						std::lock_guard<std::mutex> l(iopollMutex);
						iopoll();
						force = false;
					}
				}
				catch(...) {}
			}
			break;

			case SystemMessage::LogRotate:
			{
				iologany << myname << "(sysCommand): logRotate" << endl;
				string fname = iolog->getLogFile();

				if( !fname.empty() )
				{
					iolog->logFile(fname, true);
					iologany << myname << "(sysCommand): ***************** GGDEB LOG ROTATE *****************" << endl;
				}
			}
			break;

			default:
				break;
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::askSensors( UniversalIO::UIOCommand cmd )
	{
		if( force_out )
			return;

		waitSM();

		if( sidTestSMReady != DefaultObjectId &&
				!shm->waitSMworking(sidTestSMReady , activateTimeout, 50) )
		{
			ostringstream err;
			err << myname
				<< "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение "
				<< activateTimeout << " мсек";

			iocrit << err.str() << endl;
			kill(SIGTERM, getpid());   // прерываем (перезапускаем) процесс...
			throw SystemError(err.str());
		}

		PassiveTimer ptAct(activateTimeout);

		while( !readconf_ok && !ptAct.checkTime() )
		{
			ioinfo << myname << "(askSensors): wait read configuration..." << endl;
			msleep(50);

			if( readconf_ok )
				break;
		}

		if( !readconf_ok )
			iocrit << myname << "(askSensors): ************* don`t read configuration?! ************" << endl;

		try
		{
			if( testLamp_s != DefaultObjectId )
				shm->askSensor(testLamp_s, cmd);
		}
		catch( const uniset::Exception& ex)
		{
			iocrit << myname << "(askSensors): " << ex << endl;
		}

		try
		{
			if( testMode_as != DefaultObjectId )
				shm->askSensor(testMode_as, cmd);
		}
		catch( const uniset::Exception& ex )
		{
			iocrit << myname << "(askSensors): " << ex << endl;
		}

		for( auto& it : iomap )
		{
			if( it.ignore )
				continue;

			ComediInterface* card = cards.getCard(it.ncard);

			if( card == NULL || it.subdev == DefaultSubdev || it.channel == DefaultChannel )
				continue;

			if( it.stype == UniversalIO::AO || it.stype == UniversalIO::DO )
			{
				try
				{
					shm->askSensor(it.si.id, cmd, myid);
				}
				catch( const uniset::Exception& ex )
				{
					iocrit << myname << "(askSensors): " << ex << endl;
				}
			}
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::sensorInfo( const uniset::SensorMessage* sm )
	{
		iolog1 << myname << "(sensorInfo): sm->id=" << sm->id
			   << " val=" << sm->value << endl;

		if( force_out )
			return;

		if( sm->id == testLamp_s )
		{
			ioinfo << myname << "(sensorInfo): test_lamp=" << sm->value << endl;
			isTestLamp = (bool)sm->value;
		}
		else if( sm->id == testMode_as )
		{
			testmode = sm->value;
			check_testmode();
		}

		for( auto& it : iomap )
		{
			if( it.si.id == sm->id )
			{
				ioinfo << myname << "(sensorInfo): sid=" << sm->id
					   << " value=" << sm->value
					   << endl;

				if( it.stype == UniversalIO::AO )
				{
					long prev_val = 0;
					long cur_val = 0;
					{
						uniset_rwmutex_wrlock lock(it.val_lock);
						prev_val = it.value;
						it.value = sm->value;
						cur_val = sm->value;
					}

					if( it.lamp )
					{
						switch( cur_val )
						{
							case lmpOFF:
								delBlink(&it, lstBlink);
								delBlink(&it, lstBlink2);
								delBlink(&it, lstBlink3);
								break;

							case lmpON:
								delBlink(&it, lstBlink);
								delBlink(&it, lstBlink2);
								delBlink(&it, lstBlink3);
								break;


							case lmpBLINK:
							{
								if( prev_val != lmpBLINK )
								{
									delBlink(&it, lstBlink2);
									delBlink(&it, lstBlink3);
									addBlink(&it, lstBlink);

									// и сразу зажигаем, чтобы не было паузы
									// (так комфортнее выглядит для оператора)
									if( it.ignore || it.subdev == DefaultSubdev || it.channel == DefaultChannel )
										break;

									ComediInterface* card = cards.getCard(it.ncard);

									if( card != NULL )
										card->setDigitalChannel(it.subdev, it.channel, 1);
								}
							}
							break;

							case lmpBLINK2:
							{
								if( prev_val != lmpBLINK2 )
								{
									delBlink(&it, lstBlink);
									delBlink(&it, lstBlink3);
									addBlink(&it, lstBlink2);

									// и сразу зажигаем, чтобы не было паузы
									// (так комфортнее выглядит для оператора)
									if( it.ignore || it.subdev == DefaultSubdev || it.channel == DefaultChannel )
										break;

									ComediInterface* card = cards.getCard(it.ncard);

									if( card != NULL )
										card->setDigitalChannel(it.subdev, it.channel, 1);
								}
							}
							break;

							case lmpBLINK3:
							{
								if( prev_val != lmpBLINK3 )
								{
									delBlink(&it, lstBlink);
									delBlink(&it, lstBlink2);
									addBlink(&it, lstBlink3);

									// и сразу зажигаем, чтобы не было паузы
									// (так комфортнее выглядит для оператора)
									if( it.ignore || it.subdev == DefaultSubdev || it.channel == DefaultChannel )
										break;

									ComediInterface* card = cards.getCard(it.ncard);

									if( card != NULL )
										card->setDigitalChannel(it.subdev, it.channel, 1);
								}
							}
							break;

							default:
								break;
						}
					}
				}
				else if( it.stype == UniversalIO::DO )
				{
					iolog1 << myname << "(sensorInfo): DO: sm->id=" << sm->id
						   << " val=" << sm->value << endl;

					uniset_rwmutex_wrlock lock(it.val_lock);
					it.value = sm->value ? 1 : 0;
				}

				break;
			}
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::timerInfo( const uniset::TimerMessage* tm )
	{

	}
	// -----------------------------------------------------------------------------
	void IOControl::waitSM()
	{
		if( !shm->waitSMready(smReadyTimeout, 50) )
		{
			ostringstream err;
			err << myname << "(execute): did not wait for the ready 'SharedMemory'. Timeout "
				<< smReadyTimeout << " msec";

			iocrit << err.str() << endl;
			//throw SystemError(err.str());
			std::terminate();
		}
	}
	// -----------------------------------------------------------------------------
	void IOControl::buildCardsList()
	{
		auto conf = uniset_conf();

		xmlNode* nnode = conf->getXMLNodesSection();

		if( !nnode )
		{
			iowarn << myname << "(buildCardsList): <nodes> not found?!" << endl;
			return;
		}

		auto xml = conf->getConfXML();

		if( !xml )
		{
			iowarn << myname << "(buildCardsList): xml=NULL?!" << endl;
			return;
		}

		xmlNode* mynode = 0;
		UniXML::iterator it1(nnode);

		if( it1.goChildren() )
		{
			for( ; it1.getCurrent(); it1.goNext() )
			{
				if( it1.getProp("name") == conf->getLocalNodeName() )
				{
					mynode = it1.getCurrent();
					break;
				}
			}
		}

		if( !mynode )
		{
			iowarn << myname << "(buildCardsList): node='" << conf->getLocalNodeName() << "' not found.." << endl;
			return;
		}

		xmlNode* cardsnode = xml->extFindNode(mynode, 1, 1, "iocards", "");

		if( !cardsnode )
		{
			iowarn << myname << "(buildCardsList): Not found <iocards> for node=" << conf->getLocalNodeName() << "(" << conf->getLocalNode() << ")" << endl;
			return;
		}

		UniXML::iterator it(cardsnode);

		if( !it.goChildren() )
		{
			iowarn << myname << "(buildCardsList): <iocards> empty.." << endl;
			return;
		}

		for( size_t i = 0; i < cards.size(); i++ )
		{
			if( cards[i] == 0 )
				break;
		}

		for( ; it.getCurrent(); it.goNext() )
		{
			std::string cname(it.getProp("name"));

			int cardnum = it.getIntProp("card");

			if( cardnum <= 0 )
			{
				iolog3 << myname << "(init): Unknown card number?!  card=" << it.getIntProp("card") << "(" << cname << ")" << endl;
				continue;

			}

			if( cardnum > maxCardNum )
			{
				iolog3 << myname << "(init): BAD card number card='" << it.getIntProp("card") << "'(" << cname << "). Must be < " << maxCardNum << endl;
				continue;
			}

			if( it.getIntProp("ignore") )
			{
				cards[cardnum] = NULL;
				iolog3 << myname << "(init): card=" << it.getProp("card") << "(" << cname << ")"
					   << " DISABLED! ignore=1" << endl;
				continue;
			}

			stringstream s;
			s << "--" << prefix << "-card" << cardnum << "-ignore";

			if( findArgParam( s.str(), conf->getArgc(), conf->getArgv()) != -1 )
			{
				cards[cardnum] = NULL;
				iolog3 << myname << "(init): card=" << it.getProp("card") << "(" << cname << ")"
					   << " DISABLED! (" << s.str() << ")" << endl;
				continue;
			}

			std::string iodev(it.getProp("dev"));

			if( iodev.empty() || iodev == "/dev/null" )
			{
				cards[cardnum] = NULL;
				iolog3 << myname << "(init): card=" << it.getProp("card") << "(" << cname << ")"
					   << " DISABLED! iodev='"
					   << iodev << "'" << endl;
				continue;
			}

			iolog3 << myname << "(init): ADD card=" << it.getProp("card") << "(" << cname << ")"  << " dev=" << iodev << endl;

			try
			{
				cards[cardnum] = new ComediInterface(iodev);
				noCards = false;
			}
			catch( const uniset::Exception& ex )
			{
				iocrit << myname << "(buildCardsList): " << ex << endl;
				throw;
			}

			if( cname == "DI32" )
			{
			}
			else if( cname == "AI16-5A-3" || cname == "AIC123xx" )
			{
			}
			else if( cname == "AO16-xx" )
			{
			}
			else if( cname == "UNIO48" || cname == "UNIO96" )
			{
				unsigned int k = 4;

				if( cname == "UNIO48" )
					k = 2;

				// инициализация subdev-ов
				for( unsigned int i = 1; i <= k; i++ )
				{
					ostringstream s;
					s << "subdev" << i;

					string subdev_name( it.getProp(s.str()) );

					if( subdev_name.empty() )
					{
						ioinfo << myname << "(buidCardList): empty subdev. ignore... (" << s.str() << ")" << endl;
						continue;
					}

					ComediInterface::SubdevType st = ComediInterface::str2type(subdev_name);

					if( st == ComediInterface::Unknown )
					{
						ostringstream err;
						err << "(buildCardsList): card=" << it.getProp("card") << "(" << cname
							<< " Unknown subdev(" << s.str() << ") type '" << it.getProp(s.str()) << "' for " << cname;
						throw SystemError(err.str());
					}

					if( st == ComediInterface::GRAYHILL )
					{
						// для Grayhill конфигурирование не требуется
						ioinfo << myname << "(buildCardsList): card=" << it.getProp("card")
							   << "(" << cname << ")"
							   << " init subdev" << i << " 'GRAYHILL'" << endl;
						continue;
					}

					ioinfo << myname << "(buildCardsList): card=" << it.getProp("card")
						   << "(" << cname << ")"
						   << " init subdev" << i << " " << it.getProp(s.str()) << endl;
					cards[cardnum]->configureSubdev(i - 1, st);
				}
			}
		}
	}
	// -----------------------------------------------------------------------------
} // end of namespace uniset
