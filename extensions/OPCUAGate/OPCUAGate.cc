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
}
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "OPCUAGate.h"
#include "unisetstd.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
OPCUAGate::OPCUAGate(uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                     const string& prefix ):
    UObject_SK(objId, cnode, string(prefix + "-")),
    prefix(prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);

    UniXML::iterator it(cnode);

    auto port = it.getPIntProp("port", 4840);
    opcServer = unisetstd::make_unique<opcua::Server>((uint16_t)port);
    opcServer->setApplicationName(it.getProp2("appName", "uniset2 OPC UA gate"));
    opcServer->setApplicationUri(it.getProp2("appUri", "urn:uniset2.server.gate"));
    opcServer->setProductUri(it.getProp2("productUri", "https://github.com/Etersoft/uniset2/"));
    updatePause_msec = it.getPIntProp("updatePause", updatePause_msec);

    opcServer->setLogger([this](auto level, auto category, auto msg)
    {
        mylog->level5()
                << "[" << opcua::getLogLevelName(level) << "] "
                << "[" << opcua::getLogCategoryName(category) << "] "
                << msg << std::endl;
    });

    auto opcConfig = opcServer->getConfig();
    opcConfig->shutdownDelay = it.getPIntProp("shutdownDelay", opcConfig->shutdownDelay);
    opcConfig->maxSubscriptions = it.getPIntProp("maxSubscriptions", opcConfig->maxSubscriptions);
    opcConfig->maxSessions = it.getPIntProp("maxSessions", opcConfig->maxSessions);
    ioNode = unisetstd::make_unique<IONode>(opcServer->getRootNode().addFolder(opcua::NodeId("io"), "I/O"));
    ioNode->node.setDescription("I/O", "en-US");

    auto localNode = conf->getLocalNode();
    auto nodes = conf->getXMLNodesSection();
    UniXML::iterator nIt = nodes;

    if( nIt.goChildren() )
    {
        for( ; nIt; nIt++ )
        {
            if( nIt.getIntProp("id") == localNode )
            {
                auto root = opcServer->getRootNode();
                root.setDescription(nIt.getProp("textname"), "ru-RU");
                root.setDisplayName(nIt.getProp("name"), "en-US");
                break;
            }
        }
    }

    serverThread = unisetstd::make_unique<ThreadCreator<OPCUAGate>>(this, &OPCUAGate::serverLoop);
    serverThread->setFinalAction(this, &OPCUAGate::serverLoopTerminate);
    updateThread = unisetstd::make_unique<ThreadCreator<OPCUAGate>>(this, &OPCUAGate::updateLoop);

    // определяем фильтр
    s_field = conf->getArgParam("--" + prefix + "-s-filter-field");
    s_fvalue = conf->getArgParam("--" + prefix + "-s-filter-value");

    vmonit(s_field);
    vmonit(s_fvalue);

    myinfo << myname << "(init): read s_field='" << s_field
           << "' s_fvalue='" << s_fvalue << "'" << endl;

    if( !shm->isLocalwork() )
        ic->addReadItem(sigc::mem_fun(this, &OPCUAGate::readItem));
    else
        readConfiguration();
}
// -----------------------------------------------------------------------------
OPCUAGate::~OPCUAGate()
{
}
//------------------------------------------------------------------------------
void OPCUAGate::readConfiguration()
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

    for( ; it.getCurrent(); it.goNext() )
    {
        if( uniset::check_filter(it, s_field, s_fvalue) )
            initVariable(it);
    }
}
// -----------------------------------------------------------------------------
bool OPCUAGate::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
    if( uniset::check_filter(it, s_field, s_fvalue) )
        initVariable(it);

    return true;
}
//------------------------------------------------------------------------------
bool OPCUAGate::initVariable( UniXML::iterator& it )
{
    string sname = it.getProp("name");
    ObjectId sid = DefaultObjectId;

    if( it.getProp("id").empty() )
        sid = uniset_conf()->getSensorID(sname);
    else
        sid = it.getPIntProp("id", DefaultObjectId);

    if( sid == DefaultObjectId )
    {
        mycrit << "(initVariable): (" << DefaultObjectId << ") Unknown Sensor ID for "
               << sname << endl;
        return false;
    }

    if( smTestID == DefaultObjectId )
        smTestID = sid;

    auto realIOType = uniset::getIOType(it.getProp("iotype"));

    if( realIOType == UniversalIO::UnknownIOType )
    {
        mycrit << "(initVariable): Unknown iotype " << it.getProp("iotype")
               << " for " << sname << endl;
        return false;
    }

    UniversalIO::IOType iotype = UniversalIO::AO; // by default

    auto rwmode = it.getProp("opcua_rwmode");

    if( rwmode == "none" )
    {
        myinfo << "(initVariable): rwmode='none'. Skip sensor " << sname << endl;
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

    opcua::Type opctype = opcua::Type::Int64;

    if( iotype == UniversalIO::DI || iotype == UniversalIO::DO )
        opctype = opcua::Type::Boolean;

    auto vnode = ioNode->node.addVariable(
                     opcua::NodeId(it.getProp("name")),
                     it.getProp("name"),
                     opctype);

    // init default value
    int64_t defVal = (int64_t)it.getPIntProp("default", 0);

    if( iotype == UniversalIO::AI || iotype == UniversalIO::AO )
        vnode.write(defVal);
    else
    {
        bool set = defVal ? true : false;
        vnode.write(set);
    }

    vnode.setDescription(it.getProp("textname"), "ru-RU");
    vnode.setDisplayName(it.getProp("name"), "en-US");

    auto i = variables.emplace(sid, vnode);
    i.first->second.stype = iotype;
    shm->initIterator(i.first->second.it);
    return true;
}
//------------------------------------------------------------------------------
void OPCUAGate::step()
{
}
//------------------------------------------------------------------------------
void OPCUAGate::sysCommand( const uniset::SystemMessage* sm )
{
    UObject_SK::sysCommand(sm);

    if( sm->command == SystemMessage::StartUp )
    {
        serverThread->start();
        updateThread->start();
    }
}
// -----------------------------------------------------------------------------
void OPCUAGate::serverLoopTerminate()
{
    opcServer->stop();
}
// -----------------------------------------------------------------------------
void OPCUAGate::serverLoop()
{
    if( opcServer == nullptr )
        return;

    opcServer->run();
    dinfo << myname << "(serverLoop): finished..." << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAGate::deactivateObject()
{
    active = false;

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
void OPCUAGate::help_print()
{
    cout << " Default prefix='rrd'" << endl;
    cout << "--prefix-name        - ID for rrdstorage. Default: OPCUAGate1. " << endl;
    cout << "--prefix-confnode    - configuration section name. Default: <NAME name='NAME'...> " << endl;
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
    cout << " LogServer: " << endl;
    cout << "--prefix-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--prefix-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--prefix-logserver-port num - listen port. Default: ID" << endl;
    cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<OPCUAGate> OPCUAGate::init_opcuagate(int argc, const char* const* argv,
        uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "OPCUAGate");

    if( name.empty() )
    {
        cerr << "(OPCUAGate): Unknown name. Usage: --" <<  prefix << "-name" << endl;
        return nullptr;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset::DefaultObjectId )
    {
        cerr << "(OPCUAGate): Not found ID for '" << name
             << " in '" << conf->getObjectsSection() << "' section" << endl;
        return nullptr;
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", name);
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        cerr << "(OPCUAGate): " << name << "(init): Not found <" + confname + ">" << endl;
        return nullptr;
    }

    return make_shared<OPCUAGate>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void OPCUAGate::askSensors( UniversalIO::UIOCommand cmd )
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
            mycrit << "(askSensors): " << ex << endl;
        }
    }
}
// -----------------------------------------------------------------------------
void OPCUAGate::sensorInfo( const uniset::SensorMessage* sm )
{
    mylog4 << "(sensorInfo): sm->id=" << sm->id << " val=" << sm->value << endl;
    auto it = variables.find(sm->id);

    if( it == variables.end() )
        return;

    try
    {
        if( sm->sensor_type == UniversalIO::DI )
            it->second.node.write(sm->value ? true : false);
        else if( sm->sensor_type == UniversalIO::AI )
            it->second.node.write(sm->value);
    }
    catch( std::exception& ex )
    {
        mycrit << "(sensorInfo): " << ex.what() << endl;
    }
}
// -----------------------------------------------------------------------------
void OPCUAGate::updateLoop()
{
    PassiveTimer ptAct(activateTimeout);

    while( !activated && !ptAct.checkTime() )
    {
        cout << myname << "(updateLoop): wait activate..." << endl;
        msleep(300);

        if( activated )
            break;
    }

    while( active )
    {
        for(  auto it = variables.begin(); it != variables.end(); it++ )
        {
            try
            {
                if( !shm->isLocalwork() )
                {
                    if( it->second.stype == UniversalIO::DO )
                        it->second.node.write(shm->localGetValue(it->second.it, it->first) ? true : false);
                    else if( it->second.stype == UniversalIO::AO )
                        it->second.node.write(shm->localGetValue(it->second.it, it->first));
                }

                if( it->second.stype == UniversalIO::DI )
                {
                    auto set = it->second.node.read<bool>();
                    mylog6 << "(updateLoop): sid=" << it->first << " set=" << set << endl;
                    shm->localSetValue(it->second.it, it->first, set ? 1 : 0, getId());
                }
                else if( it->second.stype == UniversalIO::AI )
                {
                    auto value = it->second.node.read<long>();
                    mylog6 << "(updateLoop): sid=" << it->first << " value=" << value << endl;
                    shm->localSetValue(it->second.it, it->first, value, getId());
                }
            }
            catch( const std::exception& ex )
            {
                mycrit << "(updateLoop): sid=" << it->first
                       << "[" << it->second.stype
                       << "] update error: " << ex.what() << endl;
            }
        }

        msleep(updatePause_msec);
    }

    myinfo << "(updateLoop): terminated.." << endl;
}
// -----------------------------------------------------------------------------