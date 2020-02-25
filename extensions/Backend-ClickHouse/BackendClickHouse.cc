/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#include <iomanip>
#include "Exceptions.h"
#include "UniSetTypes.h"
#include "unisetstd.h"
#include "BackendClickHouse.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
BackendClickHouse::TagList BackendClickHouse::parseTags( const std::string& tags )
{
	BackendClickHouse::TagList ret;

	auto taglist = uniset::explode_str(tags, ' ');
	if( taglist.empty() )
		return ret;

	for( const auto& t: taglist )
	{
		auto tag = uniset::explode_str(t, '=');
		if( tag.size() < 2 )
			throw uniset::SystemError("Bad format for tag '" + t + "'. Must be 'tag=val'");

		ret.emplace_back(std::make_pair(tag[0], tag[1]));
	}

	return ret;
}
//--------------------------------------------------------------------------------
BackendClickHouse::BackendClickHouse( uniset::ObjectId objId, xmlNode* cnode,
								  uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
								  const string& prefix ):
	UObject_SK(objId, cnode, string(prefix + "-")),
	prefix(prefix)
{
	auto conf = uniset_conf();

	if( ic )
		ic->logAgregator()->add(logAgregator());

	shm = make_shared<SMInterface>(shmId, ui, objId, ic);
	db = unisetstd::make_unique<ClickHouseInterface>();
	createColumns();

	init(cnode);

	if( smTestID == DefaultObjectId && !clickhouseParams.empty() )
	{
		// берём первый датчик из списка
		smTestID = clickhouseParams.begin()->first;
	}
}
// -----------------------------------------------------------------------------
BackendClickHouse::~BackendClickHouse()
{
}
// -----------------------------------------------------------------------------
void BackendClickHouse::init( xmlNode* cnode )
{
	UniXML::iterator it(cnode);

	auto conf = uniset_conf();

	dbhost = conf->getArg2Param("--" + prefix + "-dbhost", it.getProp("dbhost"), "localhost");
	dbport = conf->getArgPInt("--" + prefix + "-dbport", it.getProp("dbport"), 9000);
	dbuser = conf->getArg2Param("--" + prefix + "-dbuser", it.getProp("dbuser"), "");
	dbpass = conf->getArg2Param("--" + prefix + "-dbpass", it.getProp("dbpass"), "");
	dbname = conf->getArg2Param("--" + prefix + "-dbname", it.getProp("dbname"), "");
	reconnectTime = conf->getArgPInt("--" + prefix + "-reconnect-time", it.getProp("reconnectTime"), reconnectTime);
	bufMaxSize = conf->getArgPInt("--" + prefix + "-buf-maxsize", it.getProp("bufMaxSize"), bufMaxSize);
	bufSize = conf->getArgPInt("--" + prefix + "-buf-size", it.getProp("bufMaxSize"), bufSize);
	bufSyncTime = conf->getArgPInt("--" + prefix + "-buf-sync-time", it.getProp("bufSyncTimeout"), bufSyncTime);

	const string tblname = conf->getArg2Param("--" + prefix + "-dbtablename", it.getProp("dtablebname"), "main_history");
	fullTableName = dbname.empty() ? tblname : dbname + "." + tblname;

	int sz = conf->getArgPInt("--" + prefix + "-uniset-object-size-message-queue", it.getProp("sizeOfMessageQueue"), 10000);
	if( sz > 0 )
		setMaxSizeOfMessageQueue(sz);

	const string ff = conf->getArg2Param("--" + prefix + "-filter-field", it.getProp("filter_field"), "" );
	const string fv = conf->getArg2Param("--" + prefix + "-filter-value", it.getProp("filter_value"), "" );

	const string gtags = conf->getArg2Param("--" + prefix + "-tags", it.getProp("tags"), "");
	globalTags = parseTags(gtags);

	myinfo << myname << "(init): clickhouse host=" << dbhost << ":" << dbport << " user=" << dbuser
		   << " " << ff << "='" << fv << "'"
		   << " tags='" << gtags << "'"
		   << endl;

	// try
	{
		auto conf = uniset_conf();

		xmlNode* snode = conf->getXMLSensorsSection();

		if( !snode )
		{
			ostringstream err;
			err << myname << "(init): Not found section <sensors>";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		UniXML::iterator it1(snode);

		if( !it1.goChildren() )
		{
			ostringstream err;
			err << myname << "(init): section <sensors> empty?!";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		for(; it1.getCurrent(); it1.goNext() )
		{
			if( !uniset::check_filter(it1, ff, fv) )
				continue;

			const std::string name = it1.getProp("name");
			ObjectId sid = conf->getSensorID( name );

			if( sid == DefaultObjectId )
			{
				ostringstream err;
				err << myname << "(init): Unknown SensorID for '" << name << "'";
				mycrit << err.str();
				throw SystemError(err.str());
			}

			auto tags = parseTags(it1.getProp("clickhouse_tags"));

			clickhouseParams.emplace( sid, ParamInfo(name, tags) );
		}

		if( clickhouseParams.empty() )
		{
			ostringstream err;
			err << myname << "(init): Not found items for send to ClickHouse..";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

	}

	myinfo << myname << "(init): " << clickhouseParams.size() << " sensors.." << endl;
}
//--------------------------------------------------------------------------------
void BackendClickHouse::createColumns()
{
	colTimeStamp = std::make_shared<clickhouse::ColumnDateTime>();
	colTimeUsec = std::make_shared<clickhouse::ColumnUInt64>();
	colSensorID = std::make_shared<clickhouse::ColumnUInt64>();
	colValue = std::make_shared<clickhouse::ColumnFloat64>();
	colNode = std::make_shared<clickhouse::ColumnUInt64>();
	colConfirm = std::make_shared<clickhouse::ColumnUInt64>();
	arrTagKeys = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
	arrTagValues = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::clearData()
{
	colTimeStamp->Clear();
	colTimeUsec->Clear();
	colSensorID->Clear();
	colValue->Clear();
	colNode->Clear();
	colConfirm->Clear();
	arrTagKeys->Clear();
	arrTagValues->Clear();
}
//--------------------------------------------------------------------------------
void BackendClickHouse::help_print( int argc, const char* const* argv )
{
	cout << " Default prefix='opentsdb'" << endl;
	cout << "--prefix-name                - ID. Default: BackendClickHouse." << endl;
	cout << "--prefix-confnode            - configuration section name. Default: <NAME name='NAME'...> " << endl;
	cout << endl;
	cout << " OpenTSDB: " << endl;
	cout << "--prefix-host  ip                         - OpenTSDB: host. Default: localhost" << endl;
	cout << "--prefix-port  num                        - OpenTSDB: port. Default: 4242" << endl;
	cout << "--prefix-prefix name                      - OpenTSDB: prefix for data" << endl;
	cout << "--prefix-tags  'TAG1=VAL1 TAG2=VAL2...'   - OpenTSDB: tags for data" << endl;
	cout << "--prefix-reconnect-time msec              - Time for attempts to connect to DB. Default: 5 sec" << endl;
	cout << endl;
	cout << "--prefix-buf-size  sz        - Buffer before save to DB. Default: 500" << endl;
	cout << "--prefix-buf-maxsize  sz     - Maximum size for buffer (drop messages). Default: 5000" << endl;
	cout << "--prefix-buf-sync-time msec  - Time period for forced data writing to DB. Default: 5 sec" << endl;
	cout << endl;
	cout << "--prefix-heartbeat-id name   - ID for heartbeat sensor." << endl;
	cout << "--prefix-heartbeat-max val   - max value for heartbeat sensor." << endl;
	cout << endl;
	cout << " Logs: " << endl;
	cout << "--prefix-log-...            - log control" << endl;
	cout << "             add-levels ...  " << endl;
	cout << "             del-levels ...  " << endl;
	cout << "             set-levels ...  " << endl;
	cout << "             logfile filanme " << endl;
	cout << "             no-debug " << endl;
	cout << endl;
	cout << " LogServer: " << endl;
	cout << "--prefix-run-logserver      - run logserver. Default: localhost:id" << endl;
	cout << "--prefix-logserver-host ip  - listen ip. Default: localhost" << endl;
	cout << "--prefix-logserver-port num - listen port. Default: ID" << endl;
	cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<BackendClickHouse> BackendClickHouse::init_clickhouse( int argc,
		const char* const* argv,
		uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "BackendClickHouse");

	if( name.empty() )
	{
		dcrit << "(BackendClickHouse): Unknown name. Usage: --" <<  prefix << "-name" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == uniset::DefaultObjectId )
	{
		dcrit << "(BackendClickHouse): Not found ID for '" << name
			  << " in '" << conf->getObjectsSection() << "' section" << endl;
		return 0;
	}

	string confname = conf->getArgParam("--" + prefix + "-confnode", name);
	xmlNode* cnode = conf->getNode(confname);

	if( !cnode )
	{
		dcrit << "(BackendClickHouse): " << name << "(init): Not found <" + confname + ">" << endl;
		return 0;
	}

	dinfo << "(BackendClickHouse): name = " << name << "(" << ID << ")" << endl;
	return make_shared<BackendClickHouse>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void BackendClickHouse::callback() noexcept
{
	// используем стандартную "низкоуровневую" реализацию
	// т.к. она нас устраивает (обработка очереди сообщений и таймеров)
	UniSetObject::callback();
}
// -----------------------------------------------------------------------------
void BackendClickHouse::askSensors( UniversalIO::UIOCommand cmd )
{
	UObject_SK::askSensors(cmd);

	// прежде чем заказывать датчики, надо убедиться что SM доступна
	if( !waitSM(smReadyTimeout) )
	{
		uterminate();
		return;
	}

	for( const auto& s : clickhouseParams )
	{
		try
		{
			shm->askSensor(s.first, cmd);
		}
		catch( const std::exception& ex )
		{
			mycrit << myname << "(askSensors): " << ex.what() << endl;
		}
	}
}
// -----------------------------------------------------------------------------
void BackendClickHouse::sensorInfo( const uniset::SensorMessage* sm )
{
	auto it = clickhouseParams.find(sm->id);

	if( it == clickhouseParams.end() )
		return;

	try
	{
		if( !sm->tm.tv_sec )
		{
			// Выдаём CRIT, но тем не менее сохраняем в БД
			mycrit << myname << "(insert_main_history): UNKNOWN TIMESTAMP! (tm.tv_sec=0)"
				   << " for sid=" << sm->id
				   << " supplier=" << uniset_conf()->oind->getMapName(sm->supplier)
				   << endl;
		}

		colTimeStamp->Append(sm->sm_tv.tv_sec);
		colTimeUsec->Append(sm->sm_tv.tv_nsec);
		colSensorID->Append(sm->id);
		colValue->Append(sm->value);
		colNode->Append(sm->node);
		colConfirm->Append(0);

		// TAGS
		auto key = std::make_shared<clickhouse::ColumnString>();
		auto val = std::make_shared<clickhouse::ColumnString>();
		for( const auto& t: it->second.tags )
		{
			key->Append(t.first);
			val->Append(t.second);
		}

		// GLOBAL TAGS
		for( const auto& t: globalTags )
		{
			key->Append(t.first);
			val->Append(t.second);
		}

		// save tags
		arrTagKeys->AppendAsColumn(key);
		arrTagValues->AppendAsColumn(val);

		if( colTimeStamp->Size() >= bufSize )
		{
			if( flushBuffer() )
				return;

			if( colTimeStamp->Size() >= bufMaxSize )
			{
				mycrit << "BUFFER OVERFLOW! MaxBufSize=" << bufMaxSize
					   << ". ALL DATA LOST!" << endl;
				clearData();
			}
		}

		if( !timerIsOn )
		{
			timerIsOn = true;
			askTimer(tmFlushBuffer, bufSyncTime, 1);
			return;
		}
	}
	catch( const uniset::Exception& ex )
	{
		mycrit << myname << "(insert_main_history): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		mycrit << myname << "(insert_main_messages): " << ex.what() << endl;
	}
	catch( ... )
	{
		mycrit << myname << "(insert_main_history): catch ..." << endl;
	}
}
// -----------------------------------------------------------------------------
void BackendClickHouse::timerInfo( const uniset::TimerMessage* tm )
{
	if( tm->id == tmFlushBuffer )
	{
		if( flushBuffer() )
			timerIsOn = false;
	}
	else if( tm->id == tmReconnect )
	{
		if( reconnect() )
			askTimer(tmReconnect, 0);
	}
}
// -----------------------------------------------------------------------------
void BackendClickHouse::sysCommand(const SystemMessage* sm)
{
	if( sm->command == SystemMessage::StartUp )
	{
		if( !reconnect() )
			askTimer(tmReconnect, reconnectTime);
	}
}
// -----------------------------------------------------------------------------
bool BackendClickHouse::flushBuffer()
{
	if( colTimeStamp->Size() == 0 )
		return false;

	if( !db || !connect_ok )
		return false;

	myinfo << myname << "(flushInsertBuffer): write insert buffer[" << colTimeStamp->Size() << "] to DB.." << endl;

	clickhouse::Block blk(8,colTimeStamp->Size());
	blk.AppendColumn("timestamp", colTimeStamp);
	blk.AppendColumn("time_usec", colTimeUsec);
	blk.AppendColumn("sensor_id", colSensorID);
	blk.AppendColumn("value", colValue);
	blk.AppendColumn("node", colNode);
	blk.AppendColumn("confirm", colConfirm);
	blk.AppendColumn("tags.name", arrTagKeys);
	blk.AppendColumn("tags.value", arrTagValues);

	if( !db->insert(fullTableName, blk) )
	{
		mycrit << myname << "(flushInsertBuffer): error: " << db->error() << endl;
		return false;
	}

	clearData();
	return true;
}
//------------------------------------------------------------------------------
bool BackendClickHouse::reconnect()
{
	connect_ok = db->reconnect(dbhost, dbuser, dbpass, dbname, dbport);
	return connect_ok;
}
//------------------------------------------------------------------------------
std::string BackendClickHouse::getMonitInfo() const
{
	ostringstream inf;

	inf << "Database: " << dbhost << ":" << dbport << " user=" << dbuser
		<< " ["
		<< " reconnect=" << reconnectTime
		<< " bufSyncTime=" << bufSyncTime
		<< " bufSize=" << bufSize
		<< " tsdbTags: ";
		for( const auto& t: globalTags )
			inf << t.first << "=" << t.second << " ";

		inf << " ]" << endl
		<< "  connection: " << ( connect_ok ? "OK" : "FAILED") << endl
		<< " buffer size: " << colTimeStamp->Size() << endl
		<< "   lastError: " << lastError << endl;

	return inf.str();
}
// -----------------------------------------------------------------------------
