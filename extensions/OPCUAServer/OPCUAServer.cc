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
#include <open62541/plugin/nodestore.h>
}

#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include "Exceptions.h"
#include "OPCUAServer.h"
#include "unisetstd.h"
#include "IOController.h"
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
static const int namespaceIndex = 0;
// -----------------------------------------------------------------------------
OPCUAServer::OPCUAServer(uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                         const string& _prefix ):
    USingleProcess(cnode, uniset_conf()->getArgc(), uniset_conf()->getArgv(), ""),
    UObject_SK(objId, cnode, string(_prefix + "-")),
    prefix(_prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    logserv = make_shared<LogServer>(logAgregator());
    logserv->init(argprefix + "logserver", cnode);

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);

    UniXML::iterator it(cnode);

    if( findArgParam("--" + argprefix + "run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
    {
        logserv_host = conf->getArg2Param("--" + argprefix + "logserver-host", it.getProp("logserverHost"), "0.0.0.0");
        logserv_port = conf->getArgPInt("--" + argprefix + "logserver-port", it.getProp("logserverPort"), getId());
    }

    httpEnabledSetParams = conf->getArgPInt("--" + argprefix + "http-enabled-setparams", it.getProp("httpEnabledSetParams"), 0);
    auto ip = conf->getArgParam("--" + argprefix + "host", "0.0.0.0");
    auto port = conf->getArgPInt("--" + argprefix + "port", it.getProp("port"), 4840);
    auto browseName = it.getProp2("browseName", conf->oind->getMapName(conf->getLocalNode()));
    auto description = it.getProp2("description", browseName);
    propPrefix = conf->getArgParam("--" + argprefix + "prop-prefix", it.getProp("propPrefix"));

    opcua::ServerConfig opcconf((uint16_t)port);
    opcconf.setApplicationName(it.getProp2("appName", "Uniset2 OPC UA Server"));
    opcconf.setApplicationUri(it.getProp2("appUri", "urn:uniset2.server"));
    opcconf.setProductUri(it.getProp2("productUri", "https://github.com/Etersoft/uniset2/"));
    opcconf.setLogger([this](auto level, auto category, auto msg)
    {
        mylog->level5() << myname
                        << "[" << getLogLevelName(level) << "] "
                        << "[" << getLogCategoryName(category) << "] "
                        << msg << std::endl;
    });

    opcServer = opcua::Server{ std::move(opcconf) };
    namePrefix = it.getProp2("namePrefix", "");
    updateTime_msec = conf->getArgPInt("--" + argprefix + "updatetime", it.getProp("updateTime"), (int)updateTime_msec);
    vmonit(updateTime_msec);
    myinfo << myname << "(init): OPC UA server " << ip << ":" << port << " updatePause=" << updateTime_msec << endl;

    auto opcConfig = UA_Server_getConfig(opcServer.handle());
    opcConfig->maxSubscriptions = conf->getArgPInt("--" + argprefix + "maxSubscriptions", it.getProp("maxSubscriptions"), (int)opcConfig->maxSubscriptions);
    opcConfig->maxSessions = conf->getArgPInt("--" + argprefix + "maxSessions", it.getProp("maxSessions"), (int)opcConfig->maxSessions);
    opcConfig->maxSecureChannels = conf->getArgPInt("--" + argprefix + "maxSecureChannels", it.getProp("maxSecureChannels"), (int)opcConfig->maxSessions);
    opcConfig->maxSessionTimeout = conf->getArgPInt("--" + argprefix + "maxSessionTimeout", it.getProp("maxSessionTimeout"), 5000);
    opcConfig->maxSecurityTokenLifetime = conf->getArgPInt("--" + argprefix + "maxSecurityTokenLifetime", it.getProp("maxSecurityTokenLifetime"), (int)opcConfig->maxSecurityTokenLifetime);

    myinfo << myname << "(init): OPC UA server:"
           << " maxSessions=" << opcConfig->maxSessions
           << " maxSubscriptions=" << opcConfig->maxSubscriptions
           << " maxSecureChannels=" << opcConfig->maxSecureChannels
           << " maxSessionTimeout=" << opcConfig->maxSessionTimeout
           << " maxSecurityTokenLifetime=" << opcConfig->maxSecurityTokenLifetime
           << endl;

    UA_LogLevel loglevel = UA_LOGLEVEL_ERROR;

    if( mylog->is_warn() )
        loglevel = UA_LOGLEVEL_WARNING;

    if( mylog->is_level5() )
        loglevel = UA_LOGLEVEL_INFO;

    if( mylog->is_level9() )
        loglevel = UA_LOGLEVEL_DEBUG;

    // HACK (init loglevel)
    UA_ServerConfig* sconf = UA_Server_getConfig(opcServer.handle());
    auto slogger = UA_Log_Stdout_new( loglevel );
    sconf->logging = slogger;

    auto parentNode = opcua::Node{opcServer, opcua::ObjectId::ObjectsFolder};
    auto uroot = parentNode.addFolder(opcua::NodeId(namespaceIndex, "uniset"), "uniset");
    uroot.writeDescription({"en", "uniset i/o"});

    ioNode = std::make_unique<IONode>(std::move(uroot.addFolder(opcua::NodeId(namespaceIndex, browseName), browseName)));
    ioNode->node.writeDescription({"ru-RU", description});
    ioNode->node.writeDisplayName({"en", browseName});
    //    opcServer->setCustomHostname(ip);

    /* Инициализация каталога */
    UniXML::iterator tit = conf->findNode(cnode, "folders");

    if( tit )
        initFolderMap(tit, "", ioNode);

    serverThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::serverLoop);
    serverThread->setFinalAction(this, &OPCUAServer::serverLoopTerminate);
    updateThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::updateLoop);

    // определяем фильтр
    s_field = conf->getArg2Param("--" + argprefix + "filter-field", it.getPropOrProp("filter_field", "filterField"));
    s_fvalue = conf->getArg2Param("--" + argprefix + "filter-value", it.getPropOrProp("filter_value", "filterValue"));
    auto regexp_fvalue = conf->getArg2Param("--" + argprefix + "filter-value-re", it.getPropOrProp("filter_value_re", "filterValueRe"));

    if( !regexp_fvalue.empty() )
    {
        try
        {
            s_fvalue = regexp_fvalue;
            s_fvalue_re = std::regex(regexp_fvalue);
        }
        catch( const std::regex_error& e )
        {
            ostringstream err;
            err << myname << "(init): '--" + argprefix + "filter-value-re' regular expression error: " << e.what();
            throw uniset::SystemError(err.str());
        }
    }

    vmonit(s_field);
    vmonit(s_fvalue);
    vmonit(propPrefix);

    myinfo << myname << "(init): read s_field='" << s_field
           << "' s_fvalue='" << s_fvalue << "'"
           << " propPrefx='" << propPrefix << "'"
           << endl;

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
// -----------------------------------------------------------------------------
void OPCUAServer::initFolderMap( uniset::UniXML::iterator it, const std::string& parent_name, std::unique_ptr<IONode>& parent )
{
    if( !it.goChildren() )
    {
        return;
    }

    for( ; it.getCurrent(); it++ )
    {
        std::string cname = it.getProp("name");

        if( cname.empty() )
        {
            ostringstream err;
            err << myname << ": not found 'name' for folder <" << it.getName() << ">" << endl;
            throw uniset::SystemError(err.str());
        }

        std::string desc = it.getProp("description");

        auto folder_name = cname;

        if( !parent_name.empty() )
            folder_name = parent_name + "." + folder_name;

        try
        {
            auto ioNode = unisetstd::make_unique<IONode>(parent->node.addFolder(opcua::NodeId(namespaceIndex, folder_name), cname));
            ioNode->node.writeDescription({"en-US", desc});

            UniXML::iterator tit = it;
            initFolderMap( tit, folder_name, ioNode );

            myinfo << myname << "(initFolderMap): add new folder=" << folder_name << "(" << desc << ")" << endl;
            foldermap.emplace(folder_name, std::move(ioNode));
        }
        catch( const opcua::BadStatus& status )
        {
            ostringstream err;
            err << myname << ": catch <" << status.what() << "> on " << folder_name << endl;
            throw uniset::SystemError(err.str());
        }
    }
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
        if( s_fvalue_re.has_value()  )
        {
            if( uniset::check_filter_re(it, s_field, *s_fvalue_re) )
                initVariable(it);
        }
        else if( uniset::check_filter(it, s_field, s_fvalue) )
            initVariable(it);
    }

    myinfo << myname << "(readConfiguration): init " << variables.size() << " variables" << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAServer::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
    if( s_fvalue_re.has_value()  )
    {
        if( uniset::check_filter_re(it, s_field, *s_fvalue_re) )
            initVariable(it);
    }
    else if( uniset::check_filter(it, s_field, s_fvalue) )
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

    auto rwmode = it.getProp(propPrefix + "opcua_rwmode");

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

    auto vtype = it.getProp(propPrefix + "opcua_type");
    opcua::DataTypeId opctype = DefaultVariableType;

    if( vtype == "bool" )
        opctype = opcua::DataTypeId::Boolean;
    else if( vtype == "float" )
        opctype = opcua::DataTypeId::Float;
    else if( iotype == UniversalIO::DI || iotype == UniversalIO::DO )
        opctype = opcua::DataTypeId::Boolean;

    uint8_t precision = (uint8_t)it.getIntProp(propPrefix + "precision");

    DefaultValueUType mask = 0;
    uint8_t offset =  0;

    auto smask = it.getProp(propPrefix + "opcua_mask");

    if( !smask.empty() )
    {
        mask = uni_atoi(smask);
        offset = firstBit(mask);
    }

    sname = namePrefix + it.getProp2(propPrefix + "opcua_name", sname);

    auto folder = it.getProp(propPrefix + "opcua_folder");

    uniset::OPCUAServer::IONode* node = nullptr;

    if( !folder.empty() )
    {
        if( foldermap.find(folder) == foldermap.end() )
        {
            ostringstream err;
            err << myname << "(initVariable): wrong opcua_folder for sensor " << sname << endl;
            throw SystemError(err.str());
        }

        node = foldermap[folder].get();
    }
    else
    {
        myinfo << myname << "(initVariable): opcua_folder not found. Using root folder for sensor " << sname << endl;
        node = ioNode.get();
    }

    if(!node)
    {
        ostringstream err;
        err << myname << "(initVariable): can't get node?!";
        throw SystemError(err.str());
    }

    auto desc = it.getProp2(propPrefix + "opcua_description", it.getProp("textname"));
    auto descLang = it.getProp2(propPrefix + "opcua_description_lang", "ru");

    auto displayName = it.getProp2(propPrefix + "opcua_displayname", sname);
    auto displayNameLang = it.getProp2(propPrefix + "opcua_displayname_lang", "en");

    auto method = it.getProp(propPrefix + "opcua_method");

    if(!method.empty())
    {
        if( iotype == UniversalIO::DO || iotype == UniversalIO::AO )
        {
            ostringstream err;
            err << myname << "(initVariable): wrong rwmode='r' for method sensor " << sname << endl;
            throw SystemError(err.str());
        }

        //Инициализация метода
        opcua::NodeId methodNodeId = opcua::NodeId(namespaceIndex, sname);

        UA_MethodAttributes attr = UA_MethodAttributes_default;
        attr.description = UA_LOCALIZEDTEXT_ALLOC(descLang.c_str(), desc.c_str());
        attr.displayName = UA_LOCALIZEDTEXT_ALLOC(displayNameLang.c_str(), displayName.c_str());
        attr.executable = true;

        UA_QualifiedName methodBrowseName = UA_QUALIFIEDNAME_ALLOC(methodNodeId.namespaceIndex(), sname.c_str());

        UA_Argument inputArguments;
        UA_Argument_init(&inputArguments);
        inputArguments.valueRank = UA_VALUERANK_SCALAR;

        if( iotype == UniversalIO::DI )
        {
            inputArguments.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
            inputArguments.description = UA_LOCALIZEDTEXT("en-US", "state");
            inputArguments.name = UA_STRING("state");
        }
        else if( iotype == UniversalIO::AI )
        {
            if( opctype == opcua::DataTypeId::Float )
            {
                inputArguments.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
                inputArguments.description = UA_LOCALIZEDTEXT("en-US", "float value");
                inputArguments.name = UA_STRING("float value");
            }
            else
            {
                inputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
                inputArguments.description = UA_LOCALIZEDTEXT("en-US", "value");
                inputArguments.name = UA_STRING("value");
            }
        }

        UA_StatusCode result = UA_Server_addMethodNode(opcServer.handle(),
                               *methodNodeId.handle(), //requestedNewNodeId
                               *node->node.id().handle(),//parentNodeId
                               UA_NODEID_NUMERIC(namespaceIndex, UA_NS0ID_HASCOMPONENT),//referenceTypeId
                               methodBrowseName,
                               attr,
                               &UA_setValueMethod,
                               1, &inputArguments,
                               0, nullptr,
                               this, nullptr);

        if (result == UA_STATUSCODE_GOOD)
        {
            auto i = methods.emplace(methodNodeId.hash(), sid);
            i.first->second.precision = precision;//only for float value
            methodCount++;
            shm->initIterator(i.first->second.it);
            myinfo << myname << "(initVariable): init method for " << sname << "(" << sid << ")" << endl;
        }

        UA_NodeId_clear(methodNodeId.handle());
        UA_MethodAttributes_clear(&attr);
        UA_QualifiedName_clear(&methodBrowseName);

        if (result != UA_STATUSCODE_GOOD)
        {
            mycrit << myname << "(initVariable): can't init method for " << sname << "(" << sid << ")" << endl;
            return false;
        }

        return true;
    }

    // s=xxx or ns=xxx
    if( sname[1] != '=' && sname[2] != '=' )
        sname = "s=" + sname;

    auto vnode = node->node.addVariable(UA_NODEID(sname.c_str()), sname);
    vnode.writeDataType(opctype);
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ);

    if( iotype == UniversalIO::AI || iotype == UniversalIO::DI )
        vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

    vnode.writeDescription({descLang, desc});
    vnode.writeDisplayName({displayNameLang, displayName});

    // init default value
    DefaultValueType defVal = (DefaultValueType)it.getPIntProp("default", 0);

    if( opctype == opcua::DataTypeId::Boolean )
    {
        bool set = defVal ? true : false;
        vnode.writeValue(opcua::Variant{set});
    }
    else if( opctype == opcua::DataTypeId::Float )
    {
        float v = defVal;
        vnode.writeValue(opcua::Variant{v});
    }
    else
        vnode.writeValue(opcua::Variant{defVal});

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
        if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
        {
            myinfo << myname << "(sysCommand): run log server " << logserv_host << ":" << logserv_port << endl;
            logserv->async_run(logserv_host, logserv_port);
        }

        active  = true;
        myinfo << myname << "(sysCommand): init " << variables.size() << " variables" << endl;
        serverThread->start();
        updateThread->start();
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::serverLoopTerminate()
{
    opcServer.stop();
}
// -----------------------------------------------------------------------------
void OPCUAServer::serverLoop()
{
    if( opcServer.isRunning() )
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
    opcServer.run();
    myinfo << myname << "(serverLoop): terminated..." << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAServer::deactivateObject()
{
    activated = false;

    if( opcServer.isRunning() )
        opcServer.stop();

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
    cout << "--run-lock file            - Запустить с защитой от повторного запуска" << endl;
    cout << "--opcua-name               - ID for rrdstorage. Default: OPCUAServer1. " << endl;
    cout << "--opcua-confnode           - configuration section name. Default: <NAME name='NAME'...> " << endl;
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
    cout << " HTTP API: " << endl;
    cout << "--opcua-http-enabled-setparams 1 - Enable API /setparams" << endl;
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
    cout << LogServer::help_print("opcua-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<OPCUAServer> OPCUAServer::init_opcua_server(int argc, const char* const* argv,
        uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    // Use passed argc/argv to get parameters
    string name = uniset::getArgParam("--" + prefix + "-name", argc, argv, "OPCUAServer");

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

    string confname = uniset::getArgParam("--" + prefix + "-confnode", argc, argv, name);
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
                    it->second.state = shm->localGetValue(it->second.it, it->first) != 0;
                    it->second.value = getBits(it->second.state ? 1 : 0, it->second.mask, it->second.offset);
                }
                else if( it->second.stype == UniversalIO::AO )
                {
                    it->second.value = getBits(shm->localGetValue(it->second.it, it->first), it->second.mask, it->second.offset);
                    it->second.state = it->second.value != 0;
                }
            }

            if( it->second.stype == UniversalIO::DO )
            {
                uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                it->second.node.writeValue(opcua::Variant{it->second.state});
            }
            else if( it->second.stype == UniversalIO::AO )
            {
                if( it->second.vtype == opcua::DataTypeId::Float )
                {
                    uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                    float fval = (float)it->second.value / pow(10.0, it->second.precision);
                    it->second.node.writeValue(opcua::Variant{ fval} );
                }
                else
                {
                    uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                    it->second.node.writeValue(opcua::Variant{it->second.value});
                }
            }
            else if( it->second.stype == UniversalIO::DI )
            {
                uniset::uniset_rwmutex_rlock lock(it->second.vmut);
                auto set = it->second.node.readValue().to<bool>();
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

                if( it->second.vtype == opcua::DataTypeId::Float )
                    val = lroundf( it->second.node.readValue().to<float>() * pow(10.0, it->second.precision) );
                else
                    val = getBits(it->second.node.readValue().to<DefaultValueType>(), it->second.mask, it->second.offset);

                mylog6 << this->myname << "(updateLoop): sid=" << it->first
                       << " value=" << val
                       << " mask=" << it->second.mask
                       << " precision=" << it->second.precision
                       << endl;

                this->shm->localSetValue(it->second.it, it->first, val, this->getId());
            }
        }
        catch( const IOController_i::NameNotFound& ex )
        {
            mycrit << this->myname << "(updateLoop): sid=" << it->first
                   << " IOController_i::NameNotFound: " << ex.err << endl;
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
    inf << "methods(call): " << methodCount << endl;
    return inf.str();
}
// -----------------------------------------------------------------------------
UA_StatusCode OPCUAServer::UA_setValueMethod(UA_Server* server,
        const UA_NodeId* sessionId, void* sessionHandle,
        const UA_NodeId* methodId, void* methodContext,
        const UA_NodeId* objectId, void* objectContext,
        size_t inputSize, const UA_Variant* input,
        size_t outputSize, UA_Variant* output)
{
    if(inputSize != 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    OPCUAServer* srv = static_cast<OPCUAServer*>(methodContext);

    if(!srv)
        return UA_STATUSCODE_BADINTERNALERROR;

    auto it = srv->methods.find(UA_NodeId_hash(methodId));

    if( it == srv->methods.end() )
        return UA_STATUSCODE_BADMETHODINVALID;

    bool ret = false;
    DefaultValueType value = 0;

    // Корректность типов проверяется на уровне libopen62541(смотри описание opcua_method в OPCUAServer.h) перед вызовом обработчика UA_setValueMethod.
    if( input[0].type == &UA_TYPES[UA_TYPES_INT32] )
    {
        value = *(UA_Int32*) input[0].data;
    }
    else if( input[0].type == &UA_TYPES[UA_TYPES_BOOLEAN] )
    {
        value = *(UA_Boolean*) input[0].data ? 1 : 0;
    }
    else if( input[0].type == &UA_TYPES[UA_TYPES_FLOAT] )
    {
        value = lroundf( ( *(UA_Float*) input[0].data ) * pow(10.0, it->second.precision) );
    }
    else
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(srv->log()->is_level6())
        srv->log()->level6() << srv->myname << "(update): " << value
                             << " nodeid: " << std::string_view((char*)methodId->identifier.string.data, methodId->identifier.string.length)
                             << " sid: "    << it->second.sid << endl;

    try
    {
        srv->shm->localSetValue(it->second.it, it->second.sid, value, srv->getId());
        ret = true;
    }
    catch( const IOController_i::NameNotFound& ex )
    {
        if(srv->log()->is_crit())
            srv->log()->crit() << srv->myname << "(UA_setValueMethod): sid=" << it->second.sid
                               << " IOController_i::NameNotFound: " << ex.err << std::endl;
    }
    catch( const std::exception& ex )
    {
        if(srv->log()->is_crit())
            srv->log()->crit() << srv->myname << "(UA_setValueMethod): (hb) " << ex.what() << std::endl;
    }


    if ( ret )
        return UA_STATUSCODE_GOOD;

    return UA_STATUSCODE_BAD;
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
Poco::JSON::Object::Ptr OPCUAServer::buildLogServerInfo()
{
    return LogServer::httpLogServerInfo(logserv, logserv_host, logserv_port);
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpRequest( const UHttp::HttpRequestContext& ctx )
{
    if( ctx.depth() > 0 )
    {
        const std::string& req = ctx[0];

        if( req == "getparam" )
            return httpGetParam(ctx.params);

        if( req == "setparam" )
            return httpSetParam(ctx.params);

        if( req == "status" )
            return httpStatus();

        if( req == "sensors" )
            return httpSensors(ctx.params);

        if( req == "get" )
            return httpGet(ctx.params);
    }

    auto json = UObject_SK::httpRequest(ctx);

    // Если запрос к корню объекта (depth==0), добавляем информацию о LogServer
    if( ctx.depth() == 0 )
    {
        if( !json )
            json = new Poco::JSON::Object();

        Poco::JSON::Object::Ptr jdata = json->getObject(myname);

        if( !jdata )
        {
            jdata = new Poco::JSON::Object();
            json->set(myname, jdata);
        }

        jdata->set("LogServer", buildLogServerInfo());
    }

    return json;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpHelp( const Poco::URI::QueryParameters& p )
{
    uniset::json::help::object myhelp(myname, UObject_SK::httpHelp(p));

    {
        uniset::json::help::item cmd("status", "get current OPC UA server status (JSON)");
        myhelp.add(cmd);
    }
    {
        uniset::json::help::item cmd("getparam", "read runtime parameters");
        cmd.param("name", "parameter to read; can be repeated");
        cmd.param("note", "supported: updateTime_msec|variablesCount|writeCount|methodCount|httpEnabledSetParams");
        myhelp.add(cmd);
    }
    {
        uniset::json::help::item cmd("setparam", "set runtime parameters");
        cmd.param("updateTime_msec", "milliseconds");
        cmd.param("note", "may be disabled by httpEnabledSetParams");
        myhelp.add(cmd);
    }
    {
        uniset::json::help::item cmd("sensors", "list OPC UA variables with pagination");
        cmd.param("limit", "max number of sensors to return (default: 50, 0=unlimited)");
        cmd.param("offset", "skip first N sensors (for pagination)");
        cmd.param("search", "filter by name (case-insensitive substring match)");
        cmd.param("iotype", "filter by type: AI|AO|DI|DO");
        cmd.param("filter", "filter by sensor names/ids (comma-separated)");
        myhelp.add(cmd);
    }
    {
        uniset::json::help::item cmd("get", "get values by sensor names or ids");
        cmd.param("name", "sensor name(s), comma-separated; can be repeated");
        cmd.param("id", "sensor id(s), comma-separated; can be repeated");
        myhelp.add(cmd);
    }

    return myhelp;
}
// -----------------------------------------------------------------------------
namespace
{
    inline long to_long(const std::string& s, const std::string& what, const std::string& myname)
    {
        try
        {
            size_t pos = 0;
            long v = std::stol(s, &pos, 10);

            if( pos != s.size() ) throw std::invalid_argument("garbage");

            return v;
        }
        catch(...)
        {
            throw uniset::SystemError(myname + "(/setparam): invalid value for '" + what + "': '" + s + "'");
        }
    }

    inline std::string UAStringToStdString( const UA_String& uas )
    {
        if (uas.length == 0 || uas.data == nullptr)
            return std::string();

        return std::string(reinterpret_cast<const char*>(uas.data), uas.length);
    }
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpGetParam( const Poco::URI::QueryParameters& p )
{
    if( p.empty() )
        throw uniset::SystemError(myname + "(/getparam): pass at least one 'name' parameter");

    std::vector<std::string> names;
    names.reserve(p.size());

    for( const auto& kv : p )
        if( kv.first == "name" && !kv.second.empty() )
            names.push_back(kv.second);

    if( names.empty() )
        throw uniset::SystemError(myname + "(/getparam): parameter 'name' is required (can be repeated)");

    using Poco::JSON::Object;
    using Poco::JSON::Array;

    Object::Ptr out = new Object();
    Object::Ptr params = new Object();
    Array::Ptr unknown = new Array();

    for( const auto& n : names )
    {
        if( n == "updateTime_msec" )
            params->set(n, (int)updateTime_msec);
        else if( n == "variablesCount" )
            params->set(n, (int)variables.size());
        else if( n == "writeCount" )
            params->set(n, (int)writeCount);
        else if( n == "methodCount" )
            params->set(n, (int)methodCount);
        else if( n == "httpEnabledSetParams" )
            params->set(n, httpEnabledSetParams ? 1 : 0);
        else
            unknown->add(n);
    }

    out->set("result", "OK");
    out->set("params", params);

    if( unknown->size() > 0 )
        out->set("unknown", unknown);

    return out;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpSetParam( const Poco::URI::QueryParameters& p )
{
    if( p.empty() )
        throw uniset::SystemError(myname + "(/setparam): pass key=value pairs");

    if( !httpEnabledSetParams )
        throw uniset::SystemError(myname + "(/setparam): disabled by admin");

    using Poco::JSON::Object;
    using Poco::JSON::Array;

    Object::Ptr out = new Object();
    Object::Ptr updated = new Object();
    Array::Ptr unknown = new Array();

    for( const auto& kv : p )
    {
        const std::string& name = kv.first;
        const std::string& val  = kv.second;

        if( name == "updateTime_msec" )
        {
            long v = to_long(val, name, myname);

            if( v < 0 )
                throw uniset::SystemError(myname + "(/setparam): value must be >= 0 (" + name + ")");

            updateTime_msec = (timeout_t)v;
            updated->set(name, (int)updateTime_msec);
        }
        else
        {
            unknown->add(name);
        }
    }

    out->set("result", "OK");
    out->set("updated", updated);

    if( unknown->size() > 0 )
        out->set("unknown", unknown);

    return out;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpStatus()
{
    using Poco::JSON::Object;
    using Poco::JSON::Array;

    Object::Ptr st = new Object();
    st->set("name", myname);
    st->set("extensionType", "OPCUAServer");
    st->set("LogServer", buildLogServerInfo());

    auto opcConfig = UA_Server_getConfig(opcServer.handle());

    // endpoints
    {
        Array::Ptr eps = new Array();

        for( size_t i = 0; i < opcConfig->endpointsSize; i++ )
        {
            Object::Ptr ep = new Object();
            ep->set("url",  UAStringToStdString(opcConfig->endpoints[i].server.applicationUri));
            ep->set("name",  UAStringToStdString(opcConfig->endpoints[i].server.applicationName.text));
            eps->add(ep);
        }

        st->set("endpoints", eps);
    }

    // runtime params
    {
        Object::Ptr rp = new Object();
        rp->set("updateTime_msec",  (int)updateTime_msec);
        st->set("params", rp);
    }

    // config
    {
        Object::Ptr cfg = new Object();
        cfg->set("maxSubscriptions", (int)opcConfig->maxSubscriptions);
        cfg->set("maxSessions", (int)opcConfig->maxSessions);
        cfg->set("maxSecureChannels", (int)opcConfig->maxSecureChannels);
        cfg->set("maxSessionTimeout", (int)opcConfig->maxSessionTimeout);
        st->set("config", cfg);
    }

    // variables info
    {
        Object::Ptr vars = new Object();
        vars->set("total", (int)variables.size());
        vars->set("read", (int)(variables.size() - writeCount));
        vars->set("write", (int)writeCount);
        vars->set("methods", (int)methodCount);
        st->set("variables", vars);
    }

    st->set("httpEnabledSetParams", httpEnabledSetParams ? 1 : 0);

    Object::Ptr out = new Object();
    out->set("result", "OK");
    out->set("status", st);
    return out;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpGetMyInfo( Poco::JSON::Object::Ptr root )
{
    auto my = UObject_SK::httpGetMyInfo(root);
    my->set("extensionType", "OPCUAServer");
    return my;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpSensors( const Poco::URI::QueryParameters& p )
{
    using Poco::JSON::Object;
    using Poco::JSON::Array;

    int limit = 50;
    int offset = 0;
    std::string search;
    UniversalIO::IOType iotypeFilter = UniversalIO::UnknownIOType;
    std::unordered_set<uniset::ObjectId> filterIds;

    auto conf = uniset_conf();

    for( const auto& kv : p )
    {
        if( kv.first == "limit" && !kv.second.empty() )
            limit = std::stoi(kv.second);
        else if( kv.first == "offset" && !kv.second.empty() )
            offset = std::stoi(kv.second);
        else if( kv.first == "search" && !kv.second.empty() )
            search = kv.second;
        else if( kv.first == "iotype" && !kv.second.empty() )
            iotypeFilter = uniset::getIOType(kv.second);
        else if( kv.first == "filter" && !kv.second.empty() )
        {
            try
            {
                auto slist = uniset::getSInfoList(kv.second, conf);
                filterIds.reserve(slist.size());

                for( const auto& s : slist )
                    filterIds.insert(s.si.id);
            }
            catch( const std::exception& ex )
            {
                // Invalid filter - ignore
            }
        }
    }

    // For backward compatibility: if search is AI/AO/DI/DO, treat as iotype
    if( iotypeFilter == UniversalIO::UnknownIOType && !search.empty() )
    {
        auto t = uniset::getIOType(search);

        if( t != UniversalIO::UnknownIOType )
        {
            iotypeFilter = t;
            search.clear();
        }
    }

    Object::Ptr out = new Object();
    Array::Ptr sensors = new Array();

    int total = 0;
    int added = 0;
    int skipped = 0;

    for( const auto& var : variables )
    {
        // Apply filter by id/name (if specified)
        if( !filterIds.empty() )
        {
            if( filterIds.find(var.first) == filterIds.end() )
                continue;
        }

        // Apply iotype filter
        if( iotypeFilter != UniversalIO::UnknownIOType && var.second.stype != iotypeFilter )
            continue;

        // Get sensor name
        std::string sensorName = conf->oind->getShortName(var.first);

        // Apply text search (case-insensitive substring match)
        if( !search.empty() )
        {
            if( !uniset::containsIgnoreCase(sensorName, search) )
                continue;
        }

        total++;

        // Apply offset
        if( skipped < offset )
        {
            skipped++;
            continue;
        }

        // Apply limit (0 = no limit)
        if( limit > 0 && added >= limit )
            continue;

        // Build sensor JSON
        Object::Ptr js = new Object();
        js->set("id", (int)var.first);
        js->set("name", sensorName);
        js->set("iotype", uniset::iotype2str(var.second.stype));

        // Read current value (thread-safe)
        {
            uniset::uniset_rwmutex_rlock lock(const_cast<IOVariable&>(var.second).vmut);

            if( var.second.vtype == opcua::DataTypeId::Boolean )
                js->set("value", var.second.state ? 1 : 0);
            else if( var.second.vtype == opcua::DataTypeId::Float )
            {
                float fval = var.second.precision > 0
                             ? (float)var.second.value / std::pow(10.0f, var.second.precision)
                             : (float)var.second.value;
                js->set("value", fval);
            }
            else
                js->set("value", (int)var.second.value);
        }

        // OPC UA specific info
        js->set("vtype", (int)var.second.vtype);

        if( var.second.mask != 0 )
            js->set("mask", (int)var.second.mask);

        if( var.second.precision > 0 )
            js->set("precision", (int)var.second.precision);

        sensors->add(js);
        added++;
    }

    out->set("result", "OK");
    out->set("sensors", sensors);
    out->set("total", total);
    out->set("limit", limit);
    out->set("offset", offset);

    return out;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr OPCUAServer::httpGet( const Poco::URI::QueryParameters& p )
{
    using Poco::JSON::Object;
    using Poco::JSON::Array;

    auto conf = uniset_conf();

    // Collect requested sensor names/ids
    std::vector<std::pair<uniset::ObjectId, std::string>> requestedSensors;
    std::vector<std::string> notFoundNames;

    for( const auto& kv : p )
    {
        if( (kv.first == "name" || kv.first == "id") && !kv.second.empty() )
        {
            auto slist = uniset::getSInfoList(kv.second, conf);

            if( slist.empty() )
                notFoundNames.push_back(kv.second);
            else
            {
                for( const auto& s : slist )
                    requestedSensors.emplace_back(s.si.id, s.fname);
            }
        }
    }

    if( requestedSensors.empty() && notFoundNames.empty() )
        throw uniset::SystemError(myname + "(/get): specify at least one 'name' or 'id' parameter");

    Object::Ptr out = new Object();
    Array::Ptr sensors = new Array();

    for( const auto& req : requestedSensors )
    {
        Object::Ptr js = new Object();
        js->set("name", req.second);
        js->set("id", (int)req.first);

        auto it = variables.find(req.first);

        if( it != variables.end() )
        {
            js->set("iotype", uniset::iotype2str(it->second.stype));

            // Read current value (thread-safe)
            {
                uniset::uniset_rwmutex_rlock lock(const_cast<IOVariable&>(it->second).vmut);

                if( it->second.vtype == opcua::DataTypeId::Boolean )
                    js->set("value", it->second.state ? 1 : 0);
                else if( it->second.vtype == opcua::DataTypeId::Float )
                {
                    float fval = it->second.precision > 0
                                 ? (float)it->second.value / std::pow(10.0f, it->second.precision)
                                 : (float)it->second.value;
                    js->set("value", fval);
                }
                else
                    js->set("value", (int)it->second.value);
            }
        }
        else
        {
            js->set("error", "not found in OPC UA variables");
        }

        sensors->add(js);
    }

    // Add entries for sensors not found in configuration
    for( const auto& name : notFoundNames )
    {
        Object::Ptr js = new Object();
        js->set("name", name);
        js->set("error", "not found in configuration");
        sensors->add(js);
    }

    out->set("result", "OK");
    out->set("sensors", sensors);

    return out;
}
// -----------------------------------------------------------------------------
#endif // DISABLE_REST_API
// -----------------------------------------------------------------------------
