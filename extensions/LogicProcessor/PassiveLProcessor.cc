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
#include <iostream>
#include "Configuration.h"
#include "PassiveLProcessor.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -------------------------------------------------------------------------
PassiveLProcessor::PassiveLProcessor( uniset::ObjectId objId,
									  uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic, const std::string& prefix ):
	UniSetObject(objId),
	shm(nullptr)
{
	auto conf = uniset_conf();

	logname = myname;
	shm = make_shared<SMInterface>(shmID, UniSetObject::ui, objId, ic);

	string conf_name(conf->getArgParam("--" + prefix + "-confnode", myname));

	xmlNode* confnode = conf->getNode(conf_name);

	if( confnode == NULL )
		throw SystemError("Not found conf-node for " + conf_name );

	UniXML::iterator it(confnode);
	string lfile = conf->getArgParam("--" + prefix + "-schema", it.getProp("schema"));

	if( lfile.empty() )
	{
		ostringstream err;
		err << myname << "(init): Unknown schema file.. Use: --" << prefix + "-schema";
		throw SystemError(err.str());
	}

	build(lfile);

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", ""); // it.getProp("heartbeat_id"));

	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);

		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('HeartBeat') for " << heart;
			dcrit << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time", conf->getHeartBeatTime());

		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", "10", 10);
	}
}
// -------------------------------------------------------------------------
PassiveLProcessor::~PassiveLProcessor()
{

}
// -------------------------------------------------------------------------
void PassiveLProcessor::step()
{
	try
	{
		LProcessor::step();
	}
	catch( const uniset::Exception& ex )
	{
		dcrit << myname << "(step): (hb) " << ex << std::endl;
	}

	if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
			ptHeartBeat.reset();
		}
		catch( const uniset::Exception& ex )
		{
			dcrit << myname << "(step): (hb) " << ex << std::endl;
		}
	}

}
// -------------------------------------------------------------------------
void PassiveLProcessor::getInputs()
{

}
// -------------------------------------------------------------------------
void PassiveLProcessor::askSensors( UniversalIO::UIOCommand cmd )
{
	try
	{
		for( auto && it : extInputs )
			shm->askSensor(it.sid, cmd);
	}
	catch( const uniset::Exception& ex )
	{
		dcrit << myname << "(askSensors): " << ex << endl;
		throw SystemError(myname + "(askSensors): do not ask sensors" );
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sensorInfo( const uniset::SensorMessage* sm )
{
	for( auto && it : extInputs )
	{
		if( it.sid == sm->id )
			it.value = sm->value;
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::timerInfo( const uniset::TimerMessage* tm )
{
	if( tm->id == tidStep )
		step();
}
// -------------------------------------------------------------------------
void PassiveLProcessor::sysCommand( const uniset::SystemMessage* sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( !shm->waitSMreadyWithCancellation(smReadyTimeout, cannceled) )
			{
				dcrit << myname << "(ERR): SM not ready. Terminated... " << endl << flush;
				//				std::terminate();
				uterminate();
				return;
			}

			std::lock_guard<std::mutex> l(mutex_start);
			askSensors(UniversalIO::UIONotify);
			askTimer(tidStep, LProcessor::sleepTime);
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
			// (т.е. RTUExchange  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(RTUExchange)
			if( shm->isLocalwork() )
				break;

			askSensors(UniversalIO::UIONotify);
		}
		break;

		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			ulogany << myname << "(sysCommand): logRotate" << std::endl;
			string fname (ulog()->getLogFile() );

			if( !fname.empty() )
			{
				ulog()->logFile(fname, true);
				ulogany << myname << "(sysCommand): ***************** ulog LOG ROTATE *****************" << std::endl;
			}

			dlogany << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog()->getLogFile();

			if( !fname.empty() )
			{
				dlog()->logFile(fname, true);
				dlogany << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// -------------------------------------------------------------------------
bool PassiveLProcessor::activateObject()
{
	// блокирование обработки StarUp
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		std::lock_guard<std::mutex> l(mutex_start);
		UniSetObject::activateObject();
		initIterators();
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool PassiveLProcessor::deactivateObject()
{
	cannceled = true;

	for( const auto& it : extOuts )
	{
		try
		{
			shm->setValue(it.sid, 0);
		}
		catch( const std::exception& ex )
		{
			dcrit << myname << "(sigterm): catch:" << ex.what() << endl;
		}
	}

	return UniSetObject::deactivateObject();
}
// ------------------------------------------------------------------------------------------
void PassiveLProcessor::initIterators()
{
	shm->initIterator(itHeartBeat);
}
// -------------------------------------------------------------------------
void PassiveLProcessor::setOuts()
{
	// выcтавляем выходы
	for( auto && it : extOuts )
	{
		try
		{
			shm->setValue( it.sid, it.el->getOut() );
		}
		catch( const uniset::Exception& ex )
		{
			dcrit << myname << "(setOuts): " << ex << endl;
		}
		catch( const std::exception& ex )
		{
			dcrit << myname << "(setOuts): catch: " << ex.what() << endl;
		}
	}
}
// -------------------------------------------------------------------------
void PassiveLProcessor::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='plproc'" << endl;
	cout << "--prefix-name  name        - ObjectID. По умолчанию: PassiveProcessor1" << endl;
	cout << "--prefix-confnode cnode    - Возможность задать настроечный узел в configure.xml. По умолчанию: name" << endl;
	cout << endl;
	cout << "--prefix-schema file       - Файл с логической схемой." << endl;
	cout << "--prefix-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--prefix-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;

}
// -----------------------------------------------------------------------------
std::shared_ptr<PassiveLProcessor> PassiveLProcessor::init_plproc(int argc, const char* const* argv,
		uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();
	string name = conf->getArgParam("--" + prefix + "-name", "PassiveProcessor1");

	if( name.empty() )
	{
		cerr << "(plproc): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == uniset::DefaultObjectId )
	{
		cerr << "(plproc): идентификатор '" << name
			 << "' не найден в конф. файле!"
			 << " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dinfo << "(plproc): name = " << name << "(" << ID << ")" << endl;
	return make_shared<PassiveLProcessor>(ID, shmID, ic, prefix);
}
// -----------------------------------------------------------------------------
