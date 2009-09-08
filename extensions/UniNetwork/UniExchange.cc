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
UniExchange::UniExchange( UniSetTypes::ObjectId id, UniSetTypes::ObjectId shmID, 
							SharedMemory* ic, const std::string prefix ):
IOController(id),
shm(0)
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

	if( it.goChildren() )
	{
		UniSetTypes::ObjectId l_id = getSharedMemoryID();
	
		for( ; it.getCurrent(); it.goNext() )
		{
			UniSetTypes::ObjectId id;
		
			string n(it.getProp("id"));
			if( !n.empty() )
				id = it.getIntProp("id");
			else
				id = conf->getControllerID(n);
				
			if( id == DefaultObjectId )
				throw SystemError("(UniExchange): Uknown ID for " + n );

			UniSetTypes::ObjectId node;

			string n1(it.getProp("node_id"));
			if( !n1.empty() )
				node = it.getIntProp("node_id");
			else
				node = conf->getNodeID(n1);
				
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
		readConfiguration();
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
	while(1)
	{
		for( NetNodeList::iterator it=nlst.begin(); it!=nlst.end(); ++it )
		{
			try
			{
				bool ok = false;;
/*
				dlog[Debug::INFO] << myname << ": resolve id=" << it->id
											<< " name="        << conf->oind->getNameById(it->id)
											<< " node="        << it->node
											<< endl;
*/
				for( 
				unsigned int i=0; i<conf->getRepeatCount(); i++)
				{
					try
					{
						if( CORBA::is_nil(it->oref) )
						{
							it->oref = ui.resolve( it->id, it->node );
						}

						if( CORBA::is_nil(it->oref) ) 
							continue;

						it->shm = IONotifyController_i::_narrow(it->oref);
						if( CORBA::is_nil(it->shm) )
						{ 
							it->oref = CORBA::Object::_nil();
							msleep(conf->getRepeatTimeout());
							continue;
						}
						
						if ( it->shm->exist() )
						{
							dlog[Debug::INFO] << " node=" << it->node << ": resolve OK ***" << endl;
							ok = true;
							break;
						}
						
					}
					catch(CORBA::TRANSIENT){}
					catch(CORBA::OBJECT_NOT_EXIST){}
					catch(CORBA::SystemException& ex){}
					catch(...){}

					it->oref = CORBA::Object::_nil();
					msleep(conf->getRepeatTimeout());
				}

				if( it->sidConnection != DefaultObjectId )
				{
					try
					{
//						shm->saveLocalState( it->sidConnection, ok );
						ui.saveState(it->sidConnection, ok, UniversalIO::DigitalInput,conf->getLocalNode());
					}
					catch(...){dlog[Debug::CRIT]<< myname << "(execute): sensor not avalible "<< conf->oind->getNameById( it->sidConnection) <<endl; }
				}

				if( !ok )
				{
					dlog[Debug::INFO] << "****** cannot connect with node=" << it->node << endl;
					continue;
				}
			}
			
			catch( Exception& ex )
			{
				cout  << myname << "(execute): " << ex << endl;
			}
			catch( ... )
			{
				cout  << myname << "(execute): catch ..." << endl;
			}
			
		}
	
		msleep(200);
	}
}
// -----------------------------------------------------------------------------
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
	string nm(conf->getArgParam("--uniexchange-id","UniExchange"));
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
IOController_i::ASensorInfoSeq* UniExchange::getAnalogSensorsMap()
{

}
// -----------------------------------------------------------------------------
IOController_i::DSensorInfoSeq* UniExchange::getDigitalSensorsMap()
{

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
	return true;
}
// ------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void UniExchange::help_print( int argc, const char** argv )
{
	cout << "--unet-polltime msec     - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
//	cout << "--unet-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
//	cout << "--unet-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--unet-sm-ready-timeout - время на ожидание старта SM" << endl;
}
// -----------------------------------------------------------------------------
