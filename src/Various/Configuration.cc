/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Vitaly Lipatov, Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
/*! \file
 *  \brief Класс работы с конфигурацией
 *  \author Vitaly Lipatov, Pavel Vainerman
 */
// --------------------------------------------------------------------------

#include <unistd.h>
#include <libgen.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <omniORB4/internal/initRefs.h>

#include "Configuration.h"
#include "Exceptions.h"
#include "MessageType.h"
#include "ObjectIndex_Array.h"
#include "ObjectIndex_XML.h"
#include "ObjectIndex_idXML.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
static const string UniSetDefaultPort = "2809";
// -------------------------------------------------------------------------
static ostream& print_help( ostream& os, int width, const string& cmd,
                            const string& help, const string& tab="" )
{
    // чтобы не менчять параметры основного потока
    // создаём свой stream...
    ostringstream info;
    info.setf(ios::left, ios::adjustfield);
    info << tab << setw(width) << cmd << " - " << help;
    return os << info.str();
}

ostream& UniSetTypes::Configuration::help(ostream& os)
{
    os << "\n UniSet Configure command: " << endl;
    print_help(os,20,"--confile","полный путь до файла конфигурации\n");
    os << "\n Debug command:\n";
    print_help(os,25,"  [debname]","имя DebugStream указанное в конфигурационном файле\n");
    print_help(os,25,"--[debname]-no-debug","отключение логов\n");
    print_help(os,25,"--[debname]-log-in-file","перенаправление лога в файл\n");
    print_help(os,25,"--[debname]-add-levels","добавить уровень вывода логов\n");
    print_help(os,25,"--[debname]-del-levels","удалить уровень вывода логов\n");
    print_help(os,25,"--uniport num","использовать заданный порт (переопеределяет 'defaultport' заданный в конф. файле в разделе <nodes>)\n");
    print_help(os,25,"--localIOR {1,0}","использовать локальные файлы для получения IOR (т.е. не использовать omniNames). Переопределяет параметр в конфигурационном файле.\n");
    print_help(os,25,"--transientIOR {1,0}","использовать генерируемые IOR(не постоянные). Переопределяет параметр в конфигурационном файле. Default=1\n");

    return os << "\nПример использования:\t myUniSetProgram "
              << "--ulog.dd-levels level1,info,system,warn --ulog.og-in-file myprogrpam.log\n\n";
}
// -------------------------------------------------------------------------

namespace UniSetTypes
{
    DebugStream ulog;
    Configuration *conf = 0;

Configuration::Configuration():
    oind(NULL),
    NSName("NameService"),
    repeatCount(2),repeatTimeout(100),
    localDBServer(UniSetTypes::DefaultObjectId),
    localNode(UniSetTypes::DefaultObjectId),
    localNodeName(""),
    fileConfName(""),
    heartbeat_msec(10000)
{
//    ulog.crit()<< " configuration FAILED!!!!!!!!!!!!!!!!!" << endl;
//    throw Exception();
}

Configuration::~Configuration()
{
    if( oind != NULL )
    {
        delete oind;
        oind=0;
    }
}
// ---------------------------------------------------------------------------------

Configuration::Configuration( int argc, const char* const* argv, const string xmlfile ):
    oind(NULL),
    _argc(argc),
    _argv(argv),
    NSName("NameService"),
    repeatCount(2),repeatTimeout(100),
    localDBServer(UniSetTypes::DefaultObjectId),
    localNode(UniSetTypes::DefaultObjectId),
    localNodeName(""),
    fileConfName(xmlfile)
{
    if( xmlfile.empty() )
        setConfFileName();

    initConfiguration(argc,argv);
}
// ---------------------------------------------------------------------------------
Configuration::Configuration( int argc, const char* const* argv, ObjectIndex* _oind,
                                const string fileConf ):
    oind(NULL),
    _argc(argc),
    _argv(argv),
    NSName("NameService"),
    repeatCount(2),repeatTimeout(100),
    localDBServer(UniSetTypes::DefaultObjectId),
    localNode(UniSetTypes::DefaultObjectId),
    localNodeName(""),
    fileConfName(fileConf)
{
    if( fileConf.empty() )
        setConfFileName();

    oind = _oind;
    initConfiguration(argc,argv);
}
// ---------------------------------------------------------------------------------
Configuration::Configuration( int argc, const char* const* argv, const string fileConf,
                UniSetTypes::ObjectInfo* omap ):
    oind(NULL),
    _argc(argc),
    _argv(argv),
    NSName("NameService"),
    repeatCount(2),repeatTimeout(100),
    localDBServer(UniSetTypes::DefaultObjectId),
    localNode(UniSetTypes::DefaultObjectId),
    localNodeName(""),
    fileConfName(fileConf)
{
    if( fileConf.empty() )
        setConfFileName();

    oind = new ObjectIndex_Array(omap);
    initConfiguration(argc,argv);
}
// ---------------------------------------------------------------------------------
void Configuration::initConfiguration( int argc, const char* const* argv )
{
//    PassiveTimer pt(UniSetTimer::WaitUpTime);
    if( ulog.is_system() )
        ulog.system() << "*** configure from file: " << fileConfName << endl;

    char curdir[FILENAME_MAX];
    getcwd(curdir,FILENAME_MAX);

    rootDir = string(curdir) + "/";

    /*! \todo Надо избавляться от глобального conf (!) */
    if( !UniSetTypes::conf )
        UniSetTypes::conf = this;

    {
        ostringstream s;
        s << this << "NameService";
        NSName = s.str();
    }

    try
    {

        try
        {
            if( !unixml.isOpen() )
            {
                if( ulog.is_info() )
                    ulog.info() << "(Configuration): open from file " <<  fileConfName << endl;
                unixml.open(fileConfName);
            }
        }
        catch(...)
        {
            ulog << "(Configuration): FAILED open configuration from " <<  fileConfName << endl;
            throw;
        }

        // default value
        heartbeat_msec = 5000;

//    cerr << "*************** initConfiguration: xmlOpen: " << pt.getCurrent() << " msec " << endl;
//    pt.reset();

        // Init ObjectIndex interface
        {
            if( oind == NULL )
            {
                UniXML_iterator it = unixml.findNode(unixml.getFirstNode(),"ObjectsMap");
                if( it == NULL )
                {
                    if( ulog.is_crit() )
                        ulog.crit()<< "(Configuration:init): not found <ObjectsMap> node in "  << fileConfName << endl;
                    throw SystemError("(Configuration:init): not found <ObjectsMap> node in " + fileConfName );
                }

                try
                {
                    if( it.getIntProp("idfromfile") == 0 )
                        oind = new ObjectIndex_XML(unixml); //(fileConfName);
                    else
                        oind = new ObjectIndex_idXML(unixml); //(fileConfName);
                }
                catch(Exception& ex )
                {
                    if( ulog.is_crit() )
                        ulog.crit()<< "(Configuration:init): INIT FAILED! from "  << fileConfName << endl;
                    throw;
                }
            }
        }

        // Настраиваем отладочные логи
        initDebug(ulog,"UniSetDebug");

//        cerr << "*************** initConfiguration: oind: " << pt.getCurrent() << " msec " << endl;
//        pt.reset();

        // default init...
        transientIOR     = false;
        localIOR     = false;

        string lnode( getArgParam("--localNode") );
        if( !lnode.empty() )
            setLocalNode(lnode);

        initParameters();

        // help
//        if( !getArgParam("--help").empty() )
//            help(cout);

        initRepSections();

        // localIOR
//        localIOR = false; // ??. initParameters()
        int lior = getArgInt("--localIOR");
        if( lior )
            localIOR = lior;

        // transientIOR
//        transientIOR = false; // ??. initParameters()
        int tior = getArgInt("--transientIOR");
        if( tior )
            transientIOR = tior;

        if( imagesDir[0]!='/' && imagesDir[0]!='.' )
            imagesDir = dataDir + imagesDir + "/";

        // считываем список узлов
        createNodesList();

        // ---------------------------------------------------------------------------------
        // добавляем новые параметры в argv
        // для передачи параметров orb по списку узлов
        // взятому из configure.xml
        // +N --> -ORBIniRef NodeName=
        // +2 --> -ORBIniRef NameService=
        _argc     = argc+2*lnodes.size()+2;
        const char** new_argv    = new const char*[_argc];

        int i = 0;
        // перегоняем старые параметры
        for( ; i < argc; i++ )
            new_argv[i] = strdup(argv[i]);

        // формируем новые, используя i в качестве индекса
        for( UniSetTypes::ListOfNode::iterator it=lnodes.begin(); it!=lnodes.end(); ++it )
        {
            new_argv[i] = "-ORBInitRef";

            string name(oind->getRealNodeName(it->id));
            ostringstream param;
            param << this << name;
            name = param.str();
            param << "=corbaname::" << it->host << ":" << it->port;
            new_argv[i+1] = strdup(param.str().c_str());

            if( ulog.is_info() )
                ulog.info() << "(Configuration): внесли параметр " << param.str() << endl;
            i+=2;

             ostringstream uri;
            uri << "corbaname::" << it->host << ":" << it->port;
            if( !omni::omniInitialReferences::setFromArgs(name.c_str(), uri.str().c_str()) )
                cerr << "**********************!!!! FAILED ADD name=" << name << " uri=" << uri.str() << endl;

            assert( i < _argc );
        }

        // т..к _argc уже изменился, то и _argv надо обновить
        // чтобы вызов getArgParam не привел к SIGSEGV
        _argv = new_argv;

        // NameService (+2)
        xmlNode* nsnode = getNode("NameService");
        if( !nsnode )
        {
            ulog.warn() << "(Configuration): не нашли раздела 'NameService' \n";
            new_argv[i]     = "";
            new_argv[i+1]     = "";
        }
        else
        {
            new_argv[i] = "-ORBInitRef";
            new_argv[i+1]     = ""; // сперва инициализиуем пустой строкой (т.к. будет вызываться getArgParam)

            string defPort( getPort( getProp(nsnode,"port") ) ); // здесь вызывается getArgParam! проходящий по _argv

            ostringstream param;
            param << this << "NameService=corbaname::" << getProp(nsnode,"host") << ":" << defPort;
            new_argv[i+1] = strdup(param.str().c_str());
            if( ulog.is_info() )
                ulog.info() << "(Configuration): внесли параметр " << param.str() << endl;

            {
                ostringstream ns_name;
                ns_name << this << "NameService";
                ostringstream uri;
                uri << "corbaname::" << getProp(nsnode,"host") << ":" << defPort;
                if( !omni::omniInitialReferences::setFromArgs(ns_name.str().c_str(), uri.str().c_str()) )
                    cerr << "**********************!!!! FAILED ADD name=" <<ns_name << " uri=" << uri.str() << endl;
            }
        }

        _argv = new_argv;
        // ------------- CORBA INIT -------------
        // orb init
        orb = CORBA::ORB_init(_argc,(char**)_argv);
        // create policy
        CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
        PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);
        CORBA::PolicyList pl;

        if( transientIOR == false )
        {
            pl.length(3);
            pl[0] = root_poa->create_lifespan_policy(PortableServer::PERSISTENT);
            pl[1] = root_poa->create_id_assignment_policy(PortableServer::USER_ID);
            pl[2] = root_poa->create_request_processing_policy(PortableServer::USE_ACTIVE_OBJECT_MAP_ONLY);
//            pl[3] = root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
        }
        else
        {
            pl.length(3);
            pl[0] = root_poa->create_lifespan_policy(PortableServer::TRANSIENT);
            pl[1] = root_poa->create_servant_retention_policy(PortableServer::RETAIN);
            pl[2] = root_poa->create_request_processing_policy(PortableServer::USE_ACTIVE_OBJECT_MAP_ONLY);
//            pl[3] = root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
        }

        policyList = pl;
        // ---------------------------------------

    }
    catch( Exception& ex )
    {
        ulog.crit()<< "Configuration:" << ex << endl;
        throw;
    }
    catch(...)
    {
        ulog.crit()<< "Configuration: INIT FAILED!!!!"<< endl;
        throw Exception("Configuration: INIT FAILED!!!!");
    }

//    cerr << "*************** initConfiguration: " << pt.getCurrent() << " msec " << endl;
}

// -------------------------------------------------------------------------
string Configuration::getArgParam( const string name, const string defval )
{
    return UniSetTypes::getArgParam(name, _argc, _argv, defval);
}

int Configuration::getArgInt( const string name, const string defval )
{
    return UniSetTypes::uni_atoi(getArgParam( name, defval ));
}

int Configuration::getArgPInt( const string name, int defval )
{
    string param = getArgParam(name,"");
    if( param.empty() )
        return defval;

    return UniSetTypes::uni_atoi(param);
}

int Configuration::getArgPInt( const string name, const string strdefval, int defval )
{
    string param = getArgParam(name,strdefval);
    if( param.empty() && strdefval.empty() )
        return defval;

    return UniSetTypes::uni_atoi(param);
}


// -------------------------------------------------------------------------
void Configuration::initParameters()
{
    xmlNode* root = unixml.findNode( unixml.getFirstNode(),"UniSet" );
    if( !root )
    {
        if( ulog.is_crit() )
            ulog.crit()<< "Configuration: INIT PARAM`s FAILED! <UniSet>...</UniSet> not found"<< endl;
        throw Exception("Configuration: INIT PARAM`s FAILED! <UniSet>...</UniSet> not found!");
    }

    UniXML_iterator it(root);
    if( !it.goChildren() )
    {
        if( ulog.is_crit() )
            ulog.crit()<< "Configuration: INIT PARAM`s FAILED!!!!"<< endl;
        throw Exception("Configuration: INIT PARAM`s FAILED!!!!");
    }

    for( ; it.getCurrent(); it.goNext() )
    {
        string name( it.getName() );

        if( name == "LocalNode" )
        {
            if( localNode == UniSetTypes::DefaultObjectId )
            {
                string nodename( it.getProp("name") );
                setLocalNode(nodename);
            }
        }
        else if( name == "LocalDBServer" )
        {
            name = it.getProp("name");
            //DBServer
            string secDB( getServicesSection()+"/"+name);
            localDBServer = oind->getIdByName(secDB);
            if( localDBServer == DefaultObjectId )
            {
                ostringstream msg;
                msg << "Configuration: DBServer '" << secDB << "' not found ServiceID in <services>!";
                if( ulog.is_crit() )
                    ulog.crit()<< msg.str() << endl;
                throw Exception(msg.str());
            }
        }
        else if( name == "CountOfNet" )
        {
            countOfNet = it.getIntProp("name");
        }
        else if( name == "RepeatTimeoutMS" )
        {
            repeatTimeout = it.getPIntProp("name", 50);
        }
        else if( name == "RepeatCount" )
        {
            repeatCount = it.getPIntProp("name", 1);
        }
        else if( name == "ImagesPath" )
        {
            imagesDir = dataDir+it.getProp("name")+"/"; // ????????
        }
        else if( name == "LocalIOR" )
        {
            localIOR = it.getIntProp("name");
        }
        else if( name == "TransientIOR" )
        {
            transientIOR = it.getIntProp("name");
        }
        else if( name == "DataDir" )
        {
            dataDir = it.getProp("name");
            if( dataDir.empty() )
                dataDir = getRootDir();
        }
        else if( name == "BinDir" )
        {
            binDir = it.getProp("name");
            if( binDir.empty() )
                binDir = getRootDir();
        }
        else if( name == "LogDir" )
        {
            logDir = it.getProp("name");
            if( logDir.empty() )
                logDir = getRootDir();
        }
        else if( name == "LockDir" )
        {
            lockDir = it.getProp("name");
            if( lockDir.empty() )
                lockDir = getRootDir();
        }
        else if( name == "ConfDir" )
        {
            confDir = it.getProp("name");
            if( confDir.empty() )
                confDir = getRootDir();
        }
        else if( name == "HeartBeatTime" )
        {
            heartbeat_msec = it.getIntProp("name");
        }
    }
}
// -------------------------------------------------------------------------
void Configuration::setLocalNode( string nodename )
{
    string virtnode = oind->getVirtualNodeName(nodename);
    if( virtnode.empty() )
        nodename = oind->mkFullNodeName(nodename,nodename);
    localNode = oind->getIdByName(nodename);

    if( localNode == DefaultObjectId )
    {
        stringstream err;
        err << "(Configuration::setLocalNode): Not found node '" << nodename << "'";
        if( ulog.is_crit() )
            ulog.crit()<< err.str() << endl;
        throw Exception(err.str());
    }

    localNodeName = oind->getRealNodeName(nodename);
    oind->initLocalNode(localNode);
}
// -------------------------------------------------------------------------
xmlNode* Configuration::getNode(const string& path)
{
    return unixml.findNode(unixml.getFirstNode(), path);
}
// -------------------------------------------------------------------------
string Configuration::getProp(xmlNode* node, const string name)
{
    return UniXML::getProp(node, name);
}
int Configuration::getIntProp(xmlNode* node, const string name)
{
    return UniXML::getIntProp(node, name);
}
int Configuration::getPIntProp(xmlNode* node, const string name, int def)
{
    return UniXML::getPIntProp(node, name, def);
}

// -------------------------------------------------------------------------
string Configuration::getField(const string path)
{
    return getProp(getNode(path),"name");
}

// -------------------------------------------------------------------------
int Configuration::getIntField(const std::string path)
{
    return unixml.getIntProp(getNode(path), "name");
}

// -------------------------------------------------------------------------
int Configuration::getPIntField(const std::string path, int def)
{
    int i = getIntField(path);;
    if (i <= 0)
        return def;
    return i;
}

// -------------------------------------------------------------------------
xmlNode* Configuration::findNode(xmlNode* node, const std::string snode, const std::string sname)
{
    if( !unixml.isOpen() )
        return 0;

    return unixml.findNode(node,snode,sname);
}
// -------------------------------------------------------------------------
string Configuration::getRootDir()
{
    return rootDir;
}

// -------------------------------------------------------------------------
void Configuration::createNodesList()
{
    xmlNode* omapnode = unixml.findNode(unixml.getFirstNode(), "ObjectsMap");
    if( !omapnode )
    {
        if( ulog.is_crit() )
            ulog.crit()<< "(Configuration): <ObjectsMap> not found!!!" << endl;
        throw Exception("(Configuration): <ObjectsMap> not found!");
    }


    xmlNode* node = unixml.findNode(omapnode, "nodes");
    if(!node)
    {
        if( ulog.is_crit() )
            ulog.crit()<< "(Configuration): <nodes> section not found!"<< endl;
        throw Exception("(Configiuration): <nodes> section not found");
    }

    UniXML_iterator it(node);
    it.goChildren();

    // определяем порт
    string defPort(getPort(unixml.getProp(node,"port")));

    lnodes.clear();
    for( ;it;it.goNext() )
    {
        string sname(getProp(it,"name"));
        if(sname.empty())
        {
            if( ulog.is_crit() )
                ulog.crit()<< "Configuration(createNodesList): unknown name='' in <nodes> "<< endl;
            throw Exception("Configuration(createNodesList: unknown name='' in <nodes>");
        }

        string nodename(sname);
        string virtnode = oind->getVirtualNodeName(nodename);
        if( virtnode.empty() )
            nodename = oind->mkFullNodeName(nodename,nodename);

        NodeInfo ninf;
//        ulog.info() << "Configuration(createNodesList): вносим узел " << nodename << endl;
        ninf.id = oind->getIdByName(nodename);
        if( ninf.id == DefaultObjectId )
        {
            if( ulog.is_crit() )
                ulog.crit()<< "Configuration(createNodesList): Not found ID for node '" << nodename << "'" << endl;
            throw Exception("Configuration(createNodesList): Not found ID for node '"+nodename+"'");
        }

        ninf.host = getProp(it,"ip").c_str();
        string tp(getProp(it,"port"));
        if( tp.empty() )
            ninf.port = defPort.c_str();
        else
            ninf.port = tp.c_str();

        string tmp(it.getProp("dbserver"));

        if( tmp.empty() )
            ninf.dbserver = UniSetTypes::DefaultObjectId;
        else
        {
            string dname(getServicesSection()+"/"+tmp);
            ninf.dbserver = oind->getIdByName(dname);
            if( ninf.dbserver == DefaultObjectId )
            {
                if( ulog.is_crit() )
                    ulog.crit()<< "Configuration(createNodesList): Not found ID for DBServer name='" << dname << "'" << endl;
                throw Exception("Configuration(createNodesList: Not found ID for DBServer name='"+dname+"'");
            }
        }

        if( ninf.id == getLocalNode() )
            localDBServer = ninf.dbserver;

        ninf.connected = false;

        initNode(ninf, it);

        if( ulog.is_info() )
            ulog.info() << "Configuration(createNodesList): add to list of nodes: node=" << nodename << " id=" << ninf.id << endl;
        lnodes.push_back(ninf);
    }

    if( ulog.is_info() )
        ulog.info() << "Configuration(createNodesList): size of node list " << lnodes.size() << endl;
}
// -------------------------------------------------------------------------
void Configuration::initNode( UniSetTypes::NodeInfo& ninfo, UniXML_iterator& it )
{
    if( ninfo.id == getLocalNode() )
        ninfo.connected = true;
    else
        ninfo.connected = false;
}
// -------------------------------------------------------------------------
string Configuration::getPropByNodeName(const string& nodename, const string& prop)
{
    xmlNode* node = getNode(nodename);
    if(node == NULL)
        return "";

    return getProp(node,prop);
}
// -------------------------------------------------------------------------
xmlNode* Configuration::initDebug( DebugStream& deb, const string& _debname )
{
    if( _debname.empty() )
    {
        deb << "(Configuration)(initDebug): INIT DEBUG FAILED!!!" << endl;
        return 0;
    }

    string debname(_debname);

    xmlNode* dnode = getNode(_debname);
    if( dnode == NULL )
        deb << "(Configuration)(initDebug):  WARNING! Not found conf. section for log '" << _debname  << "'" << endl;
    else
    {
        if( !getProp(dnode,"name").empty() )
            debname = getProp(dnode,"name");
    }

    string no_deb("--"+debname+"-no-debug");
    for (int i=1; i<_argc; i++)
    {
        if( no_deb == _argv[i] )
        {
            deb.addLevel(Debug::NONE);
            return dnode;
        }
    }

    // смотрим настройки файла
    if( dnode )
    {
        string conf_debug_levels(getProp(dnode,"levels"));
        if( !conf_debug_levels.empty() )
            deb.addLevel( Debug::value(conf_debug_levels) );
        else
            deb.addLevel(Debug::NONE);

            string debug_file(getProp(dnode,"file"));
            if( !debug_file.empty() )
                deb.logFile(debug_file);
    }

    // теперь смотрим командную строку
    string log_in("--"+debname+"-log-in-file");
    string add_level("--"+debname+"-add-levels");
    string del_level("--"+debname+"-del-levels");

    // смотрим командную строку
    for (int i=1; i < (_argc - 1); i++)
    {
        if( log_in == _argv[i] )        // "--debug-log_in_file"
        {
            deb.logFile(_argv[i+1]);
        }
        else if( add_level == _argv[i] )    // "--debug-add-levels"
        {
            deb.addLevel(Debug::value(_argv[i+1]));
        }
        else if( del_level == _argv[i] )    // "--debug-del-levels"
        {
            deb.delLevel(Debug::value(_argv[i+1]));
        }
    }

    return dnode;
}
// -------------------------------------------------------------------------
void Configuration::initRepSections()
{
    // Реализация под жёсткую структуру репозитория
    xmlNode* node( unixml.findNode(unixml.getFirstNode(),"RootSection") );
    if( node == NULL )
    {
        ostringstream msg;
        msg << "Configuration(initRepSections): Not found section <RootSection> in " << fileConfName;
        if( ulog.is_crit() )
            ulog.crit()<< msg.str() << endl;
        throw SystemError(msg.str());
    }

    secRoot         = unixml.getProp(node,"name");
    secSensors         = secRoot + "/" + getRepSectionName("sensors",xmlSensorsSec);
    secObjects        = secRoot + "/" + getRepSectionName("objects",xmlObjectsSec);
    secControlles     = secRoot + "/" + getRepSectionName("controllers",xmlControllersSec);
    secServices        = secRoot + "/" + getRepSectionName("services",xmlServicesSec);
}

string Configuration::getRepSectionName( const string sec, xmlNode* secnode )
{
    xmlNode* node = unixml.findNode(unixml.getFirstNode(),sec);
    if( node == NULL )
    {
        ostringstream msg;
        msg << "Configuration(initRepSections): Not found section '" << sec << "' in " << fileConfName;
        if( ulog.is_crit() )
            ulog.crit()<< msg.str() << endl;
        throw SystemError(msg.str());
    }

    secnode = node;

    string ret(unixml.getProp(node,"section"));
    if( ret.empty() )
        ret = unixml.getProp(node,"name");

    return ret;
}
// -------------------------------------------------------------------------
void Configuration::setConfFileName( const string fn )
{
    if( !fn.empty() )
    {
        fileConfName = fn;
        return;
    }

    // Определение конфигурационного файла
    // в порядке убывания приоритета

    string tmp( getArgParam("--confile") );
    if( !tmp.empty() )
    {
        fileConfName = tmp;
    }
    else if( getenv("UNISET_CONFILE") != NULL )
    {
        fileConfName = getenv("UNISET_CONFILE");
    }

    if( fileConfName.empty() )
    {
        ostringstream msg;
        msg << "\n\n***** CRIT: Unknown configure file." << endl
            << " Use --confile filename " << endl
            << " OR define enviropment variable UNISET_CONFILE" << endl;
        if( ulog.is_crit() )
            ulog.crit()<< msg.str();
        throw SystemError(msg.str());
    }
}
// -------------------------------------------------------------------------
string Configuration::getPort(const string port)
{
    // Порт задан в параметрах программы
    string defport(getArgParam("--uniset-port"));
    if( !defport.empty() )
        return defport;

    // Порт задан в переменной окружения
    if( getenv("UNISET_PORT") != NULL )
    {
        defport = getenv("UNISET_PORT");
        return defport;
    }

    // Порт задан в параметрах
    if( !port.empty() )
        return port;

    // Порт по умолчанию
    return UniSetDefaultPort;
}
// -------------------------------------------------------------------------
ObjectId Configuration::getSensorID( const std::string name )
{
    if( name.empty() )
        return DefaultObjectId;

    return oind->getIdByName(getSensorsSection()+"/"+name);
}
// -------------------------------------------------------------------------
ObjectId Configuration::getControllerID( const std::string name )
{
    if( name.empty() )
        return DefaultObjectId;

    return oind->getIdByName(getControllersSection()+"/"+name);
}
// -------------------------------------------------------------------------
ObjectId Configuration::getObjectID( const std::string name )
{
    if( name.empty() )
        return DefaultObjectId;

    return oind->getIdByName(getObjectsSection()+"/"+name);
}
// -------------------------------------------------------------------------
ObjectId Configuration::getServiceID( const std::string name )
{
    if( name.empty() )
        return DefaultObjectId;

    return oind->getIdByName(getServicesSection()+"/"+name);
}
// -------------------------------------------------------------------------
UniSetTypes::ObjectId Configuration::getNodeID( const std::string name, std::string alias )
{
    if( name.empty() )
        return DefaultObjectId;

    if( alias.empty() )
        alias = name;
//    return oind->getNodeId( oind->mkFullNodeName(name,alias) );
    return oind->getIdByName( oind->mkFullNodeName(name,alias) );
}

// -------------------------------------------------------------------------
xmlNode* Configuration::xmlSensorsSec = 0;
xmlNode* Configuration::xmlObjectsSec = 0;
xmlNode* Configuration::xmlControllersSec = 0;
xmlNode* Configuration::xmlServicesSec = 0;
xmlNode* Configuration::xmlNodesSec = 0;
// -------------------------------------------------------------------------
xmlNode* Configuration::getXMLSensorsSection()
{
    if( xmlSensorsSec )
        return xmlSensorsSec;

    xmlSensorsSec = unixml.findNode(unixml.getFirstNode(),"sensors");
    return xmlSensorsSec;
}
// -------------------------------------------------------------------------
xmlNode* Configuration::getXMLObjectsSection()
{
    if( xmlObjectsSec )
        return xmlObjectsSec;

    xmlObjectsSec = unixml.findNode(unixml.getFirstNode(),"objects");
    return xmlObjectsSec;
}
// -------------------------------------------------------------------------
xmlNode* Configuration::getXMLControllersSection()
{
    if( xmlControllersSec )
        return xmlControllersSec;

    xmlControllersSec = unixml.findNode(unixml.getFirstNode(),"controllers");
    return xmlControllersSec;

}
// -------------------------------------------------------------------------
xmlNode* Configuration::getXMLServicesSection()
{
    if( xmlServicesSec )
        return xmlServicesSec;

    xmlServicesSec = unixml.findNode(unixml.getFirstNode(),"services");
    return xmlServicesSec;
}
// -------------------------------------------------------------------------
xmlNode* Configuration::getXMLNodesSection()
{
    if( xmlNodesSec )
        return xmlNodesSec;

    xmlNodesSec = unixml.findNode(unixml.getFirstNode(),"nodes");
    return xmlNodesSec;
}
// -------------------------------------------------------------------------
xmlNode* Configuration::getXMLObjectNode( UniSetTypes::ObjectId id )
{
    const ObjectInfo* i = oind->getObjectInfo(id);
    if( i )
        return (xmlNode*)(i->data);

    return 0;
}
// -------------------------------------------------------------------------
UniversalIO::IOType Configuration::getIOType( UniSetTypes::ObjectId id )
{
    const ObjectInfo* i = oind->getObjectInfo(id);
    if( i && (xmlNode*)(i->data) )
    {
        UniXML_iterator it((xmlNode*)(i->data));
        return UniSetTypes::getIOType( it.getProp("iotype") );
    }

    return UniversalIO::UnknownIOType;
}
// -------------------------------------------------------------------------
UniversalIO::IOType Configuration::getIOType( const std::string name )
{
    // Если указано "короткое" имя
    // то просто сперва ищём ID, а потом по нему
    // iotype
    ObjectId id = getSensorID(name);
    if( id != DefaultObjectId )
        return getIOType(id);

    // Пробуем искать, считая, что задано mapname
    // ("RootSection/Sensors/name")
    const ObjectInfo* i = oind->getObjectInfo(name);
    if( i && (xmlNode*)(i->data) )
    {
        UniXML_iterator it((xmlNode*)(i->data));
        return UniSetTypes::getIOType( it.getProp("iotype") );
    }

    return UniversalIO::UnknownIOType;
}
// -------------------------------------------------------------------------
void uniset_init( int argc, const char* const* argv, const std::string& xmlfile )
{
    string confile = UniSetTypes::getArgParam( "--confile", argc, argv, xmlfile );
    UniSetTypes::conf = new Configuration(argc, argv, confile);
}
// -------------------------------------------------------------------------
} // end of UniSetTypes namespace
