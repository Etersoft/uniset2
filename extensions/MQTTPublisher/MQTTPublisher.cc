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
#include "Exceptions.h"
#include "MQTTPublisher.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MQTTPublisher::MQTTPublisher(UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
							 const string& prefix ):
	mosquittopp(NULL),
	UObject_SK(objId, cnode, string(prefix + "-")),
	prefix(prefix)
{
	auto conf = uniset_conf();

	if( ic )
		ic->logAgregator()->add(logAgregator());

	shm = make_shared<SMInterface>(shmId, ui, objId, ic);

	UniXML::iterator it(cnode);

	std::string t = conf->getArg2Param("--" + argprefix + "mqtt-topic", it.getProp("topic"), "");

	{
		ostringstream s;
		if( t.empty() )
			s <<  conf->getRootSection() << "/sensors";
		else
			s << t;

		topic = s.str();
	}

	string ff = conf->getArg2Param("--" + argprefix + "filter-field", it.getProp("filterField"), "");
	string fv = conf->getArg2Param("--" + argprefix + "filter-value", it.getProp("filterValue"), "");

	myinfo << myname << "(init): filter-field=" << ff << " filter-value=" << fv << endl;

	xmlNode* senssec = conf->getXMLSensorsSection();

	if( !senssec )
	{
		ostringstream err;
		err << myname << "(init): Not found sensors section";
		mycrit << err.str() << endl;
		throw SystemError(err.str());
	}

	UniXML::iterator sit(senssec);

	if( !sit.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): empty sensors section?!";
		mycrit << err.str() << endl;
		throw SystemError(err.str());
	}

	ostringstream pubname;

	for( ; sit.getCurrent(); sit++ )
	{
		if( !UniSetTypes::check_filter(sit, ff, fv) )
			continue;

		std::string sname = sit.getProp("name");
		ObjectId sid = conf->getSensorID(sname);

		if( sid == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(init): Unkown ID for sensor '" << sname << "'";
			mycrit << err.str() << endl;
			throw SystemError(err.str());
		}

		pubname.str("");
		pubname << topic << "/" << sname;

		MQTTInfo m(sid, pubname.str());
		publist.emplace(sid, std::move(m) );

		if( smTestID == DefaultObjectId )
			smTestID = sid;

		UniXML::iterator i(sit);

		if( !i.goChildren() )
			continue;

		if( !i.find("mqtt") )
			continue;

		MQTTTextInfo mi(topic, sit, i);
		textpublist.emplace(sid, std::move(mi) );
	}

	if( publist.empty() )
	{
		ostringstream err;
		err << myname << "(init): FAIL! Empty publish list...";
		mycrit << err.str() << endl;
		throw SystemError(err.str());
	}

	// Работа с MQTT
	mosqpp::lib_init();
	host = conf->getArg2Param("--" + argprefix + "mqtt-host", it.getProp("mqttHost"), "localhost");
	port = conf->getArgPInt("--" + argprefix + "mqtt-port", it.getProp("mqttPort"), 1883);
	keepalive = conf->getArgPInt("--" + argprefix + "mqtt-keepalive", it.getProp("mqttKeepAlive"), 60);

	// см. sysCommad()
	//	connect_async(host.c_str(),port,keepalive);
	//	loop_start();
}
// -----------------------------------------------------------------------------
MQTTPublisher::~MQTTPublisher()
{
	loop_stop();            // Kill the thread
	mosqpp::lib_cleanup();  // Mosquitto library cleanup
}
// -----------------------------------------------------------------------------
void MQTTPublisher::sigterm(int signo )
{
	myinfo << myname << "(sigterm): signo=" << signo << endl;
	disconnect();
	connectOK = false;
	UObject_SK::sigterm(signo);
}
//--------------------------------------------------------------------------------
bool MQTTPublisher::deactivateObject()
{
	myinfo << myname << "(deactivateObject): ...disconnect.." << endl;
	disconnect();
	connectOK = false;
	return UObject_SK::deactivateObject();
}
//--------------------------------------------------------------------------------
void MQTTPublisher::sysCommand(const SystemMessage* sm)
{
	UObject_SK::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
	{
		if( !connectOK )
		{
			connect_async(host.c_str(), port, keepalive);
			loop_start();
		}
	}
}
//--------------------------------------------------------------------------------
void MQTTPublisher::help_print( int argc, const char* const* argv )
{
	cout << " Default prefix='mqtt'" << endl;
	cout << "--prefix-name        - ID for mqttpublisher. Default: MQTTPublisher1. " << endl;
	cout << "--prefix-confnode    - configuration section name. Default: <NAME name='NAME'...> " << endl;
	cout << "--prefix-heartbeat-id name   - ID for heartbeat sensor." << endl;
	cout << "--prefix-heartbeat-max val   - max value for heartbeat sensor." << endl;
	cout << "--prefix-filter-field        - Фильтр для загрузки списка датчиков." << endl;
	cout << "--prefix-filter-value        - Значение фильтра для загрузки списка датчиков." << endl;
	cout << endl;
	cout << " MQTT: " << endl;
	cout << "--prefix-mqtt-topic name     - Name for topic. Default: ROOT_NAME_PRROJECT/sensors/sensorname'" << endl;
	cout << "--prefix-mqtt-host host           - host(ip) MQTT Broker (server). Default: localhost" << endl;
	cout << "--prefix-mqtt-port port           - port for MQTT Broker (server). Default: 1883" << endl;
	cout << "--prefix-mqtt-keepalive val       - keepalive for connection to MQTT Broker (server). Default: 60" << endl;
	cout << endl;
	cout << " Logs: " << endl;
	cout << "--prefix-log-...            - log control" << endl;
	cout << "             add-levels ...  " << endl;
	cout << "             del-levels ...  " << endl;
	cout << "             set-levels ...  " << endl;
	cout << "             logfile filanme " << endl;
	cout << "             no-debug " << endl;
	cout << " LogServer: " << endl;
	cout << "--prefix-run-logserver      - run logserver. Default: localhost:id" << endl;
	cout << "--prefix-logserver-host ip  - listen ip. Default: localhost" << endl;
	cout << "--prefix-logserver-port num - listen port. Default: ID" << endl;
	cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
void MQTTPublisher::on_connect(int rc)
{
	connectOK = ( rc == 0 );
	myinfo << myname << "(on_connect): connect to " << host << ":" <<  port << " " << ( connectOK ? "OK" : "FAIL" ) << endl;

	if( connectOK )
		askSensors(UniversalIO::UIONotify);

	//	else
	//	{
	//		askTimer(reconnectTimer,reconnectTime);
	//	}
}
// -----------------------------------------------------------------------------
void MQTTPublisher::on_message( const mosquitto_message* message )
{

}
// -----------------------------------------------------------------------------
void MQTTPublisher::on_subscribe( int mid, int qos_count, const int* granted_qos )
{

}
// -----------------------------------------------------------------------------
std::shared_ptr<MQTTPublisher> MQTTPublisher::init_mqttpublisher(int argc, const char* const* argv,
		UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "MQTTPublisher");

	if( name.empty() )
	{
		dcrit << "(MQTTPublisher): Unknown name. Usage: --" <<  prefix << "-name" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == UniSetTypes::DefaultObjectId )
	{
		dcrit << "(MQTTPublisher): Not found ID for '" << name
			  << " in '" << conf->getObjectsSection() << "' section" << endl;
		return 0;
	}

	string confname = conf->getArgParam("--" + prefix + "-confnode", name);
	xmlNode* cnode = conf->getNode(confname);

	if( !cnode )
	{
		dcrit << "(MQTTPublisher): " << name << "(init): Not found <" + confname + ">" << endl;
		return 0;
	}

	dinfo << "(MQTTPublisher): name = " << name << "(" << ID << ")" << endl;
	return make_shared<MQTTPublisher>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void MQTTPublisher::askSensors( UniversalIO::UIOCommand cmd )
{
	UObject_SK::askSensors(cmd);

	for( const auto& i : publist )
	{
		try
		{
			shm->askSensor(i.second.sid, cmd);
		}
		catch( const std::exception& ex )
		{
			mycrit << myname << "(askSensors): " << ex.what() << endl;
		}
	}
}
// -----------------------------------------------------------------------------
void MQTTPublisher::sensorInfo( const UniSetTypes::SensorMessage* sm )
{
	auto i = publist.find(sm->id);

	if( i != publist.end() )
	{
		ostringstream m;
		m << sm->value;

		string tmsg(m.str());

		//subscribe(NULL, i.second.pubname.c_str());
		myinfo << "(sensorInfo): publish: topic='" << i->second.pubname << "' msg='" << tmsg.c_str() << "'" << endl;

		int ret = publish(NULL, i->second.pubname.c_str(), tmsg.size(), tmsg.c_str(), 1, false);

		if( ret != MOSQ_ERR_SUCCESS )
		{
			mycrit << myname << "(sensorInfo): PUBLISH FAILED: err(" << ret << "): " << mosqpp::strerror(ret) << endl;
		}
	}

	auto t = textpublist.find(sm->id);

	if( t != textpublist.end() )
		t->second.check(this, sm->value, mylog, myname);
}
// -----------------------------------------------------------------------------
MQTTPublisher::MQTTTextInfo::MQTTTextInfo( const string& rootsec, UniXML::iterator s, UniXML::iterator i ):
	xmlnode(s)
{
	auto conf = uniset_conf();
	sid = conf->getSensorID(s.getProp("name"));

	string sname(s.getProp("name"));


	if( sid == DefaultObjectId )
	{
		ostringstream err;
		err << "(MQTTTextInfo): Unknown ID for " << sname;
		throw SystemError(err.str());
	}

	std::string subtopic(i.getProp("subtopic"));

	if( !subtopic.empty() )
		pubname = rootsec + "/" + subtopic;
	else
		pubname = rootsec + "/" + sname + "/textevent";

	if( !i.goChildren() )
	{
		ostringstream err;
		err << "(MQTTTextInfo): INIT FAIL! empty list <mqtt> for " << sname;
		throw SystemError(err.str());
	}

	for( ; i.getCurrent(); i++ )
	{
		long min = 0;
		long max = 0;

		if( i.getName() == "range" )
		{
			min = i.getIntProp("min");
			max = i.getIntProp("max");

			if( min > max )
				std::swap(min, max);
		}
		else
		{
			min = max = i.getIntProp("value");
		}

		RangeInfo r(min, max, i.getProp("text"));
		rlist.push_back( std::move(r) );
	}
}
// -----------------------------------------------------------------------------
bool MQTTPublisher::RangeInfo::check( long val ) const
{
	return ( val >= rmin && val <= rmax );
}
//--------------------------------------------------------------------------------
void MQTTPublisher::MQTTTextInfo::check( mosqpp::mosquittopp* serv, long value , std::shared_ptr<DebugStream>& log , const string& myname )
{
	for( auto && r : rlist )
	{
		if( r.check(value) )
		{
			string tmsg = replace(&r, value);

			//subscribe(NULL, i.second.pubname.c_str());
			if( log->is_info() )
				log->info() << myname << "(check): publish: topic='" << pubname << "' msg='" << tmsg << "'" << endl;

			int ret = serv->publish(NULL, pubname.c_str(), tmsg.size(), tmsg.c_str(), 1, false);

			if( ret != MOSQ_ERR_SUCCESS )
			{
				if( log->is_crit() )
					log->crit() << myname << "(check): PUBLISH FAILED: err(" << ret << "): " << mosqpp::strerror(ret) << endl;
			}
		}
	}
}
//--------------------------------------------------------------------------------
std::string MQTTPublisher::MQTTTextInfo::replace( RangeInfo* ri, long value )
{
	std::string txt(ri->text);

	ostringstream v;
	v << value;

	ostringstream id;
	id << sid;

	ostringstream r;
	r << "[" << ri->rmin << ":" << ri->rmax << "]";

	ostringstream smin;
	smin << ri->rmin;

	ostringstream smax;
	smax << ri->rmax;

	txt = replace_all(txt, "%v", v.str());
	txt = replace_all(txt, "%n", xmlnode.getProp("name"));
	txt = replace_all(txt, "%t", xmlnode.getProp("textname"));
	txt = replace_all(txt, "%i", id.str());
	txt = replace_all(txt, "%rmin", smin.str());
	txt = replace_all(txt, "%rmax", smax.str());
	txt = replace_all(txt, "%r", r.str());

	return std::move(txt);
}
//--------------------------------------------------------------------------------
