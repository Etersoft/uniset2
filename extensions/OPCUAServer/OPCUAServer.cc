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
#include <cmath>
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
                         const string& prefix ):
    UObject_SK(objId, cnode, string(prefix + "-")),
    prefix(prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);

    UniXML::iterator it(cnode);

    auto ip = conf->getArgParam("--" + argprefix + "host", "");
    auto port = conf->getArgPInt("--" + argprefix + "port", it.getProp("port"), 4840);

    opcServer = unisetstd::make_unique<opcua::Server>((uint16_t)port);
    opcServer->setApplicationName(it.getProp2("appName", "Uniset2 OPC UA Server"));
    opcServer->setApplicationUri(it.getProp2("appUri", "urn:uniset2.server"));
    opcServer->setProductUri(it.getProp2("productUri", "https://github.com/Etersoft/uniset2/"));
    updatePause_msec = conf->getArgPInt("--" + argprefix + "-updatepause", it.getProp("updatePause"), (int)updatePause_msec);
    myinfo << myname << "(init): OPC UA server " << ip << ":" << port << endl;

    opcServer->setLogger([this](auto level, auto category, auto msg)
    {
        mylog->level5() << myname
                        << "[" << opcua::getLogLevelName(level) << "] "
                        << "[" << opcua::getLogCategoryName(category) << "] "
                        << msg << std::endl;
    });

    auto opcConfig = opcServer->getConfig();
    opcConfig->maxSubscriptions = conf->getArgPInt("--" + argprefix + "maxSubscriptions", it.getProp("maxSubscriptions"), (int)opcConfig->maxSubscriptions);
    opcConfig->maxSessions = conf->getArgPInt("--" + argprefix + "maxSessions", it.getProp("maxSessions"), (int)opcConfig->maxSessions);

    myinfo << myname << "(init): OPC UA server: "
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

    auto uroot = opcServer->getRootNode().addFolder(opcua::NodeId("uniset"), "uniset");
    uroot.setDescription("uniset i/o", "en");

    auto localNode = conf->getLocalNode();
    auto nodes = conf->getXMLNodesSection();
    UniXML::iterator nIt = nodes;

    if( nIt.goChildren() )
    {
        for( ; nIt; nIt++ )
        {
            if( nIt.getIntProp("id") == localNode )
            {
                const auto uname = nIt.getProp("name");
                auto unode = uroot.addFolder(opcua::NodeId(uname), uname);
                unode.setDescription(nIt.getProp("textname"), "ru-RU");
                unode.setDisplayName(uname, "en");

                ioNode = unisetstd::make_unique<IONode>(unode.addFolder(opcua::NodeId("io"), "I/O"));
                ioNode->node.setDescription("I/O", "en-US");

                auto opcAddr = init3_str(ip, nIt.getProp2("opcua_ip", nIt.getProp("ip")), "127.0.0.1");
                opcServer->setCustomHostname(opcAddr);
                myinfo << myname << "(init): OPC UA address " << opcAddr << ":" << port << endl;
                break;
            }
        }
    }

    if( ioNode == nullptr )
        throw SystemError("not found localNode=" + std::to_string(localNode) + " in <nodes>");

    serverThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::serverLoop);
    serverThread->setFinalAction(this, &OPCUAServer::serverLoopTerminate);
    updateThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::updateLoop);

    // определяем фильтр
    s_field = conf->getArgParam("--" + prefix + "-s-filter-field");
    s_fvalue = conf->getArgParam("--" + prefix + "-s-filter-value");

    vmonit(s_field);
    vmonit(s_fvalue);

    myinfo << myname << "(init): read s_field='" << s_field
           << "' s_fvalue='" << s_fvalue << "'" << endl;

    if( !shm->isLocalwork() )
        ic->addReadItem(sigc::mem_fun(this, &OPCUAServer::readItem));
    else
        readConfiguration();
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
    }
    else
    {
        if( realIOType == UniversalIO::DI || realIOType == UniversalIO::DO )
            iotype = UniversalIO::DO; // read access
        else
            iotype = UniversalIO::AO; // read access
    }

    sname = it.getProp2("opcua_name", sname);

    opcua::Type opctype = DefaultVariableType;

    if( iotype == UniversalIO::DI || iotype == UniversalIO::DO )
        opctype = opcua::Type::Boolean;

    auto vnode = ioNode->node.addVariable(opcua::NodeId(sname), sname, opctype);
    vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ);

    if( iotype == UniversalIO::AI || iotype == UniversalIO::DI )
        vnode.setAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

    auto desc = it.getProp2("opcua_description", it.getProp("textname"));
    auto descLang = it.getProp2("opcua_description_lang", "ru");
    vnode.setDescription(desc, descLang);

    auto displayName = it.getProp2("opcua_displayname", sname);
    auto displayNameLang = it.getProp2("opcua_displayname_lang", "en");
    vnode.setDisplayName(displayName, displayNameLang);

    // init default value
    DefaultValueType defVal = (DefaultValueType)it.getPIntProp("default", 0);

    if( iotype == UniversalIO::AI || iotype == UniversalIO::AO )
        vnode.write(defVal);
    else
    {
        bool set = defVal ? true : false;
        vnode.write(set);
    }

    auto i = variables.emplace(sid, vnode);
    i.first->second.stype = iotype;
    shm->initIterator(i.first->second.it);
    return true;
}
//------------------------------------------------------------------------------
void OPCUAServer::step()
{
}
//------------------------------------------------------------------------------
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
    cout << "--opcua-heartbeat-id name   - ID for heartbeat sensor." << endl;
    cout << "--opcua-heartbeat-max val   - max value for heartbeat sensor." << endl;
    cout << endl;
    cout << "OPC UA:" << endl;
    cout << "--opcua-updatepause msec     - Пауза между обновлением информации в или из SM. По умолчанию 200" << endl;
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
    cout << "             logfile filanme " << endl;
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
        if( sm->sensor_type == UniversalIO::DO || sm->sensor_type == UniversalIO::DI )
            it->second.node.write(sm->value ? true : false);
        else if( sm->sensor_type == UniversalIO::AO || sm->sensor_type == UniversalIO::AI )
            it->second.node.write((DefaultValueType)sm->value);
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
        msleep(updatePause_msec);
    }

    myinfo << myname << "(updateLoop): terminated.." << endl;
}
// -----------------------------------------------------------------------------
void OPCUAServer::update()
{
    for( auto it = this->variables.begin(); it != this->variables.end(); it++ )
    {
        try
        {
            if( !this->shm->isLocalwork() )
            {
                if( it->second.stype == UniversalIO::DO )
                    it->second.node.write(shm->localGetValue(it->second.it, it->first) ? true : false);
                else if( it->second.stype == UniversalIO::AO )
                    it->second.node.write((DefaultValueType)shm->localGetValue(it->second.it, it->first));
            }

            if( it->second.stype == UniversalIO::DI )
            {
                auto set = it->second.node.read<bool>();
                mylog6 << this->myname << "(updateLoop): sid=" << it->first << " set=" << set << endl;
                this->shm->localSetValue(it->second.it, it->first, set ? 1 : 0, this->getId());
            }
            else if( it->second.stype == UniversalIO::AI )
            {
                auto value = it->second.node.read<DefaultValueType>();
                mylog6 << this->myname << "(updateLoop): sid=" << it->first << " value=" << value << endl;
                this->shm->localSetValue(it->second.it, it->first, value, this->getId());
            }
        }
        catch( const std::exception& ex )
        {
            mycrit << this->myname << "(updateLoop): sid=" << it->first
                   << "[" << it->second.stype
                   << "] update error: " << ex.what() << endl;
        }
    }
}
// -----------------------------------------------------------------------------