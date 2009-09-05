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
 *  \date   $Date: 2008/09/16 19:02:46 $
 *  \version $Id: Configuration.cc,v 1.41 2008/09/16 19:02:46 vpashka Exp $
 */
// --------------------------------------------------------------------------

#include <unistd.h>
#include <libgen.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include "Configuration.h"
#include "Exceptions.h"
#include "MessageType.h"
#include "ObjectIndex_Array.h"
#include "ObjectIndex_XML.h"
#include "ObjectIndex_idXML.h"
#include "DefaultMessageInterface.h"
#include "MessageInterface_XML.h"
#include "MessageInterface_idXML.h"
#include "SystemGuard.h"

// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
static const string UniSetDefaultPort = "2809";
// -------------------------------------------------------------------------
static ostream& print_help( ostream& os, int width, const string cmd, 
							const string help, const string tab="" )
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
			  << "--unideb-add-levels level1,info,system,warn --unideb-log-in-file myprogrpam.log\n\n";
}
// -------------------------------------------------------------------------

namespace UniSetTypes
{
	DebugStream unideb;
	Configuration *conf;

Configuration::Configuration():
	mi(NULL),
	oind(NULL),
	NSName("NameService"),
	repeatCount(2),repeatTimeout(100), 	
	localTimerService(UniSetTypes::DefaultObjectId),
	localDBServer(UniSetTypes::DefaultObjectId),
	localInfoServer(UniSetTypes::DefaultObjectId),
	fileConfName("")
{
//	unideb[Debug::CRIT] << " configuration FAILED!!!!!!!!!!!!!!!!!" << endl;
//	throw Exception();
}

Configuration::~Configuration()
{
	if( oind != NULL )
	{
		delete oind;
		oind=0;
	}

	if( mi != NULL )
	{
		delete mi;	
		mi=0;
	}
/*
	// ??. ObjectsActivator.cc	
	unideb[Debug::SYSTEM] << "(Configuration): orb destroy..." << endl;
	try
	{
		orb->destroy();
	}
	catch(...){}
	unideb[Debug::SYSTEM] << "(Configuration): orb destroy ok."<< endl;
*/	
}
// ---------------------------------------------------------------------------------

Configuration::Configuration( int argc, const char* const* argv, const string xmlfile ):
	mi(NULL),
	oind(NULL),
	_argc(argc),
	_argv(argv),
	NSName("NameService"),
	repeatCount(2),repeatTimeout(100),
	localTimerService(UniSetTypes::DefaultObjectId),
	localDBServer(UniSetTypes::DefaultObjectId),
	localInfoServer(UniSetTypes::DefaultObjectId),
	fileConfName(xmlfile)
{
	if( xmlfile.empty() )
		setConfFileName();

	initConfiguration(argc,argv);
}
// ---------------------------------------------------------------------------------
Configuration::Configuration( int argc, const char* const* argv, ObjectIndex* _oind,
								const string fileConf ):
	mi(NULL),
	oind(NULL),
	_argc(argc),
	_argv(argv),
	NSName("NameService"),
	repeatCount(2),repeatTimeout(100), 
	localTimerService(UniSetTypes::DefaultObjectId),
	localDBServer(UniSetTypes::DefaultObjectId),
	localInfoServer(UniSetTypes::DefaultObjectId),
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
	mi(NULL),
	oind(NULL),
	_argc(argc),
	_argv(argv),
	NSName("NameService"),
	repeatCount(2),repeatTimeout(100), 
	localTimerService(UniSetTypes::DefaultObjectId),
	localDBServer(UniSetTypes::DefaultObjectId),
	localInfoServer(UniSetTypes::DefaultObjectId),
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
//	PassiveTimer pt(UniSetTimer::WaitUpTime);
	if( unideb.debugging(Debug::SYSTEM) )
		unideb[Debug::SYSTEM] << "*** configure from file: " << fileConfName << endl;

	char curdir[FILENAME_MAX];
	getcwd(curdir,FILENAME_MAX);

	rootDir = string(curdir) + "/";
	UniSetTypes::conf = this;

	try
	{

		try
		{
			if( !unixml.isOpen() )
			{
				if( unideb.debugging(Debug::INFO) )
					unideb[Debug::INFO] << "Configuration: open from file " <<  fileConfName << endl;
				unixml.open(fileConfName);
			}
		}
		catch(...)
		{
			unideb << " FAILED open configuration from " <<  fileConfName << endl;
			throw;
		}

//	cerr << "*************** initConfiguration: xmlOpen: " << pt.getCurrent() << " msec " << endl;
//	pt.reset();
	
		// Init ObjectIndex interface
		{
			if( oind == NULL )
			{
				xmlNode* cnode = unixml.findNode(unixml.getFirstNode(),"ObjectsMap");
				if( !cnode )
				{
					unideb[Debug::CRIT] << "(Configuration:init): not found <ObjectsMap> node in "  << fileConfName << endl;
					throw SystemError("(Configuration:init): not found <ObjectsMap> node in " + fileConfName );
				}

				try
				{
					if( unixml.getIntProp(cnode,"idfromfile") == 0 )
						oind = new ObjectIndex_XML(unixml); //(fileConfName);
					else
						oind = new ObjectIndex_idXML(unixml); //(fileConfName);
				}
				catch(Exception& ex )
				{
					unideb[Debug::CRIT] << "(Configuration:init): INIT FAILED! from "  << fileConfName << endl;
					throw ex;
				}
			}
		}
	
		// Init MessageInterface
		{
			xmlNode* cnode = unixml.findNode(unixml.getFirstNode(),"messages");
			if( cnode == NULL )
				mi = new DefaultMessageInterface();
			else
			{
				if( unixml.getIntProp(cnode,"idfromfile") == 0 )
					mi = new MessageInterface_XML(unixml); // (fileConfName);
				else
					mi = new MessageInterface_idXML(unixml); // (fileConfName);
			}
		}
		
		// Настраиваем отладочные логи
		initDebug(unideb, "UniSetDebug");

//		cerr << "*************** initConfiguration: oind: " << pt.getCurrent() << " msec " << endl;
//		pt.reset();

		// default init...
		transientIOR 	= false;
		localIOR 	= false;

		initParameters();


		string lnode( getArgParam("--localNode") );
		if( !lnode.empty() )
			setLocalNode(lnode);

		// help
//		if( !getArgParam("--help").empty() )
//			help(cout);

		initRepSections();

		// localIOR
//		localIOR = false; // ??. initParameters()
		int lior = getArgInt("--localIOR");
		if( lior )
			localIOR = lior;

		// transientIOR
//		transientIOR = false; // ??. initParameters()
		int tior = getArgInt("--transientIOR");
		if( tior )
			transientIOR = tior;

		if( imagesDir[0]!='/' && imagesDir[0]!='.' )
			imagesDir = dataDir + imagesDir + "/";

//		cerr << "*************** initConfiguration: parameters...: " << pt.getCurrent() << " msec " << endl;
//		pt.reset();
		
		// считываем список узлов
		createNodesList();

		// ---------------------------------------------------------------------------------
		// добавляем новые параметры в argv
		// для передачи параметров orb по списку узлов
		// взятому из configure.xml
		// +N --> -ORBIniRef NodeName=
		// +2 --> -ORBIniRef NameService=
		_argc 	= argc+2*lnodes.size()+2;
		const char** new_argv	= new const char*[_argc];

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
			param << name << "=corbaname::" << it->host << ":" << it->port;
			new_argv[i+1] = strdup(param.str().c_str());

			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << "(Configuration): внесли параметр " << param.str() << endl;
			i+=2;

			assert( i < _argc );
		}
		
		// т..к _argc уже изменился, то и _argv надо обновить
		// чтобы вызов getArgParam не привел к SIGSEGV
		_argv = new_argv;

		// NameService (+2)
		xmlNode* nsnode = getNode("NameService");
		if( !nsnode )
		{
			unideb[Debug::WARN] << "(Configuration): не нашли раздела 'NameService' \n";
			new_argv[i] 	= "";
			new_argv[i+1] 	= "";
		}
		else
		{
			new_argv[i] = "-ORBInitRef";
			new_argv[i+1] 	= ""; // сперва инициализиуем пустой строкой (т.к. будет вызываться getArgParam)
			
			string defPort( getPort( getProp(nsnode,"port") ) ); // здесь вызывается getArgParam! проходящий по _argv

			ostringstream param;
			param <<"NameService=corbaname::" << getProp(nsnode,"host") << ":" << defPort;
			new_argv[i+1] = strdup(param.str().c_str());
			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << "(Configuration): внесли параметр " << param.str() << endl;
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
//			pl[3] = root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
		}
		else
		{
			pl.length(3);
			pl[0] = root_poa->create_lifespan_policy(PortableServer::TRANSIENT);
			pl[1] = root_poa->create_servant_retention_policy(PortableServer::RETAIN);
			pl[2] = root_poa->create_request_processing_policy(PortableServer::USE_ACTIVE_OBJECT_MAP_ONLY);
//			pl[3] = root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
		}	
	
		policyList = pl;
		// ---------------------------------------		

	}
	catch( Exception& ex )
	{
		unideb[Debug::CRIT] << "Configuration:" << ex << endl;
		throw;
	}
	catch(...)
	{
		unideb[Debug::CRIT] << "Configuration: INIT FAILED!!!!"<< endl;
		throw Exception("Configuration: INIT FAILED!!!!");
	}

//	cerr << "*************** initConfiguration: " << pt.getCurrent() << " msec " << endl;
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
	int i = getArgInt(name);
	if ( i <= 0 )
		return defval;
	return i;
}

int Configuration::getArgPInt( const string name, const string strdefval, int defval )
{
	int i = getArgInt(name, strdefval);
	if ( i <= 0 )
		return defval;
	return i;
}


// -------------------------------------------------------------------------
// ????????????? ????????... (??? ???????????)
// WARNING: ??????? ??????? ?? ?????????????????? ???????? ? ????. ?????!!!
// ????????: ???? LocalNode ????? ????? LocalXXXServer, ?? ?????????????? ?? ????? ???????!!!
// ?.?. ?? ????? ??? ???????? LocalNode id.
// ???? ?????????? ??????? ?? ??????????? ????????????.
void Configuration::initParameters()
{
	xmlNode* root = unixml.findNode( unixml.getFirstNode(),"UniSet" );
	if( !root )
	{
		unideb[Debug::CRIT] << "Configuration: INIT PARAM`s FAILED! <UniSet>...</UniSet> not found"<< endl;
		throw Exception("Configuration: INIT PARAM`s FAILED! <UniSet>...</UniSet> not found!");
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		unideb[Debug::CRIT] << "Configuration: INIT PARAM`s FAILED!!!!"<< endl;
		throw Exception("Configuration: INIT PARAM`s FAILED!!!!");
	}
	
	for( ; it.getCurrent(); it.goNext() )
	{
		string name( it.getName() );
		
		if( name == "LocalNode" )
		{
			string nodename( it.getProp("name") );
			setLocalNode(nodename);
		}
		else if( name == "LocalTimerService" )
		{
			name = it.getProp("name");
		
			// TimerService
			string secTime( getServicesSection()+"/"+name);
			localTimerService = oind->getIdByName(secTime);
			if( localTimerService == DefaultObjectId )
			{
				ostringstream msg;
				msg << "Configuration: TimerService  '" << secTime << "' ?? ?????? ????????????? ÷ ObjectsMap!!!";
				unideb[Debug::CRIT] << msg.str() << endl;
				throw Exception(msg.str());
			}
		}
		else if( name == "LocalInfoServer" )
		{
			name = it.getProp("name");
			// InfoServer
			string secInfo( getServicesSection()+"/"+name);
			localInfoServer = oind->getIdByName(secInfo);
			if( localInfoServer == DefaultObjectId )
			{
				ostringstream msg;
				msg << "Configuration: InfoServer '" << secInfo << "' ?? ?????? ????????????? ÷ ObjectsMap!!!";
				unideb[Debug::CRIT] << msg.str() << endl;
				throw Exception(msg.str());
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
				msg << "Configuration: DBServer '" << secDB << "' ?? ?????? ????????????? ÷ ObjectsMap!!!";
				unideb[Debug::CRIT] << msg.str() << endl;
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
		unideb[Debug::CRIT] << "(Configuration::setLocalNode): node '" << nodename << "' ?? ?????? ????????????? ÷ ObjectsMap!!!" << endl;
		throw Exception("(Configuration::setLocalNode): node '"+nodename+"' ?? ?????? ????????????? ÷ ObjectsMap!!!");
	}
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
		unideb[Debug::CRIT] << "(Configuration): не нашли раздела ObjectsMap..."<< endl;
		throw Exception("configiuration: не нашли раздела ObjectsMap");
	}


	xmlNode* node = unixml.findNode(omapnode, "nodes");
	if(!node)
	{
		unideb[Debug::CRIT] << "(Configuration): не нашли раздела ObjectsMap/nodes..."<< endl;
		throw Exception("configiuration: не нашли раздела ObjectsMap/nodes");
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
			unideb[Debug::CRIT] << "Configuration(createNodesList): не задано имя узла "<< endl;
			throw Exception("Configuration(createNodesList: в списке узлов не задано имя!");
//			continue;
		}

		string nodename(sname);
		string virtnode = oind->getVirtualNodeName(nodename);
		if( virtnode.empty() )
			nodename = oind->mkFullNodeName(nodename,nodename);

		NodeInfo ninf;
//		unideb[Debug::INFO] << "Configuration(createNodesList): вносим узел " << nodename << endl;
		ninf.id = oind->getIdByName(nodename);
		if( ninf.id == DefaultObjectId )
		{
			unideb[Debug::CRIT] << "Configuration(createNodesList): node '" << nodename << "' НЕ НАЙДЕН ИДЕНТИФИКАТОР В ObjectsMap!!!" << endl;
			throw Exception("Configuration(createNodesList): node "+nodename+" НЕ НАЙДЕН ИДЕНТИФИКАТОР В ObjectsMap!!!");
//			continue;
		}
		
		ninf.host = getProp(it,"ip").c_str();
		string tp(getProp(it,"port"));
		if( tp.empty() )
			ninf.port = defPort.c_str();
		else
			ninf.port = tp.c_str();

		string tmp(getProp(it,"infserver"));
		if( tmp.empty() )
			ninf.infserver = UniSetTypes::DefaultObjectId;
		else
		{
			string iname(getServicesSection()+"/"+tmp);
			ninf.infserver = oind->getIdByName(iname);
			if( ninf.infserver == DefaultObjectId )
			{
				unideb[Debug::CRIT] << "Configuration(createNodesList): InfoServre '" << iname << "' НЕ НАЙДЕН ИДЕНТИФИКАТОР В ObjectsMap!!!" << endl;
				throw Exception("Configuration(createNodesList: InfoServer "+iname+" НЕ НАЙДЕН ИДЕНТИФИКАТОР В ObjectsMap!!!");
			}
		}

		tmp = it.getProp("dbserver");
		
		if( tmp.empty() )
			ninf.dbserver = UniSetTypes::DefaultObjectId;
		else
		{
			string dname(getServicesSection()+"/"+tmp);
			ninf.dbserver = oind->getIdByName(dname);
			if( ninf.dbserver == DefaultObjectId )
			{
				unideb[Debug::CRIT] << "Configuration(createNodesList): DBServer '" << dname << "' ?? ?????? ????????????? ÷ ObjectsMap!!!" << endl;
				throw Exception("Configuration(createNodesList: DBServer "+dname+" ?? ?????? ????????????? ÷ ObjectsMap!!!");
			}
		}

		if( ninf.id == getLocalNode() )
		{
			localDBServer = ninf.dbserver;
			localInfoServer = ninf.infserver;
		}

		ninf.connected = false;
		
		initNode(ninf, it);

		if( unideb.debugging(Debug::INFO) )
			unideb[Debug::INFO] << "Configuration(createNodesList): добавляем в список узел name: " << nodename << " id=" << ninf.id << endl;
		lnodes.push_back(ninf);
	}	

	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << "Configuration(createNodesList): ВСЕГО УЗЛОВ В СПИСКЕ " << lnodes.size() << endl;
}
// -------------------------------------------------------------------------
void Configuration::initNode( UniSetTypes::NodeInfo& ninfo, UniXML_iterator& it )
{
	if( ninfo.id == conf->getLocalNode() )
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
		deb << "(Configuration)(initDebug): INIT DEBUG FAILED!!! НЕ ЗАДАНО НАЗВАНИЕ ЛОГА\n";
		return 0;
	}

	string debname(_debname);

	xmlNode* dnode = conf->getNode(_debname);
	if( dnode == NULL )
	{
		deb << "(Configuration)(initDebug):  WARNING! не нашли раздела " << _debname << endl;
		// тогда считаем, что задано название самого DebugStream
	}
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
				deb.logFile(debug_file.c_str());
	}
	
	// теперь смотрим командную строку
	string log_in("--"+debname+"-log-in-file");
	string add_level("--"+debname+"-add-levels");
	string del_level("--"+debname+"-del-levels");
		
	// смотрим командную строку
	for (int i=1; i < (_argc - 1); i++)
	{
		if( log_in == _argv[i] )		// "--debug-log_in_file"
		{
			deb.logFile(_argv[i+1]);
		}
		else if( add_level == _argv[i] )	// "--debug-add-levels"
		{
			deb.addLevel(Debug::value(_argv[i+1]));
		}
		else if( del_level == _argv[i] )	// "--debug-del-levels"
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
		msg << "Configuration(initRepSections): не нашли параметр RootSection в конф. файле " << fileConfName;
		unideb[Debug::CRIT] << msg.str() << endl;
		throw SystemError(msg.str());
	}

	secRoot 		= unixml.getProp(node,"name");
	secSensors 		= secRoot + "/" + getRepSectionName("sensors",xmlSensorsSec);
	secObjects		= secRoot + "/" + getRepSectionName("objects",xmlObjectsSec);
	secControlles 	= secRoot + "/" + getRepSectionName("controllers",xmlControllersSec);
	secServices		= secRoot + "/" + getRepSectionName("services",xmlServicesSec);
}

string Configuration::getRepSectionName( const string sec, xmlNode* secnode )
{
	xmlNode* node = unixml.findNode(unixml.getFirstNode(),sec);
	if( node == NULL )
	{
		ostringstream msg;
		msg << "Configuration(initRepSections): не нашли секцию " << sec << " в конф. файле " << fileConfName;
		unideb[Debug::CRIT] << msg.str() << endl;
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
		msg << "\n\n***** CRIT: Не задан конфигурационный файл. \n"
			<< " Задайте ключ --confile\n"
			<< " Или определите переменную окружения UNISET_CONFILE\n\n";
//			<< " Или разместить файл в /etc/uniset-configure.xml \n\n";
		unideb[Debug::CRIT] << msg.str();
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
		
	return oind->getIdByName(conf->getSensorsSection()+"/"+name);
}
// -------------------------------------------------------------------------
ObjectId Configuration::getControllerID( const std::string name )
{
	if( name.empty() )
		return DefaultObjectId;

	return oind->getIdByName(conf->getControllersSection()+"/"+name);
}
// -------------------------------------------------------------------------
ObjectId Configuration::getObjectID( const std::string name )
{
	if( name.empty() )
		return DefaultObjectId;

	return oind->getIdByName(conf->getObjectsSection()+"/"+name);
}
// -------------------------------------------------------------------------
ObjectId Configuration::getServiceID( const std::string name )
{
	if( name.empty() )
		return DefaultObjectId;

	return oind->getIdByName(conf->getServicesSection()+"/"+name);
}
// -------------------------------------------------------------------------
UniSetTypes::ObjectId Configuration::getNodeID( const std::string name, std::string alias )
{
	if( name.empty() )
		return DefaultObjectId;

	if( alias.empty() )
		alias = name;
//	return oind->getNodeId( oind->mkFullNodeName(name,alias) );
	return oind->getIdByName( oind->mkFullNodeName(name,alias) );
}

// -------------------------------------------------------------------------
xmlNode* Configuration::xmlSensorsSec = 0;
xmlNode* Configuration::xmlObjectsSec = 0;
xmlNode* Configuration::xmlControllersSec = 0;
xmlNode* Configuration::xmlServicesSec = 0;
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
void uniset_init( int argc, const char* const* argv, const std::string xmlfile )
{
	string confile = UniSetTypes::getArgParam( "--confile", argc, argv, xmlfile );
	UniSetTypes::conf = new Configuration(argc, argv, confile);
}
// -------------------------------------------------------------------------
} // end of UniSetTypes namespace
