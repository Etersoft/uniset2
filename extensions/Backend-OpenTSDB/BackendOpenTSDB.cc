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
#include <iomanip>
#include "Exceptions.h"
#include <Poco/Net/NetException.h>
#include "BackendOpenTSDB.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
BackendOpenTSDB::BackendOpenTSDB( uniset::ObjectId objId, xmlNode* cnode,
                                  uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                                  const string& prefix ):
    USingleProcess(cnode, uniset_conf()->getArgc(), uniset_conf()->getArgv(),""),
    UObject_SK(objId, cnode, string(prefix + "-")),
    prefix(prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);

    init(cnode);

    if( smTestID == DefaultObjectId && !tsdbParams.empty() )
    {
        // берём первый датчик из списка
        smTestID = tsdbParams.begin()->first;
    }
}
// -----------------------------------------------------------------------------
BackendOpenTSDB::~BackendOpenTSDB()
{
}
// -----------------------------------------------------------------------------
void BackendOpenTSDB::init( xmlNode* cnode )
{
    UniXML::iterator it(cnode);

    auto conf = uniset_conf();

    host = conf->getArg2Param("--" + prefix + "-host", it.getProp("host"), "localhost");
    port = conf->getArgPInt("--" + prefix + "-port", it.getProp("port"), port);
    tsdbMeasurement = conf->getArg2Param("--" + prefix + "-measurement", it.getProp("measurement"), "sensors");
    tsdbProject = conf->getArg2Param("--" + prefix + "-project", it.getProp("project"), "");
    tsdbTags = conf->getArg2Param("--" + prefix + "-tags", it.getProp("tags"), "");
    reconnectTime = conf->getArgPInt("--" + prefix + "-reconnect-time", it.getProp("reconnectTime"), reconnectTime);
    bufMaxSize = conf->getArgPInt("--" + prefix + "-buf-maxsize", it.getProp("bufMaxSize"), bufMaxSize);
    bufSize = conf->getArgPInt("--" + prefix + "-buf-size", it.getProp("bufMaxSize"), bufSize);
    bufSyncTime = conf->getArgPInt("--" + prefix + "-buf-sync-time", it.getProp("bufSyncTimeout"), bufSyncTime);

    int sz = conf->getArgPInt("--" + prefix + "-uniset-object-size-message-queue", it.getProp("sizeOfMessageQueue"), 10000);

    if( sz > 0 )
        setMaxSizeOfMessageQueue(sz);

    const string ff = conf->getArg2Param("--" + prefix + "-filter-field", it.getPropOrProp("filter_field", "filterField"), "" );
    const string fv = conf->getArg2Param("--" + prefix + "-filter-value", it.getPropOrProp("filter_value", "filterValue"), "" );

    myinfo << myname << "(init): influxdb host=" << host << ":" << port
           << " " << ff << "='" << fv << "'"
           << " measurement='" << tsdbMeasurement << "'"
           << " project='" << tsdbProject << "'"
           << " tags='" << tsdbTags << "'"
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

            std::string pname = it1.getProp2("tsdb_name", it1.getProp("name"));

            if( pname.empty() )
            {
                ostringstream err;
                err << myname << "(init): Unknown name sensor id = '" << it1.getProp("id") << "'";
                mycrit << err.str() << endl;
                throw SystemError(err.str());
            }

            ObjectId sid = conf->getSensorID( it1.getProp("name") );

            if( sid == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): Unknown SensorID for '" << it1.getProp("name") << "'";
                mycrit << err.str();
                throw SystemError(err.str());
            }

            tsdbParams.emplace( sid, ParamInfo(pname, it1.getProp("tsdb_tags")) );
        }

        if( tsdbParams.empty() )
        {
            ostringstream err;
            err << myname << "(init): Not found items for send to OpenTSDB...";
            mycrit << err.str() << endl;
            throw SystemError(err.str());
        }

    }
}
//--------------------------------------------------------------------------------
void BackendOpenTSDB::help_print( int argc, const char* const* argv )
{
    cout << " Default prefix='opentsdb'" << endl;
    cout << "--prefix-name                - ID. Default: BackendOpenTSDB." << endl;
    cout << "--prefix-confnode            - configuration section name. Default: <NAME name='NAME'...> " << endl;
    cout << "--run-lock file              - Запустить с защитой от повторного запуска" << endl;
    cout << endl;
    cout << " InfluxDB (Line Protocol): " << endl;
    cout << "--prefix-host  ip                         - InfluxDB: host. Default: localhost" << endl;
    cout << "--prefix-port  num                        - InfluxDB: port. Default: 4242" << endl;
    cout << "--prefix-measurement name                 - InfluxDB: measurement name. Default: sensors" << endl;
    cout << "--prefix-project name                     - InfluxDB: project tag value" << endl;
    cout << "--prefix-tags  'tag1=val1,tag2=val2...'   - InfluxDB: additional tags (comma-separated)" << endl;
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
    cout << "             logfile filename" << endl;
    cout << "             no-debug " << endl;
    cout << endl;
    cout << " LogServer: " << endl;
    cout << "--prefix-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--prefix-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--prefix-logserver-port num - listen port. Default: ID" << endl;
    cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<BackendOpenTSDB> BackendOpenTSDB::init_opendtsdb( int argc,
        const char* const* argv,
        uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "BackendOpenTSDB");
    ObjectId ID = conf->getDBServer();

    if( !name.empty() )
    {
        ID = conf->getServiceID(name);
        if( ID == uniset::DefaultObjectId )
        {
            dcrit << "(BackendOpenTSDB): Not found ServiceID for '" << name
                  << " in '" << conf->getServicesSection() << "' section" << endl;
            return nullptr;
        }
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", "LocalDBServer");
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        dcrit << "(BackendOpenTSDB): " << name << "(init): Not found <" + confname + ">" << endl;
        return 0;
    }

    dinfo << "(BackendOpenTSDB): name = " << name << "(" << ID << ")" << endl;
    return make_shared<BackendOpenTSDB>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void BackendOpenTSDB::callback() noexcept
{
    // используем стандартную "низкоуровневую" реализацию
    // т.к. она нас устраивает (обработка очереди сообщений и таймеров)
    UniSetObject::callback();
}
// -----------------------------------------------------------------------------
void BackendOpenTSDB::askSensors( UniversalIO::UIOCommand cmd )
{
    UObject_SK::askSensors(cmd);

    // прежде чем заказывать датчики, надо убедиться что SM доступна
    if( !waitSM(smReadyTimeout) )
    {
        uterminate();
        return;
    }

    for( const auto& s : tsdbParams )
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
void BackendOpenTSDB::sensorInfo( const uniset::SensorMessage* sm )
{
    auto it = tsdbParams.find(sm->id);

    if( it != tsdbParams.end() )
    {
        // если буфер заполнился, делаем попытку записи в БД
        if( buf.size() >= bufSize )
        {
            if ( !flushBuffer() )
            {
                // если размер буфера стал максимальный (теряем сообщения)
                if( buf.size() >= bufMaxSize && !flushBuffer() )
                {
                    mycrit << "buffer overflow. Lost data: sid=" << sm->id << " value=" << sm->value << endl;
                    return;
                }
            }
        }

        // InfluxDB Line Protocol:
        // <measurement>,<tag_key>=<tag_value>,... <field_key>=<field_value> <timestamp_ns>
        // Example: sensors,project=theatre,sensor=FC8_Fault_S value=0 1762405075021

        ostringstream s;

        s << tsdbMeasurement;

        // tags (comma-separated, no spaces)
        if( !tsdbProject.empty() )
            s << ",project=" << tsdbProject;

        s << ",sensor=" << it->second.name;

        if( !tsdbTags.empty() )
            s << "," << tsdbTags;

        if( !it->second.tags.empty() )
            s << "," << it->second.tags;

        // field value
        s << " value=" << sm->value;

        // timestamp in milliseconds
        s << " " << sm->sm_tv.tv_sec << setw(3) << setfill('0') << sm->sm_tv.tv_nsec / 1000000;

        s << "\n";

        buf.push_back(s.str());

        myinfo << myname << "(sensorInfo): " << s.str() << endl;

        if( !timerIsOn )
        {
            timerIsOn = true;
            askTimer(tmFlushBuffer, bufSyncTime, 1);
        }
    }
}
// -----------------------------------------------------------------------------
void BackendOpenTSDB::timerInfo( const uniset::TimerMessage* tm )
{
    if( tm->id == tmFlushBuffer )
        flushBuffer();
    else if( tm->id == tmReconnect )
    {
        if( reconnect() )
            askTimer(tmReconnect, 0);
    }
}
// -----------------------------------------------------------------------------
void BackendOpenTSDB::sysCommand(const SystemMessage* sm)
{
    if( sm->command == SystemMessage::StartUp )
    {
        if( !reconnect() )
            askTimer(tmReconnect, reconnectTime);
    }
}
// -----------------------------------------------------------------------------
bool BackendOpenTSDB::flushBuffer()
{
    if( buf.empty() )
    {
        if( timerIsOn )
        {
            askTimer(tmFlushBuffer, 0);
            timerIsOn = false;
        }

        return true;
    }

    if( !tcp || !tcp->isConnected() )
        return false;

    ostringstream q;

    for( const auto& s : buf )
        q << s;

    try
    {
        const string s(q.str());
        ssize_t ret = tcp->sendBytes(s.data(), s.size());

        if( ret < 0 )
        {
            int errnum = errno;

            if( errnum == EPIPE || errnum == EBADF )
            {
                mywarn << "(flushBuffer): send error (" << errnum << "): " << strerror(errnum) << endl;
                reconnect();
                return false;
            }
        }


        buf.clear();
        askTimer(tmFlushBuffer, 0);
        timerIsOn = false;

        if( !lastError.empty() )
            lastError = "";

        return true;
    }
    catch( Poco::IOException& ex )
    {
        mywarn << "(flushBuffer): (io): " << ex.displayText() << endl;
        lastError = ex.displayText();

        if( !reconnect() )
            askTimer(tmReconnect, reconnectTime);
    }
    catch( std::exception& ex )
    {
        mywarn << "(flushBuffer): (std): " << ex.what() << endl;
        lastError = ex.what();
    }

    return false;
}
//------------------------------------------------------------------------------
bool BackendOpenTSDB::reconnect()
{
    if( tcp )
    {
        tcp->forceDisconnect();
        tcp = nullptr;
    }

    try
    {
        myinfo << myname << "(reconnect): " << host << ":" << port << endl;

        tcp = make_shared<UTCPStream>();
        tcp->create(host, port, 500);
        //      tcp->setReceiveTimeout(UniSetTimer::millisecToPoco(replyTimeOut_ms));
        tcp->setKeepAlive(true); // tcp->setKeepAliveParams((replyTimeOut_ms > 1000 ? replyTimeOut_ms / 1000 : 1));
        tcp->setNoDelay(true);

        if( !lastError.empty() )
            lastError = "";

        return true;
    }
    catch( Poco::TimeoutException& ex)
    {
        mycrit << myname << "(connect): " << host << ":" << port << " timeout exception" << endl;
        lastError = "connect timeout";
    }
    catch( Poco::Net::NetException& ex)
    {
        mycrit << myname << "(connect): " << host << ":" << port << " error: " << ex.displayText() << endl;
        lastError = ex.displayText();
    }
    catch( const std::exception& e )
    {
        mycrit << myname << "(connect): " << host << ":" << port << " error: " << e.what() << endl;
        lastError = e.what();
    }
    catch( ... )
    {
        mycrit << myname << "(connect): " << host << ":" << port << " unknown exception..." << endl;
    }

    tcp = nullptr;
    return false;
}
//------------------------------------------------------------------------------
std::string BackendOpenTSDB::getMonitInfo() const
{
    ostringstream inf;

    inf << "Database: " << host << ":" << port
        << " ["
        << " reconnect=" << reconnectTime
        << " bufSyncTime=" << bufSyncTime
        << " bufSize=" << bufSize
        << " measurement: '" << tsdbMeasurement << "'"
        << " project: '" << tsdbProject << "'"
        << " tags: '" << tsdbTags << "'"
        << " ]" << endl
        << "  connection: " << ( tcp && tcp->isConnected() ? "OK" : "FAILED") << endl
        << " buffer size: " << buf.size() << endl
        << "   lastError: " << lastError << endl;

    return inf.str();
}
// -----------------------------------------------------------------------------
