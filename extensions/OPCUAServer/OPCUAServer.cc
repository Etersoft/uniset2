/*
 * Copyright (c) 2023 Pavel Vainerman.
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
extern "C" {
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
}
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include "Exceptions.h"
#include "OPCUAServer.h"
#include "unisetstd.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
// ( val, confval, default val )
static const std::string init3_str( const std::string& s1, const std::string& s2, const std::string& s3 )
{
    if( !s1.empty() )
        return s1;

    if( !s2.empty() )
        return s2;

    return s3;
}
// -----------------------------------------------------------------------------
OPCUAServer::OPCUAServer(uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                         const string& _prefix ):
    UObject_SK(objId, cnode, string(_prefix + "-")),
    prefix(_prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);

    UniXML::iterator it(cnode);

    auto ip = conf->getArgParam("--" + argprefix + "host", "0.0.0.0");
    auto port = conf->getArgPInt("--" + argprefix + "port", it.getProp("port"), 4840);
    auto browseName = it.getProp2("browseName", conf->oind->getMapName(conf->getLocalNode()));
    auto description = it.getProp2("description", browseName);

    opcServer = unisetstd::make_unique<opcua::Server>((uint16_t)port);
    opcServer->setApplicationName(it.getProp2("appName", "Uniset2 OPC UA Server"));
    opcServer->setApplicationUri(it.getProp2("appUri", "urn:uniset2.server"));
    opcServer->setProductUri(it.getProp2("productUri", "https://github.com/Etersoft/uniset2/"));
    updateTime_msec = conf->getArgPInt("--" + argprefix + "updatetime", it.getProp("updateTime"), (int)updateTime_msec);
    vmonit(updateTime_msec);
    myinfo << myname << "(init): OPC UA server " << ip << ":" << port << " updatePause=" << updateTime_msec << endl;

    opcServer->setLogger([this](auto level, auto category, auto msg)
    {
        mylog->level5() << myname
                        << "[" << opcua::getLogLevelName(level) << "] "
                        << "[" << opcua::getLogCategoryName(category) << "] "
                        << msg << std::endl;
    });

    auto opcConfig = UA_Server_getConfig(opcServer->handle());
    opcConfig->maxSubscriptions = conf->getArgPInt("--" + argprefix + "maxSubscriptions", it.getProp("maxSubscriptions"), (int)opcConfig->maxSubscriptions);
    opcConfig->maxSessions = conf->getArgPInt("--" + argprefix + "maxSessions", it.getProp("maxSessions"), (int)opcConfig->maxSessions);

    myinfo << myname << "(init): OPC UA server:"
           << " maxSessions=" << opcConfig->maxSessions
           << " maxSubscriptions=" << opcConfig->maxSubscriptions
           << endl;

    UA_LogLevel loglevel = UA_LOGLEVEL_ERROR;

    if( mylog->is_warn() )
        loglevel = UA_LOGLEVEL_WARNING;

    if( mylog->is_level5() )
        loglevel = UA_LOGLEVEL_INFO;

    if( mylog->is_level9() )
        loglevel = UA_LOGLEVEL_DEBUG;

    opcConfig->logger = UA_Log_Stdout_withLevel( loglevel );

    auto uroot = opcServer->getRootNode().addFolder(opcua::NodeId(0, "uniset"), "uniset");
    auto unode = uroot.addFolder(opcua::NodeId(0, browseName), browseName);
    uroot.writeDescription({"en", "uniset i/o"});
    unode.writeDescription({"ru-RU", description});
    unode.writeDisplayName({"en", browseName});

    ioNode = unisetstd::make_unique<IONode>(unode.addFolder(opcua::NodeId(0, "io"), "I/O"));
    ioNode->node.writeDescription({"en-US", "I/O"});
    opcServer->setCustomHostname(ip);

    serverThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::serverLoop);
    serverThread->setFinalAction(this, &OPCUAServer::serverLoopTerminate);
    updateThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::updateLoop);

    // определяем фильтр
    s_field = conf->getArg2Param("--" + argprefix + "filter-field", it.getProp("filterField"));
    s_fvalue = conf->getArg2Param("--" + argprefix + "filter-value", it.getProp("filterValue"));

    vmonit(s_field);
    vmonit(s_fvalue);

    myinfo << myname << "(init): read s_field='" << s_field
           << "' s_fvalue='" << s_fvalue << "'" << endl;

    if( !shm->isLocalwork() )
        ic->addReadItem(sigc::mem_fun(this, &OPCUAServer::readItem));
    else
        readConfiguration();

    // делаем очередь большой по умолчанию
    if( shm->isLocalwork() )
    {
        int sz = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 10000);

        if( sz > 0 )
            setMaxSizeOfMessageQueue(sz);
    }
}
// -----------------------------------------------------------------------------
OPCUAServer::~OPCUAServer()
{
}
//------------------------------------------------------------------------------
void OPCUAServer::readConfiguration()
{
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
        mywarn << myname << "(readConfiguration): section <sensors> empty?!!" << endl;
        return;
    }

    for( ; it; it++ )
    {
        if( uniset::check_filter(it, s_field, s_fvalue) )
            initVariable(it);
    }

    myinfo << myname << "(readConfiguration): init " << variables.size() << " variables" << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAServer::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
    if( uniset::check_filter(it, s_field, s_fvalue) )
        initVariable(it);

    return true;
}
//------------------------------------------------------------------------------
bool OPCUAServer::initVariable( UniXML::iterator& it )
{
    string sname = it.getProp("name");
    ObjectId sid = DefaultObjectId;

    if( it.getProp("id").empty() )
        sid = uniset_conf()->getSensorID(sname);
    else
        sid = it.getPIntProp("id", DefaultObjectId);

    if( sid == DefaultObjectId )
    {
        mycrit << myname << "(initVariable): (" << DefaultObjectId << ") Unknown Sensor ID for "
               << sname << endl;
        return false;
    }

    if( smTestID == DefaultObjectId )
        smTestID = sid;

    auto realIOType = uniset::getIOType(it.getProp("iotype"));

    if( realIOType == UniversalIO::UnknownIOType )
    {
        mycrit << myname << "(initVariable): Unknown iotype " << it.getProp("iotype")
               << " for " << sname << endl;
        return false;
    }

    UniversalIO::IOType iotype = UniversalIO::AO; // by default

    auto rwmode = it.getProp("opcua_rwmode");

    if( rwmode == "none" )
    {
        myinfo << myname << "(initVariable): rwmode='none'. Skip sensor " << sname << endl;
        return false;
    }

    if( rwmode == "w" )
    {
        if( realIOType == UniversalIO::DI || realIOType == UniversalIO::DO )
            iotype = UniversalIO::DI; // write access
        else
            iotype = UniversalIO::AI; // write access

        writeCount++;
    }
    else
    {
        if( realIOType == UniversalIO::DI || realIOType == UniversalIO::DO )
            iotype = UniversalIO::DO; // read access
        else
            iotype = UniversalIO::AO; // read access
    }

    auto vtype = it.getProp("opcua_type");
    opcua::Type opctype = DefaultVariableType;

    if( vtype == "bool" )
        opctype = opcua::Type::Boolean;
    else if( vtype == "float" )
        opctype = opcua::Type::Float;
    else if( iotype == UniversalIO::DI || iotype == UniversalIO::DO )
        opctype = opcua::Type::Boolean;

    uint8_t precision = (uint8_t)it.getIntProp("precision");

    DefaultValueUType mask = 0;
    uint8_t offset =  0;

    auto smask = it.getProp("opcua_mask");

    if( !smask.empty() )
    {
        mask = uni_atoi(smask);
        offset = firstBit(mask);
    }

    sname = it.getProp2("opcua_name", sname);

    auto vnode = ioNode->node.addVariable(opcua::NodeId(0, sname), sname);
    vnode.writeDataType(opctype);
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ);

    if( iotype == UniversalIO::AI || iotype == UniversalIO::DI )
        vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

    auto desc = it.getProp2("opcua_description", it.getProp("textname"));
    auto descLang = it.getProp2("opcua_description_lang", "ru");
    vnode.writeDescription({descLang, desc});

    auto displayName = it.getProp2("opcua_displayname", sname);
    auto displayNameLang = it.getProp2("opcua_displayname_lang", "en");
    vnode.writeDisplayName({displayNameLang, displayName});

    // init default value
    DefaultValueType defVal = (DefaultValueType)it.getPIntProp("default", 0);

    if( opctype == opcua::Type::Boolean )
    {
        bool set = defVal ? true : false;
        vnode.writeScalar(set);
    }
    else if( opctype == opcua::Type::Float )
    {
        float v = defVal;
        vnode.writeScalar(v);
    }
    else
        vnode.writeScalar(defVal);

    auto i = variables.emplace(sid, vnode);
    i.first->second.stype = iotype;
    i.first->second.mask = mask;
    i.first->second.offset = offset;
    i.first->second.vtype = opctype;
    i.first->second.precision = precision;

    shm->initIterator(i.first->second.it);
    return true;
}
//------------------------------------------------------------------------------
void OPCUAServer::callback() noexcept
{
    try
    {
        // вызываем базовую функцию callback вместо UObject_SK::callback!
        // (оптимизация, чтобы не делать лишних проверок не нужных в этом процессе)
        UniSetObject::callback();
    }
    catch( const std::exception& ex )
    {
        mycrit << myname << "(callback): catch " << ex.what()  <<   endl;
    }
}
// -----------------------------------------------------------------------------
CORBA::Boolean OPCUAServer::exist()
{
    bool ret = UObject_SK::exist();

    if( !ret )
        return false;

    return active;
}
//------------------------------------------------------------------------------
void OPCUAServer::sysCommand( const uniset::SystemMessage* sm )
{
    UObject_SK::sysCommand(sm);

    if( sm->command == SystemMessage::StartUp )
    {
        active  = true;
        myinfo << myname << "(sysCommand): init " << variables.size() << " variables" << endl;
        serverThread->start();
        updateThread->start();
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::serverLoopTerminate()
{
    opcServer->stop();
}
// -----------------------------------------------------------------------------
void OPCUAServer::serverLoop()
{
    if( opcServer == nullptr )
        return;

    PassiveTimer ptAct(activateTimeout);

    while( (!activated || !firstUpdate.load()) && !ptAct.checkTime() )
    {
        myinfo << myname << "(serverLoop): wait activate..." << endl;
        msleep(300);
    }

    if( !firstUpdate )
        mywarn << myname << "(serverLoop): first update [FAILED]..." << endl;

    myinfo << myname << "(serverLoop): started..." << endl;
    opcServer->run();
    myinfo << myname << "(serverLoop): terminated..." << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAServer::deactivateObject()
{
    activated = false;

    if( opcServer )
        opcServer->stop();

    if( updateThread )
    {
        if( updateThread->isRunning() )
            updateThread->join();
    }

    if( serverThread )
    {
        if( serverThread->isRunning() )
            serverThread->join();
    }

    return UObject_SK::deactivateObject();
}
// -----------------------------------------------------------------------------
void OPCUAServer::help_print()
{
    cout << "--opcua-name        - ID for rrdstorage. Default: OPCUAServer1. " << endl;
    cout << "--opcua-confnode    - configuration section name. Default: <NAME name='NAME'...> " << endl;
    cout << "--opcua-heartbeat-id name  - ID for heartbeat sensor." << endl;
    cout << "--opcua-heartbeat-max val  - max value for heartbeat sensor." << endl;
    cout << "--opcua-filter-field name  - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
    cout << "--opcua-filter-value val   - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
    cout << "--opcua-sm-ready-timeout   - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
    cout << "--opcua-sm-test-sid        - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;

    cout << endl;
    cout << "OPC UA:" << endl;
    cout << "--opcua-updatetime msec      - Пауза между обновлением информации в/из SM. По умолчанию 200" << endl;
    cout << "--opcua-host ip              - IP на котором слушает OPC UA сервер" << endl;
    cout << "--opcua-port port            - Порт на котором слушает OPC UA сервер" << endl;
    cout << "--opcua-maxSubscriptions num - Максимальное количество подписок" << endl;
    cout << "--opcua-maxSessions num      - Максимальное количество сессий" << endl;
    cout << endl;
    cout << "Logs:" << endl;
    cout << "--opcua-log-...            - log control" << endl;
    cout << "             add-levels ...  " << endl;
    cout << "             del-levels ...  " << endl;
    cout << "             set-levels ...  " << endl;
    cout << "             logfile filename" << endl;
    cout << "             no-debug " << endl;
    cout << "LogServer: " << endl;
    cout << "--opcua-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--opcua-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--opcua-logserver-port num - listen port. Default: ID" << endl;
    cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<OPCUAServer> OPCUAServer::init_opcua_server(int argc, const char* const* argv,
        uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "OPCUAServer");

    if( name.empty() )
    {
        cerr << "(OPCUAServer): Unknown name. Usage: --" <<  prefix << "-name" << endl;
        return nullptr;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset::DefaultObjectId )
    {
        cerr << "(OPCUAServer): Not found ID for '" << name
             << " in '" << conf->getObjectsSection() << "' section" << endl;
        return nullptr;
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", name);
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        cerr << "(OPCUAServer): " << name << "(init): Not found <" + confname + ">" << endl;
        return nullptr;
    }

    return make_shared<OPCUAServer>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void OPCUAServer::askSensors( UniversalIO::UIOCommand cmd )
{
    if( !shm->isLocalwork() )
        return;

    for(  auto it = variables.begin(); it != variables.end(); it++ )
    {
        try
        {
            if( it->second.stype == UniversalIO::AO || it->second.stype == UniversalIO::DO )
                shm->askSensor(it->first, cmd, getId());
        }
        catch( const uniset::Exception& ex )
        {
            mycrit << myname << "(askSensors): " << ex << endl;
        }
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::sensorInfo( const uniset::SensorMessage* sm )
{
    mylog4 << myname << "(sensorInfo): sm->id=" << sm->id << " val=" << sm->value << endl;
    auto it = variables.find(sm->id);

    if( it == variables.end() )
        return;

    try
    {
        if( it->second.stype == UniversalIO::DO || it->second.stype == UniversalIO::AO )
        {
            uniset::uniset_rwmutex_wrlock lock(it->second.vmut);
            it->second.state = sm->value ? true : false;
            it->second.value = getBits(sm->value, it->second.mask, it->second.offset);
        }
    }
    catch( std::exception& ex )
    {
        mycrit << myname << "(sensorInfo): " << ex.what() << endl;
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::updateLoop()
{
    myinfo << myname << "(updateLoop): started..." << endl;
    PassiveTimer ptAct(activateTimeout);

    while( !activated && !ptAct.checkTime() )
    {
        myinfo << myname << "(updateLoop): wait activate..." << endl;
        msleep(300);
    }

    firstUpdate = false;
    update();
    myinfo << myname << "(updateLoop): first update [ok].." << endl;
    firstUpdate = true;

    while( activated )
    {
        update();
        msleep(updateTime_msec);
    }

    myinfo << myname << "(updateLoop): terminated.." << endl;
}
// ------------------------------------------------------------------------------------------
uint8_t OPCUAServer::firstBit( OPCUAServer::DefaultValueUType mask )
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(mask);
#else
    uint8_t n = 0;

    while( mask != 0 )
    {
        if( mask & 1 )
            break;

        mask = (mask >> 1);
        n++;
    }

    return n;
#endif
}
// ------------------------------------------------------------------------------------------
OPCUAServer::DefaultValueUType OPCUAServer::forceSetBits(
    OPCUAServer::DefaultValueUType value,
    OPCUAServer::DefaultValueUType set,
    OPCUAServer::DefaultValueUType mask,
    uint8_t offset )
{
    if( mask == 0 )
        return set;

    return OPCUAServer::setBits(value, set, mask, offset);
}
// ------------------------------------------------------------------------------------------
OPCUAServer::DefaultValueUType OPCUAServer::setBits(
    OPCUAServer::DefaultValueUType value,
    OPCUAServer::DefaultValueUType set,
    OPCUAServer::DefaultValueUType mask,
    uint8_t offset )
{
    if( mask == 0 )
        return value;

    return (value & (~mask)) | ((set << offset) & mask);
}
// ------------------------------------------------------------------------------------------
OPCUAServer::DefaultValueUType OPCUAServer::getBits(
    OPCUAServer::DefaultValueUType value,
    OPCUAServer::DefaultValueUType mask,
    uint8_t offset )
{
    if( mask == 0 )
        return value;

    return (value & mask) >> offset;
}
// ------------------------------------------------------------------------------------------
void OPCUAServer::update()
{
    auto t_start = std::chrono::steady_clock::now();

    for( auto it = this->variables.begin(); it != this->variables.end(); it++ )
    {
        try
        {
            if( !this->shm->isLocalwork() )
            {
                if( it->second.stype == UniversalIO::DO )
                {
                    it->second.state = shm->localGetValue(it->second.it, it->first) ? true : false;
                    it->second.value = getBits(it->second.state ? 1 : 0, it->second.mask, it->second.offset);
                }
                else if( it->second.stype == UniversalIO::AO )
                {
                    it->second.value = getBits(shm->localGetValue(it->second.it, it->first), it->second.mask, it->second.offset);
                    it->second.state = it->second.value ? true : false;
                }
            }

            if( it->second.stype == UniversalIO::DO )
            {
                uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                it->second.node.writeScalar(it->second.state);
            }
            else if( it->second.stype == UniversalIO::AO )
            {
                if( it->second.vtype == opcua::Type::Float )
                {
                    uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                    float fval = (float)it->second.value / pow(10.0, it->second.precision);
                    it->second.node.writeScalar( fval );
                }
                else
                {
                    uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                    it->second.node.writeScalar(it->second.value);
                }
            }
            else if( it->second.stype == UniversalIO::DI )
            {
                uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                auto set = it->second.node.readScalar<bool>();
                auto val = getBits(set ? 1 : 0, it->second.mask, it->second.offset);
                mylog6 << this->myname << "(updateLoop): sid=" << it->first
                       << " set=" << set
                       << " mask=" << it->second.mask
                       << endl;
                this->shm->localSetValue(it->second.it, it->first, val, this->getId());
            }
            else if( it->second.stype == UniversalIO::AI )
            {
                uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                DefaultValueType val = 0;

                if( it->second.vtype == opcua::Type::Float )
                    val = lroundf( it->second.node.readScalar<float>() * pow(10.0, it->second.precision) );
                else
                    val = getBits(it->second.node.readScalar<DefaultValueType>(), it->second.mask, it->second.offset);

                mylog6 << this->myname << "(updateLoop): sid=" << it->first
                       << " value=" << val
                       << " mask=" << it->second.mask
                       << " precision=" << it->second.precision
                       << endl;

                this->shm->localSetValue(it->second.it, it->first, val, this->getId());
            }
        }
        catch( const std::exception& ex )
        {
            mycrit << this->myname << "(updateLoop): sid=" << it->first
                   << "[" << it->second.stype
                   << "][" << (int)it->second.vtype
                   << "] update error: " << ex.what() << endl;
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    mylog8 << myname << "(update): " << setw(10) << setprecision(7) << std::fixed
           << std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count() << " sec" << endl;
}
// -----------------------------------------------------------------------------
std::string OPCUAServer::getMonitInfo() const
{
    ostringstream inf;
    inf << "   iolist: " << variables.size() << endl;
    inf << "write(in): " << writeCount << endl;
    inf << "read(out): " << variables.size() - writeCount << endl;
    return inf.str();
}
// -----------------------------------------------------------------------------