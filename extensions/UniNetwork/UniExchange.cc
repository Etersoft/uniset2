// $Id: NetExchange.cc,v 1.2 2009/04/07 16:11:23 pv Exp $
// --------------------------------------------------------------------------
#include "Configuration.h"
#include "Extensions.h"
#include "Exceptions.h"
#include "UniExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
UniExchange::NetNodeInfo::NetNodeInfo():
	oref(CORBA::Object::_nil()),
	id(DefaultObjectId),
	node(conf->getLocalNode()),
	sidConnection(DefaultObjectId),
	smap(1)
{

}
// --------------------------------------------------------------------------
UniExchange::UniExchange( UniSetTypes::ObjectId id, UniSetTypes::ObjectId shmID, 
							SharedMemory* ic, const std::string prefix ):
IOController(id),
shm(0),
polltime(200),
mymap(1),
maxIndex(0),
smReadyTimeout(15000)
{
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(UniExchange): Not find conf-node for " + myname );

	shm = new SMInterface(shmID,&ui,id,ic);

	UniXML_iterator it(cnode);

	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	polltime = conf->getArgInt("--" + prefix + "-polltime",it.getProp("polltime"));
	if( polltime <= 0 )
		polltime = 200;
	dlog[Debug::INFO] << myname << "(init): polltime=" << polltime << endl;

	int updatetime = conf->getArgInt("--" + prefix + "-updatetime",it.getProp("updatetime"));
	if( updatetime <= 0 )
		updatetime = 200;
	dlog[Debug::INFO] << myname << "(init): updatetime=" << polltime << endl;

	ptUpdate.setTiming(updatetime);

	smReadyTimeout = conf->getArgInt("--io-sm-ready-timeout",it.getProp("ready_timeout"));
	if( smReadyTimeout == 0 )
		smReadyTimeout = 15000;
	else if( smReadyTimeout < 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;

	dlog[Debug::INFO] << myname << "(init): smReadyTimeout=" << smReadyTimeout << endl;

	if( it.goChildren() )
	{
		for( ; it.getCurrent(); it.goNext() )
		{
			UniSetTypes::ObjectId id;
		
			string n(it.getProp("id"));
			if( !n.empty() )
				id = it.getIntProp("id");
			else
			{
				id = conf->getControllerID( it.getProp("name") );
				n = it.getProp("name");
			}
				
			if( id == DefaultObjectId )
				throw SystemError("(UniExchange): Uknown ID for " + n );

			UniSetTypes::ObjectId node;

			string n1(it.getProp("node_id"));
			if( !n1.empty() )
				node = it.getIntProp("node_id");
			else
			{
				n1 = it.getProp("node");
				node = conf->getNodeID(n1);
			}

			if( id == DefaultObjectId )
				throw SystemError("(UniExchange): Uknown ID for node=" + n1 );
			
			NetNodeInfo ni;
			ni.oref = CORBA::Object::_nil();
			ni.id   = id;
			ni.node = node;
			ni.sidConnection = conf->getSensorID(it.getProp("sid_connection"));

			dlog[Debug::INFO] << myname << ": add point " << n << ":" << n1 << endl;
			nlst.push_back(ni);
		}
	}
	
	if( shm->isLocalwork() )
	{
		readConfiguration();
		mymap.resize(maxIndex);
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&UniExchange::readItem) );
}

// -----------------------------------------------------------------------------

UniExchange::~UniExchange()
{
	delete shm;
}

// -----------------------------------------------------------------------------
void UniExchange::execute()
{
	if( !shm->waitSMready(smReadyTimeout,50) )
	{
		ostringstream err;
		err << myname << "(execute): Не дождались готовности SharedMemory к работе в течение "
					<< smReadyTimeout << " мсек";

		unideb[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}

	PassiveTimer pt(UniSetTimer::WaitUpTime);
	if( shm->isLocalwork() )
	{
		maxIndex = 0;
		readConfiguration();
		cerr << "************************** readConfiguration: " << pt.getCurrent() << " msec " << endl;
	}

	mymap.resize(maxIndex);
	initIterators();
	init_ok = true;

	while(1)
	{
		for( NetNodeList::iterator it=nlst.begin(); it!=nlst.end(); ++it )
		{
			bool ok = false;
			try
			{
				if( dlog.debugging(Debug::INFO) )
					dlog[Debug::INFO] << myname << ": connect to id=" << it->id << " node=" << it->node << endl;

				IOController_i::ShortMapSeq_var sseq = ui.getSensors( it->id, it->node );
				ok = true;

				if( dlog.debugging(Debug::INFO) )
					dlog[Debug::INFO] << myname << " update sensors from id=" << it->id << " node=" << it->node << endl;
				it->update(sseq,shm);
			}
			catch( Exception& ex )
			{
				dlog[Debug::INFO]  << myname << "(execute): " << ex << endl;
			}
			catch( ... )
			{
				dlog[Debug::INFO]  << myname << "(execute): catch ..." << endl;
			}
	
			if( it->sidConnection != DefaultObjectId )
			{
				try
				{
					shm->localSaveState(it->conn_dit,it->sidConnection,ok,getId());
				}
				catch(...)
				{
					dlog[Debug::CRIT]<< myname << "(execute): sensor not avalible "
							<< conf->oind->getNameById( it->sidConnection) 
							<< endl; 
				}
			}

			if( !ok && dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << ": ****** cannot connect with node=" << it->node << endl;
		}

		if( ptUpdate.checkTime() )
		{
			updateLocalData();
			ptUpdate.reset();
		}
	
		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
void UniExchange::NetNodeInfo::update( IOController_i::ShortMapSeq_var& map, SMInterface* shm  )
{
	bool reinit = false;
	if( smap.size() != map->length() )
	{
		reinit = true;
		// init new map...
		smap.resize(map->length());
	}
	
	int size = map->length();
	for( int i=0; i<size; i++ )
	{
		SInfo* s = &(smap[i]);
		IOController_i::ShortMap* m = &(map[i]);
		if( reinit )
		{
			shm->initDIterator(s->dit);
			shm->initAIterator(s->ait);
			s->type 	= m->type;
			s->id 		= m->id;
		}

		s->val = m->value;
		
		try
		{
			if( m->type == UniversalIO::DigitalInput )
				shm->localSaveState( s->dit, m->id, (m->value ? true : false ), shm->ID() );
			else if( m->type == UniversalIO::DigitalOutput )
				shm->localSetState( s->dit, m->id, (m->value ? true : false ), shm->ID() );
			else if( map[i].type == UniversalIO::AnalogInput )
				shm->localSaveValue( s->ait, m->id, m->value, shm->ID() );
			else if( map[i].type == UniversalIO::AnalogOutput )
				shm->localSetValue( s->ait, m->id, m->value, shm->ID() );
		}
		catch( Exception& ex )
		{
			dlog[Debug::INFO]  << "(update): " << ex << endl;
		}
		catch( ... )
		{
			dlog[Debug::INFO]  << "(update): catch ..." << endl;
		}
	}
}
// --------------------------------------------------------------------------
IOController_i::ShortMapSeq* UniExchange::getSensors()
{
	if( !init_ok )
		throw CORBA::COMM_FAILURE();

	IOController_i::ShortMapSeq* res = new IOController_i::ShortMapSeq();
	res->length( mymap.size() );

	int i=0;
	for( SList::iterator it=mymap.begin(); it!=mymap.end(); ++it )
	{
		IOController_i::ShortMap m;
		{
			uniset_spin_lock lock(it->val_lock,30);
			m.id 	= it->id;
			m.value = it->val;
			m.type = it->type;
		}
		(*res)[i++] = m;
	}

	return res;
}
// --------------------------------------------------------------------------
void UniExchange::updateLocalData()
{
	for( SList::iterator it=mymap.begin(); it!=mymap.end(); ++it )
	{
		try
		{
			uniset_spin_lock lock(it->val_lock,30);
			if( it->type == UniversalIO::DigitalInput ||
				it->type == UniversalIO::DigitalOutput )
			{
				it->val = shm->localGetState( it->dit, it->id );
			}
			else if( it->type == UniversalIO::AnalogInput ||
					it->type == UniversalIO::AnalogOutput )
			{
				it->val = shm->localGetValue( it->ait, it->id );
			}
		}
		catch( Exception& ex )
		{
			dlog[Debug::INFO]  << "(update): " << ex << endl;
		}
		catch( ... )
		{
			dlog[Debug::INFO]  << "(update): catch ..." << endl;
		}
	}
	
	init_ok = true;
}
// --------------------------------------------------------------------------
void UniExchange::initIterators()
{
	for( SList::iterator it=mymap.begin(); it!=mymap.end(); ++it )
	{
		shm->initDIterator(it->dit);
		shm->initAIterator(it->ait);
	}
}
// --------------------------------------------------------------------------
void UniExchange::askSensors( UniversalIO::UIOCommand cmd )
{

}
// -----------------------------------------------------------------------------
void UniExchange::processingMessage( UniSetTypes::VoidMessage* msg )
{
	try
	{
		switch( msg->type )
		{
			case Message::SensorInfo:
			{
				SensorMessage sm( msg );
				sensorInfo( &sm );
				break;
			}

			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
				break;
			}

			case Message::SysCommand:
			{
				SystemMessage sm( msg );
				sysCommand( &sm );
				break;
			}

			default:
				break;
		}
	}
	catch(Exception& ex)
	{
		cout  << myname << "(processingMessage): " << ex << endl;
	}
}
// -----------------------------------------------------------------------------
void UniExchange::sysCommand( SystemMessage* sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			askSensors(UniversalIO::UIONotify);
			break;
		}
		
		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::WatchDog:
			askSensors(UniversalIO::UIONotify);
			break;

		default:
			break;
	}
}
// -----------------------------------------------------------------------------
void UniExchange::sigterm( int signo )
{
}
// -----------------------------------------------------------------------------
void UniExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{

}
// -----------------------------------------------------------------------------
void UniExchange::timerInfo( UniSetTypes::TimerMessage* tm )
{
	
}
// -----------------------------------------------------------------------------
UniExchange* UniExchange::init_exchange( int argc, const char* const* argv, 
										UniSetTypes::ObjectId icID, SharedMemory* ic, 
											const std::string prefix )
{
	string p("--" + prefix + "-name");
	string nm(UniSetTypes::getArgParam(p,argc,argv,"UniExchange"));

	UniSetTypes::ObjectId ID = conf->getControllerID(nm);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(uniexchange): Not found ID  for " << nm
			<< " in section " << conf->getControllersSection() << endl;
		return 0;
	}
	return new UniExchange(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
void UniExchange::readConfiguration()
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
bool UniExchange::check_item( UniXML_iterator& it )
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

bool UniExchange::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------
bool UniExchange::initItem( UniXML_iterator& it )
{
	SInfo i;

	i.id = DefaultObjectId;
	if( it.getProp("id").empty() )
		i.id = conf->getSensorID(it.getProp("name"));
	else
	{
		i.id = it.getIntProp("id");
		if( i.id <=0 )
			i.id = DefaultObjectId;
	}
	
	if( i.id == DefaultObjectId )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(initItem): Unknown ID for " 
				<< it.getProp("name") << endl;
		return false;
	}

	i.type = UniSetTypes::getIOType(it.getProp("iotype"));
	if( i.type == UniversalIO::UnknownIOType )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(initItem): Unknown iotype= " 
				<< it.getProp("iotype") << " for " << it.getProp("name") << endl;
		return false;
	}

	i.val = 0;

	mymap[maxIndex++] = i;
	if( maxIndex >= mymap.size() )
		mymap.resize(maxIndex+10);

	return true;
}
// ------------------------------------------------------------------------------------------
void UniExchange::help_print( int argc, const char** argv )
{
	cout << "--unet-polltime msec     - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
//	cout << "--unet-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
//	cout << "--unet-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--unet-sm-ready-timeout - время на ожидание старта SM" << endl;
}
// -----------------------------------------------------------------------------
