#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "MBTCPMultiSlave.h"
#include "modbus/ModbusRTUSlaveSlot.h"
#include "modbus/ModbusTCPServerSlot.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
using namespace ModbusRTU;
// -----------------------------------------------------------------------------
MBTCPMultiSlave::MBTCPMultiSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, const std::shared_ptr<SharedMemory> ic, const string& prefix ):
	MBSlave(objId, shmId, ic, prefix),
	sesscount_id(DefaultObjectId)
{
	auto conf = uniset_conf();

	string conf_name(conf->getArgParam("--" + prefix + "-confnode", myname));
	cnode = conf->getNode(conf_name);

	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBTCPMultiSlave): Not found conf-node for " + myname );

	UniXML::iterator it(cnode);

	waitTimeout = conf->getArgInt("--" + prefix + "-wait-timeout", it.getProp("waitTimeout"));

	if( waitTimeout == 0 )
		waitTimeout = 4000;

	sessTimeout = conf->getArgInt("--" + prefix + "-session-timeout", it.getProp("sessTimeout"));

	if( sessTimeout == 0 )
		sessTimeout = 10000;

	sessMaxNum = conf->getArgInt("--" + prefix + "-session-maxnum", it.getProp("sessMaxNum"));

	if( sessMaxNum == 0 )
		sessMaxNum = 10;

	sesscount_id = conf->getSensorID( conf->getArgParam("--" + prefix + "-session-count-id", it.getProp("sesscount")) );

	UniXML::iterator cit(it);

	if( cit.find("clients") && cit.goChildren() )
	{
		for( ; cit; cit++ )
		{
			ClientInfo c;
			c.iaddr = cit.getProp("ip");

			if( c.iaddr.empty() )
			{
				ostringstream err;
				err << myname << "(init): Unknown ip=''";
				mbcrit << err.str() << endl;
				throw SystemError(err.str());
			}

			// resolve (если получиться)
			ost::InetAddress ia(c.iaddr.c_str());
			c.iaddr = string( ia.getHostname() );

			if( !cit.getProp("respond").empty() )
			{
				c.respond_s = conf->getSensorID(cit.getProp("respond"));

				if( c.respond_s == DefaultObjectId )
				{
					ostringstream err;
					err << myname << "(init): Not found sensor ID for " << cit.getProp("respond");
					mbcrit << err.str() << endl;
					throw SystemError(err.str());
				}
			}

			if( !cit.getProp("askcount").empty() )
			{
				c.askcount_s = conf->getSensorID(cit.getProp("askcount"));

				if( c.askcount_s == DefaultObjectId )
				{
					ostringstream err;
					err << myname << "(init): Not found sensor ID for " << cit.getProp("askcount");
					mbcrit << err.str() << endl;
					throw SystemError(err.str());
				}
			}

			c.invert = cit.getIntProp("invert");

			if( !cit.getProp("timeout").empty() )
			{
				c.tout = cit.getIntProp("timeout");
				c.ptTimeout.setTiming(c.tout);
			}

			cmap[c.iaddr] = c;
			mbinfo << myname << "(init): add client: " << c.iaddr << " respond=" << c.respond_s << " askcount=" << c.askcount_s << endl;
		}
	}
}
// -----------------------------------------------------------------------------
MBTCPMultiSlave::~MBTCPMultiSlave()
{
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::help_print( int argc, const char* const* argv )
{
	MBSlave::help_print(argc, argv);

	cerr << endl;
	cout << "--prefix-wait-timeout msec      - Время ожидания очередного соединения и обновление статистики работы. По умолчанию: 4 сек." << endl;
	cout << "--prefix-session-timeout msec   - Таймаут на закрытие соединения с 'клиентом', если от него нет запросов. По умолчанию: 10 сек." << endl;
	cout << "--prefix-session-maxnum num     - Маскимальное количество соединений. По умолчанию: 10." << endl;
	cout << "--prefix-session-count-id  id   - Датчик для отслеживания текущего количества соединений." << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<MBTCPMultiSlave> MBTCPMultiSlave::init_mbslave( int argc, const char* const* argv, UniSetTypes::ObjectId icID,
		const std::shared_ptr<SharedMemory> ic, const string& prefix )
{
	auto conf = uniset_conf();
	string name = conf->getArgParam("--" + prefix + "-name", "MBSlave1");

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

	dinfo << "(mbslave): name = " << name << "(" << ID << ")" << endl;
	return make_shared<MBTCPMultiSlave>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::execute_tcp()
{
	auto sslot = dynamic_pointer_cast<ModbusTCPServerSlot>(mbslot);

	if( !sslot )
	{
		mbcrit << myname << "(execute_tcp): DYNAMIC CAST ERROR (mbslot --> ModbusTCPServerSlot)" << std::endl;
		raise(SIGTERM);
		return;
	}

	if( mblog->is_level9() )
		sslot->setLog(mblog);

	for( auto && i : cmap )
		i.second.ptTimeout.reset();

	sslot->setMaxSessions( sessMaxNum );

	mbinfo << myname << "(execute_tcp): thread running.." << endl;

	while( !cancelled )
	{
		try
		{
			sslot->waitQuery( addr, waitTimeout );

			// Обновляем информацию по соединениям
			sess.clear();
			sslot->getSessions(sess);

			for( auto && s : sess )
			{
				auto i = cmap.find( s.iaddr );

				if( i != cmap.end() )
				{
					// если ещё в списке, значит отвечает (т.е. сбрасываем таймер)
					if( i->second.tout == 0 )
						i->second.ptTimeout.setTiming( waitTimeout );
					else
						i->second.ptTimeout.reset();

					i->second.askCount = s.askCount;
				}
			}

			// а теперь проходим по списку и выставляем датчики..
			for( const auto& it : cmap )
			{
				auto c = it.second;

				mblog4 << myname << "(work): " << c.iaddr << " resp=" << (c.invert ? c.ptTimeout.checkTime() : !c.ptTimeout.checkTime())
					  << " askcount=" << c.askCount
					  << endl;


				if( c.respond_s != DefaultObjectId )
				{
					try
					{
						bool st = c.invert ? c.ptTimeout.checkTime() : !c.ptTimeout.checkTime();
						shm->localSetValue(c.respond_it, c.respond_s, st, getId());
					}
					catch( const Exception& ex )
					{
						mbcrit << myname << "(execute_tcp): " << ex << std::endl;
					}
				}

				if( c.askcount_s != DefaultObjectId )
				{
					try
					{
						shm->localSetValue(c.askcount_it, c.askcount_s, c.askCount, getId());
					}
					catch( const Exception& ex )
					{
						mbcrit << myname << "(execute_tcp): " << ex << std::endl;
					}
				}
			}

#if 0

			if( res != ModbusRTU::erTimeOut )
				ptTimeout.reset();

			// собираем статистику обмена
			if( prev != ModbusRTU::erTimeOut )
			{
				//  с проверкой на переполнение
				askCount = askCount >= numeric_limits<long>::max() ? 0 : askCount + 1;

				if( res != ModbusRTU::erNoError )
					errmap[res]++;
			}

			prev = res;

			if( res != ModbusRTU::erNoError && res != ModbusRTU::erTimeOut )
				mblog[Debug::WARN] << myname << "(execute_tcp): " << ModbusRTU::mbErr2Str(res) << endl;

#endif

			if( !activated )
				continue;

			if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
			{
				try
				{
					shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
					ptHeartBeat.reset();
				}
				catch( const Exception& ex )
				{
					mbcrit << myname << "(execute_tcp): (hb) " << ex << std::endl;
				}
			}

			if( respond_id != DefaultObjectId )
			{
				bool state = ptTimeout.checkTime() ? false : true;

				if( respond_invert )
					state ^= true;

				try
				{
					shm->localSetValue(itRespond, respond_id, (state ? 1 : 0), getId());
				}
				catch( const Exception& ex )
				{
					mbcrit << myname << "(execute_rtu): (respond) " << ex << std::endl;
				}
			}

			if( askcount_id != DefaultObjectId )
			{
				try
				{
					shm->localSetValue(itAskCount, askcount_id, askCount, getId());
				}
				catch( const Exception& ex )
				{
					mbcrit << myname << "(execute_rtu): (askCount) " << ex << std::endl;
				}
			}

			if( sesscount_id != DefaultObjectId )
			{
				try
				{
					shm->localSetValue(sesscount_it, sesscount_id, sslot->getCountSessions(), getId());
				}
				catch( const Exception& ex )
				{
					mbcrit << myname << "(execute_rtu): (sessCount) " << ex << std::endl;
				}
			}
		}
		catch(...) {}
	}

	mbinfo << myname << "(execute_tcp): thread stopped.." << endl;
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::initIterators()
{
	MBSlave::initIterators();

	shm->initIterator(sesscount_it);

	for( auto && i : cmap )
		i.second.initIterators(shm);
}
// -----------------------------------------------------------------------------
bool MBTCPMultiSlave::deactivateObject()
{
	if( mbslot )
	{
		auto sslot = dynamic_pointer_cast<ModbusTCPServerSlot>(mbslot);

		if( sslot )
			sslot->sigterm(SIGTERM);
	}

	return MBSlave::deactivateObject();
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::sigterm( int signo )
{
	if( mbslot )
	{
		auto sslot = dynamic_pointer_cast<ModbusTCPServerSlot>(mbslot);

		if( sslot )
			sslot->sigterm(signo);
	}

	MBSlave::sigterm(signo);
}
// -----------------------------------------------------------------------------
