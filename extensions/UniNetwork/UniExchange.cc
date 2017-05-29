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
#include "Configuration.h"
#include "Extensions.h"
#include "Exceptions.h"
#include "UniExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
UniExchange::NetNodeInfo::NetNodeInfo():
	oref(CORBA::Object::_nil()),
	id(DefaultObjectId),
	node(uniset_conf()->getLocalNode()),
	sidConnection(DefaultObjectId),
	smap(1)
{

}
// --------------------------------------------------------------------------
UniExchange::UniExchange(uniset::ObjectId id, uniset::ObjectId shmID,
						 const std::shared_ptr<SharedMemory>& ic, const std::string& prefix ):
	IOController(id),
	polltime(200),
	mymap(1),
	maxIndex(0),
	smReadyTimeout(15000)
{
	auto conf = uniset_conf();

	cnode = conf->getNode(myname);

	if( cnode == NULL )
		throw uniset::SystemError("(UniExchange): Not found conf-node for " + myname );

	shm = make_shared<SMInterface>(shmID, ui, id, ic);

	UniXML::iterator it(cnode);

	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dinfo << myname << "(init): read fileter-field='" << s_field
		  << "' filter-value='" << s_fvalue << "'" << endl;

	polltime = conf->getArgInt("--" + prefix + "-polltime", it.getProp("polltime"));

	if( polltime <= 0 )
		polltime = 200;

	dinfo << myname << "(init): polltime=" << polltime << endl;

	int updatetime = conf->getArgInt("--" + prefix + "-updatetime", it.getProp("updatetime"));

	if( updatetime <= 0 )
		updatetime = 200;

	dinfo << myname << "(init): updatetime=" << polltime << endl;

	ptUpdate.setTiming(updatetime);

	int sm_tout = conf->getArgInt("--io-sm-ready-timeout", it.getProp("ready_timeout"));

	if( sm_tout == 0 )
		smReadyTimeout = 60000;
	else if( sm_tout < 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;
	else
		smReadyTimeout = sm_tout;

	dinfo << myname << "(init): smReadyTimeout=" << smReadyTimeout << endl;

	if( it.goChildren() )
	{
		for( ; it.getCurrent(); it.goNext() )
		{
			uniset::ObjectId id;

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

			uniset::ObjectId node;

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

			dinfo << myname << ": add point " << n << ":" << n1 << endl;
			nlst.push_back( std::move(ni) );
		}
	}

	if( shm->isLocalwork() )
	{
		readConfiguration();
		mymap.resize(maxIndex);
	}
	else
		ic->addReadItem( sigc::mem_fun(this, &UniExchange::readItem) );
}

// -----------------------------------------------------------------------------
UniExchange::~UniExchange()
{
}
// -----------------------------------------------------------------------------
void UniExchange::execute()
{
	if( !shm->waitSMready(smReadyTimeout, 50) )
	{
		ostringstream err;
		err << myname << "(execute): Не дождались готовности SharedMemory к работе в течение "
			<< smReadyTimeout << " мсек";

		ucrit << err.str() << endl;
		//throw SystemError(err.str());
		std::terminate();
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
		for( auto& it : nlst )
		{
			bool ok = false;

			try
			{
				dinfo << myname << ": connect to id=" << it.id << " node=" << it.node << endl;

				IOController_i::ShortMapSeq_var sseq = ui->getSensors( it.id, it.node );
				ok = true;

				dinfo << myname << " update sensors from id=" << it.id << " node=" << it.node << endl;
				it.update(sseq, shm);
			}
			catch( const uniset::Exception& ex )
			{
				dwarn << myname << "(execute): " << ex << endl;
			}
			catch( ... )
			{
				dwarn << myname << "(execute): catch ..." << endl;
			}

			if( it.sidConnection != DefaultObjectId )
			{
				try
				{
					shm->localSetValue(it.conn_it, it.sidConnection, ok, getId());
				}
				catch(...)
				{
					dcrit << myname << "(execute): sensor not avalible "
						  << uniset_conf()->oind->getNameById(it.sidConnection)
						  << endl;
				}
			}

			if( !ok )
				dinfo << myname << ": ****** cannot connect with node=" << it.node << endl;
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
void UniExchange::NetNodeInfo::update( IOController_i::ShortMapSeq_var& map, const std::shared_ptr<SMInterface>& shm  )
{
	bool reinit = false;

	if( smap.size() != map->length() )
	{
		reinit = true;
		// init new map...
		smap.resize(map->length());
	}

	size_t size = map->length();

	for( size_t i = 0; i < size; i++ )
	{
		SInfo* s = &(smap[i]);
		IOController_i::ShortMap* m = &(map[i]);

		if( reinit )
		{
			shm->initIterator(s->ioit);
			s->type     = m->type;
			s->id         = m->id;
		}

		s->val = m->value;

		try
		{
			/*
			            if( s->type == UniversalIO::DO || s->type == UniversalIO::DI )
			                shm->localSetValue( s->ioit, m->id, (m->value ? 1:0), shm->ID() );
			            else
			                shm->localSetValue( s->ioit, m->id, m->value, shm->ID() );
			*/
			shm->localSetValue( s->ioit, m->id, m->value, shm->ID() );
		}
		catch( const uniset::Exception& ex )
		{
			dwarn  << "(update): " << ex << endl;
		}
		catch( ... )
		{
			dwarn  << "(update): catch ..." << endl;
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

	int i = 0;

	for( auto& it : mymap )
	{
		IOController_i::ShortMap m;
		{
			uniset_rwmutex_rlock lock(it.val_lock);
			m.id     = it.id;
			m.value  = it.val;
			m.type   = it.type;
		}
		(*res)[i++] = m;
	}

	return res;
}
// --------------------------------------------------------------------------
void UniExchange::updateLocalData()
{
	for( auto& it : mymap )
	{
		try
		{
			uniset_rwmutex_wrlock lock(it.val_lock);
			it.val = shm->localGetValue( it.ioit, it.id );
		}
		catch( const uniset::Exception& ex )
		{
			dwarn  << "(update): " << ex << endl;
		}
		catch( ... )
		{
			dwarn  << "(update): catch ..." << endl;
		}
	}

	init_ok = true;
}
// --------------------------------------------------------------------------
void UniExchange::initIterators()
{
	for( auto& it : mymap )
		shm->initIterator(it.ioit);
}
// --------------------------------------------------------------------------
void UniExchange::askSensors( UniversalIO::UIOCommand cmd )
{

}
// -----------------------------------------------------------------------------
void UniExchange::sysCommand( const SystemMessage* sm )
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
std::shared_ptr<UniExchange> UniExchange::init_exchange(int argc, const char* const* argv,
		uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	string p("--" + prefix + "-name");
	string nm(uniset::getArgParam(p, argc, argv, "UniExchange"));

	uniset::ObjectId ID = conf->getControllerID(nm);

	if( ID == uniset::DefaultObjectId )
	{
		cerr << "(uniexchange): Not found ID  for " << nm
			 << " in section " << conf->getControllersSection() << endl;
		return 0;
	}

	return make_shared<UniExchange>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void UniExchange::readConfiguration()
{
	//    readconf_ok = false;
	xmlNode* root = uniset_conf()->getXMLSensorsSection();

	if(!root)
	{
		ostringstream err;
		err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
		throw SystemError(err.str());
	}

	UniXML::iterator it(root);

	if( !it.goChildren() )
	{
		std::cerr << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
		return;
	}

	for( ; it.getCurrent(); it.goNext() )
	{
		if( uniset::check_filter(it, s_field, s_fvalue) )
			initItem(it);
	}

	//    readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool UniExchange::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
	if( uniset::check_filter(it, s_field, s_fvalue) )
		initItem(it);

	return true;
}

// ------------------------------------------------------------------------------------------
bool UniExchange::initItem( UniXML::iterator& it )
{
	SInfo i;

	i.id = DefaultObjectId;

	if( it.getProp("id").empty() )
		i.id = uniset_conf()->getSensorID(it.getProp("name"));
	else
	{
		i.id = it.getIntProp("id");

		if( i.id <= 0 )
			i.id = DefaultObjectId;
	}

	if( i.id == DefaultObjectId )
	{
		dcrit << myname << "(initItem): Unknown ID for "
			  << it.getProp("name") << endl;
		return false;
	}

	i.type = uniset::getIOType(it.getProp("iotype"));

	if( i.type == UniversalIO::UnknownIOType )
	{
		dcrit << myname << "(initItem): Unknown iotype= "
			  << it.getProp("iotype") << " for " << it.getProp("name") << endl;
		return false;
	}

	i.val = 0;

	mymap[maxIndex++] = std::move(i);

	if( maxIndex >= mymap.size() )
		mymap.resize(maxIndex + 10);

	return true;
}
// ------------------------------------------------------------------------------------------
void UniExchange::help_print( int argc, const char** argv )
{
	cout << "--unet-polltime msec     - Пауза между опросами узлов. По умолчанию 200 мсек." << endl;
	//    cout << "--unet-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	//    cout << "--unet-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--unet-sm-ready-timeout - время на ожидание старта SM" << endl;
}
// -----------------------------------------------------------------------------
