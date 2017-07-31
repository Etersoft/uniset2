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
#include <cassert>
#include <omniORB4/internal/initRefs.h>

#include "Configuration.h"
#include "Exceptions.h"
#include "MessageType.h"
#include "ObjectIndex_Array.h"
#include "ObjectIndex_XML.h"
#include "ObjectIndex_idXML.h"
#include "UniSetActivator.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
static const string UniSetDefaultPort = "2809";
// -------------------------------------------------------------------------
static ostream& print_help( ostream& os, int width, const string& cmd,
							const string& help, const string& tab = "" )
{
	// чтобы не менчять параметры основного потока
	// создаём свой stream...
	ostringstream info;
	info.setf(ios::left, ios::adjustfield);
	info << tab << setw(width) << cmd << " - " << help;
	return os << info.str();
}

ostream& uniset::Configuration::help(ostream& os)
{
	os << "\n UniSet Configure command: " << endl;
	print_help(os, 20, "--confile", "полный путь до файла конфигурации\n");
	os << "\n Debug command:\n";
	print_help(os, 25, "  [debname]", "имя DebugStream указанное в конфигурационном файле\n");
	print_help(os, 25, "--[debname]-no-debug", "отключение логов\n");
	print_help(os, 25, "--[debname]-logfile", "перенаправление лога в файл\n");
	print_help(os, 25, "--[debname]-add-levels", "добавить уровень вывода логов\n");
	print_help(os, 25, "--[debname]-del-levels", "удалить уровень вывода логов\n");
	print_help(os, 25, "--[debname]-show-microseconds", "Выводить время с микросекундами\n");
	print_help(os, 25, "--[debname]-show-milliseconds", "Выводить время с миллисекундами\n");
	print_help(os, 25, "--uniport num", "использовать заданный порт (переопеределяет 'defaultport' заданный в конф. файле в разделе <nodes>)\n");
	print_help(os, 25, "--localIOR {1,0}", "использовать локальные файлы для получения IOR (т.е. не использовать omniNames). Переопределяет параметр в конфигурационном файле.\n");
	print_help(os, 25, "--transientIOR {1,0}", "использовать генерируемые IOR(не постоянные). Переопределяет параметр в конфигурационном файле. Default=1\n");

	return os << "\nПример использования:\t myUniSetProgram "
		   << "--ulog-add-levels level1,info,system,warn --ulog-logfile myprogrpam.log\n\n";
}
// -------------------------------------------------------------------------
namespace uniset
{
	static std::shared_ptr<Configuration> uconf;
	static std::shared_ptr<DebugStream> _ulog;

	std::shared_ptr<DebugStream> ulog() noexcept
	{
		if( _ulog )
			return _ulog;

		_ulog = make_shared<DebugStream>();
		_ulog->setLogName("ulog");
		return _ulog;
	}

	std::shared_ptr<Configuration> uniset_conf() noexcept
	{
		// Не делаем assert или exception
		// потому-что считаем, что может быть необходимость не вызывать uniset_init()
		// Т.е. проверка if( uconf ) возлагается на пользователя.

		// assert( uconf );

		// if( uconf == nullptr )
		//		throw SystemError("Don`t init uniset configuration! First use uniset_init().");

		return uconf; // см. uniset_init..
	}
	// -------------------------------------------------------------------------


	Configuration::Configuration():
		oind(NULL),
		//		_argc(0),
		//		_argv(nullptr),
		NSName("NameService"),
		repeatCount(2), repeatTimeout(100),
		localDBServer(uniset::DefaultObjectId),
		localNode(uniset::DefaultObjectId),
		localNodeName(""),
		fileConfName(""),
		heartbeat_msec(3000)
	{
		//    ulog.crit()<< " configuration FAILED!!!!!!!!!!!!!!!!!" << endl;
		//    throw Exception();
	}

	Configuration::~Configuration()
	{
		if( oind )
			oind.reset();

		if( unixml )
			unixml.reset();

		for( int i = 0; i < _argc; i++ )
			delete[] _argv[i];

		delete[] _argv;
	}
	// ---------------------------------------------------------------------------------

	Configuration::Configuration( int argc, const char* const* argv, const string& xmlfile ):
		oind(nullptr),
		unixml(nullptr),
		//		_argc(argc),
		//		_argv(argv),
		NSName("NameService"),
		repeatCount(2), repeatTimeout(100),
		localDBServer(uniset::DefaultObjectId),
		localNode(uniset::DefaultObjectId),
		localNodeName(""),
		fileConfName(xmlfile)
	{
		initConfiguration(argc, argv);
	}
	// ---------------------------------------------------------------------------------
	Configuration::Configuration( int argc, const char* const* argv, shared_ptr<ObjectIndex> _oind,
								  const string& fileConf ):
		oind(nullptr),
		unixml(nullptr),
		//		_argc(argc),
		//		_argv(argv),
		NSName("NameService"),
		repeatCount(2), repeatTimeout(100),
		localDBServer(uniset::DefaultObjectId),
		localNode(uniset::DefaultObjectId),
		localNodeName(""),
		fileConfName(fileConf)
	{
		oind = _oind;
		initConfiguration(argc, argv);
	}
	// ---------------------------------------------------------------------------------
	Configuration::Configuration( int argc, const char* const* argv, const string& fileConf,
								  uniset::ObjectInfo* omap ):
		oind(NULL),
		//		_argc(argc),
		//		_argv(argv),
		NSName("NameService"),
		repeatCount(2), repeatTimeout(100),
		localDBServer(uniset::DefaultObjectId),
		localNode(uniset::DefaultObjectId),
		localNodeName(""),
		fileConfName(fileConf)
	{
		shared_ptr<ObjectIndex_Array> _oi = make_shared<ObjectIndex_Array>(omap);
		oind = static_pointer_cast<ObjectIndex>(_oi);

		initConfiguration(argc, argv);
	}
	// ---------------------------------------------------------------------------------
	void Configuration::initConfiguration( int argc, const char* const* argv )
	{
		// т.к. мы не знаем откуда эти argc и argv и может они будут удалены сразу после завершения функции
		// то надёжнее скопировать себе всё..
		_argc = argc;
		_argv = new const char* [argc];

		for( int i = 0; i < argc; i++ )
			_argv[i] = uniset::uni_strdup(argv[i]);

		iorfile = make_shared<IORFile>();

		// инициализировать надо после argc,argv
		if( fileConfName.empty() )
			setConfFileName();

		//    PassiveTimer pt(UniSetTimer::WaitUpTime);
		ulogsys << "*** configure from file: " << fileConfName << endl;

		// -------------------------------------------------------------------------
		xmlSensorsSec = 0;
		xmlObjectsSec = 0;
		xmlControllersSec = 0;
		xmlServicesSec = 0;
		xmlNodesSec = 0;
		// -------------------------------------------------------------------------
		char curdir[FILENAME_MAX];

		if( getcwd(curdir, FILENAME_MAX) == NULL )
			rootDir = "";
		else
			rootDir = string(curdir) + "/";

		{
			ostringstream s;
			s << this << "NameService";
			NSName = s.str();
		}

		try
		{

			try
			{
				if( !unixml )
					unixml = make_shared<UniXML>();

				if( !unixml->isOpen() )
				{
					uinfo << "(Configuration): open from file " <<  fileConfName << endl;
					unixml->open(fileConfName);
				}
			}
			catch(...)
			{
				ostringstream err;
				err << "(Configuration): FAILED open configuration from " <<  fileConfName;
				ulogany << err.str() << endl;
				throw uniset::SystemError(err.str());
			}


			//    cerr << "*************** initConfiguration: xmlOpen: " << pt.getCurrent() << " msec " << endl;
			//    pt.reset();

			// Init ObjectIndex interface
			{
				if( oind == nullptr )
				{
					UniXML::iterator it = unixml->findNode(unixml->getFirstNode(), "ObjectsMap");

					if( !it )
					{
						ucrit << "(Configuration:init): not found <ObjectsMap> node in "  << fileConfName << endl;
						throw uniset::SystemError("(Configuration:init): not found <ObjectsMap> node in " + fileConfName );
					}

					try
					{
						if( it.getIntProp("idfromfile") == 0 )
						{
							shared_ptr<ObjectIndex_XML> oi = make_shared<ObjectIndex_XML>(unixml); //(fileConfName);
							oind = static_pointer_cast<ObjectIndex>(oi);
						}
						else
						{
							shared_ptr<ObjectIndex_idXML> oi = make_shared<ObjectIndex_idXML>(unixml); //(fileConfName);
							oind = static_pointer_cast<ObjectIndex>(oi);
						}
					}
					catch( const uniset::Exception& ex )
					{
						ucrit << ex << endl;
						throw;
					}
				}
			}

			// Настраиваем отладочные логи
			initLogStream(ulog(), "ulog");

			// default init...
			transientIOR     = false;
			localIOR     = false;

			string lnode( getArgParam("--localNode") );

			if( !lnode.empty() )
				setLocalNode(lnode);

			initParameters();
			initRepSections();

			// localIOR
			int lior = getArgInt("--localIOR");

			if( lior )
				localIOR = lior;

			// transientIOR
			int tior = getArgInt("--transientIOR");

			if( tior )
				transientIOR = tior;

			if( imagesDir[0] != '/' && imagesDir[0] != '.' )
				imagesDir = dataDir + imagesDir + "/";

			// считываем список узлов
			createNodesList();

			std::list< std::pair<string, string> > omniParams;
			// ---------------------------------------------------------------------------------
			UniXML::iterator omniIt(unixml->findNode(unixml->getFirstNode(), "omniORB") );

			if( omniIt && omniIt.goChildren() )
			{
				for(; omniIt.getCurrent(); omniIt++ )
				{
					std::string p(omniIt.getProp("name"));

					if( p.empty() )
					{
						uwarn << "(Configuration::init): unknown omniORB param...name=''" << endl;
					}
					else
					{
						// для endPoint надо отдельно проверить доступность адреса
						// иначе иницилизация omni не произойдёт, а нужно чтобы
						// всё запускалось даже если сеть не вся "поднялась"
						if( p == "endPoint" )
						{
							const string param(omniIt.getProp("arg"));
							bool endPointIsAvailable = omniIt.getProp("ignore_checking").empty() ? checkOmniORBendPoint(param) : true;

							// по умолчанию "недоступность" игнорируется
							// но если указан параметр 'error_if_not_available'
							// то кидаем исключение при недоступности
							if( !endPointIsAvailable && !omniIt.getProp("error_if_not_available").empty() )
							{
								ostringstream err;
								err << "Configuration: ERROR: endpoint '"
									<< param
									<< "' not available!";

								ucrit << err.str() << endl;
								throw Exception(err.str());
							}

							if( endPointIsAvailable )
							{
								uinfo << "(Configuration): add omniORB option '" << p << "' " << param << endl;
								omniParams.emplace_back( std::make_pair(p, param) );
							}
						}
						else
						{
							const string a(omniIt.getProp("arg"));
							uinfo << "(Configuration): add omniORB option '" << p << "' " << a << endl;
							omniParams.emplace_back( std::make_pair(p, a) );
						}
					}
				}
			}


			xmlNode* nsnode = getNode("NameService");

			// ---------------------------------------------------------------------------------
			// формируем options для ORB_init()
			// Прототип из документации на omniORB4: const char* options[][2] = { { "traceLevel", "1" }, { 0, 0 } };
			// --------------------------------------------------
			// + спискок узлов (сформированный из configure.xml)
			// + список параметров omniORB из секции <omniORB>
			// +1 для завершающего {0,0}
			int onum = lnodes.size() + omniParams.size() + 1;

			if( nsnode )
				onum += 1; // +1 --> IniRef NameService=

			const char* (*omni_options)[2] = new const char* [onum][2];

			int i = 0;

			// формируем новые, используя i в качестве индекса
			for( auto& it : lnodes )
			{
				// делаем uni_strdup чтобы потом не думая
				// "где мы выделяли, а где не мы"
				// делать delete[]
				omni_options[i][0] = uni_strdup("InitRef");

				string name(oind->getNodeName(it.id));
				ostringstream o;
				o << this << name;
				name = o.str();
				o << "=corbaname::" << it.host << ":" << it.port;
				omni_options[i][1] = uni_strdup(o.str());

				uinfo << "(Configuration): add omniORB option 'InitRef' (nodes) " << o.str() << endl;
				i++;

				ostringstream uri;
				uri << "corbaname::" << it.host << ":" << it.port;

				if( !omni::omniInitialReferences::setFromArgs(name.c_str(), uri.str().c_str()) )
					ucrit << "(Configuration): init omniInitialReferences: FAILED ADD name=" << name << " uri=" << uri.str() << endl;

				assert( i < onum );
			}

			for( auto& p : omniParams )
			{
				// делаем uni_strdup чтобы потом не думая
				// "где мы выделяли, а где не мы"
				// делать delete[]
				omni_options[i][0] = uni_strdup(p.first);
				omni_options[i][1] = uni_strdup(p.second);
				i++;
				assert( i < onum );
			}

			// initRef for NameService
			if( nsnode )
			{
				// делаем uni_strdup чтобы потом не думая
				// "где мы выделяли, а где не мы"
				// делать delete[]
				omni_options[i][0] = uni_strdup("InitRef");
				string defPort( getPort( getProp(nsnode, "port") ) );

				ostringstream param;
				param << this << "NameService=corbaname::" << getProp(nsnode, "host") << ":" << defPort;
				omni_options[i][1] = uni_strdup(param.str());
				uinfo << "(Configuration): add omniORB option 'InitRef' " << param.str() << endl;

				{
					ostringstream ns_name;
					ns_name << this << "NameService";
					ostringstream uri;
					uri << "corbaname::" << getProp(nsnode, "host") << ":" << defPort;

					if( !omni::omniInitialReferences::setFromArgs(ns_name.str().c_str(), uri.str().c_str()) )
						cerr << "**********************!!!! FAILED ADD name=" << ns_name.str() << " uri=" << uri.str() << endl;
				}

				i++;
			}
			else
				uwarn << "(Configuration): не нашли раздела 'NameService' \n";

			omni_options[i][0] = 0;
			omni_options[i][1] = 0;
			// ------------- CORBA INIT -------------
			// orb init
			orb = CORBA::ORB_init(_argc, (char**)_argv, "omniORB4", omni_options);

			// освобождаем память..
			for( int k = 0; k < onum; k++ )
			{
				// на самом деле последний элемент = {0,0}
				// но delete от 0 разрёшён и не приводит "к краху"
				// так что отдельно не обрабатываем этот случай.

				delete[] omni_options[k][0]; // см. uni_strdup()
				delete[] omni_options[k][1]; // см. uni_strdup()
			}

			delete[] omni_options;

			// create policy
			CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
			PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);

			if( transientIOR == false )
			{
				policyList.length(3);
				policyList[0] = root_poa->create_lifespan_policy(PortableServer::PERSISTENT);
				policyList[1] = root_poa->create_id_assignment_policy(PortableServer::USER_ID);
				policyList[2] = root_poa->create_request_processing_policy(PortableServer::USE_ACTIVE_OBJECT_MAP_ONLY);
				//            policyList[3] = root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
			}
			else
			{
				policyList.length(3);
				policyList[0] = root_poa->create_lifespan_policy(PortableServer::TRANSIENT);
				policyList[1] = root_poa->create_servant_retention_policy(PortableServer::RETAIN);
				policyList[2] = root_poa->create_request_processing_policy(PortableServer::USE_ACTIVE_OBJECT_MAP_ONLY);
				//            policyList[3] = root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
			}

			// ---------------------------------------

		}
		catch( const uniset::Exception& ex )
		{
			ucrit << "Configuration:" << ex << endl;
			throw;
		}
		catch(...)
		{
			ucrit << "Configuration: INIT FAILED!!!!" << endl;
			throw Exception("Configuration: INIT FAILED!!!!");
		}

		//    cerr << "*************** initConfiguration: " << pt.getCurrent() << " msec " << endl;
	}

	// -------------------------------------------------------------------------
	std::string Configuration::getArg2Param( const std::string& name, const std::string& defval, const std::string& defval2 ) const noexcept
	{
		string s(uniset::getArgParam(name, _argc, _argv, ""));

		if( !s.empty() )
			return s;

		if( !defval.empty() )
			return defval;

		return defval2;
	}

	string Configuration::getArgParam( const string& name, const string& defval ) const noexcept
	{
		return uniset::getArgParam(name, _argc, _argv, defval);
	}

	int Configuration::getArgInt( const string& name, const string& defval ) const noexcept
	{
		return uniset::uni_atoi(getArgParam( name, defval ));
	}

	int Configuration::getArgPInt( const string& name, int defval ) const noexcept
	{
		string param = getArgParam(name, "");

		if( param.empty() )
			return defval;

		return uniset::uni_atoi(param);
	}

	int Configuration::getArgPInt( const string& name, const string& strdefval, int defval ) const noexcept
	{
		string param = getArgParam(name, strdefval);

		if( param.empty() && strdefval.empty() )
			return defval;

		return uniset::uni_atoi(param);
	}


	// -------------------------------------------------------------------------
	void Configuration::initParameters()
	{
		xmlNode* root = unixml->findNode( unixml->getFirstNode(), "UniSet" );

		if( !root )
		{
			ucrit << "Configuration: INIT PARAM`s FAILED! <UniSet>...</UniSet> not found" << endl;
			throw uniset::SystemError("Configuration: INIT PARAM`s FAILED! <UniSet>...</UniSet> not found!");
		}

		UniXML::iterator it(root);

		if( !it.goChildren() )
		{
			ucrit << "Configuration: INIT PARAM`s FAILED!!!!" << endl;
			throw uniset::SystemError("Configuration: INIT PARAM`s FAILED!!!!");
		}

		for( ; it.getCurrent(); it.goNext() )
		{
			string name( it.getName() );

			if( name == "LocalNode" )
			{
				if( localNode == uniset::DefaultObjectId )
				{
					string nodename( it.getProp("name") );
					setLocalNode(nodename);
				}
			}
			else if( name == "LocalDBServer" )
			{
				name = it.getProp("name");
				//DBServer
				string secDB( getServicesSection() + "/" + name);
				localDBServer = oind->getIdByName(secDB);

				if( localDBServer == DefaultObjectId )
				{
					ostringstream msg;
					msg << "Configuration: DBServer '" << secDB << "' not found ServiceID in <services>!";
					ucrit << msg.str() << endl;
					throw uniset::SystemError(msg.str());
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
				imagesDir = dataDir + it.getProp("name") + "/"; // ????????
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

		// Heartbeat init...
		xmlNode* cnode = getNode("HeartBeatTime");

		if( cnode )
		{
			UniXML::iterator hit(cnode);
			heartbeat_msec = hit.getIntProp("msec");

			if( heartbeat_msec <= 0 )
				heartbeat_msec = 3000;
		}

		// NC ready timeout init...
		cnode = getNode("NCReadyTimeout");

		if( cnode )
		{
			UniXML::iterator hit(cnode);
			ncreadytimeout_msec = hit.getIntProp("msec");

			if( ncreadytimeout_msec < 0 )
				ncreadytimeout_msec = UniSetTimer::WaitUpTime;
			else if( ncreadytimeout_msec == 0 )
				ncreadytimeout_msec = 180000;
		}

		// startup ingore timeout init...
		cnode = getNode("StartUpIgnoreTimeout");

		if( cnode )
		{
			UniXML::iterator hit(cnode);
			startupIgnoretimeout_msec = hit.getIntProp("msec");

			if( startupIgnoretimeout_msec < 0 )
				startupIgnoretimeout_msec = UniSetTimer::WaitUpTime;
			else if( startupIgnoretimeout_msec == 0 )
				startupIgnoretimeout_msec = 5000;
		}
	}
	// -------------------------------------------------------------------------
	void Configuration::setLocalNode( const string& nodename )
	{
		localNode = oind->getIdByName(nodename);

		if( localNode == DefaultObjectId )
		{
			stringstream err;
			err << "(Configuration::setLocalNode): Not found node '" << nodename << "'";
			ucrit << err.str() << endl;
			throw uniset::SystemError(err.str());
		}

		localNodeName = nodename;
		oind->initLocalNode(localNode);
	}
	// -------------------------------------------------------------------------
	bool Configuration::checkOmniORBendPoint( const std::string& endPoint )
	{
		// проверяем доступность endPoint попыткой создать соединение
		auto ep = omni::giopEndpoint::str2Endpoint( endPoint.c_str() );

		if( !ep )
			return false;

		bool ret = false;

		try
		{
			ret = ep->Bind();

			if( ret )
				ep->Shutdown();
		}
		catch( std::exception& ex )
		{
			uwarn << "(Configuration::checkOmniORBendPoint): " << ex.what() << endl;
			ret = false;
		}

		ulogsys << "(Configuration::checkOmniORBendPoint): check " << endPoint << " "
				<< ( ret ? "OK" : "FAILED" )
				<< endl;

		return ret;
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::getNode(const string& path) const noexcept
	{
		return unixml->findNode(unixml->getFirstNode(), path);
	}
	// -------------------------------------------------------------------------
	string Configuration::getProp(xmlNode* node, const string& name) const noexcept
	{
		return UniXML::getProp(node, name);
	}
	int Configuration::getIntProp(xmlNode* node, const string& name) const noexcept
	{
		return UniXML::getIntProp(node, name);
	}
	int Configuration::getPIntProp(xmlNode* node, const string& name, int def) const noexcept
	{
		return UniXML::getPIntProp(node, name, def);
	}

	// -------------------------------------------------------------------------
	string Configuration::getField(const string& path) const noexcept
	{
		return getProp(getNode(path), "name");
	}

	// -------------------------------------------------------------------------
	int Configuration::getIntField(const std::string& path) const noexcept
	{
		return unixml->getIntProp(getNode(path), "name");
	}

	// -------------------------------------------------------------------------
	int Configuration::getPIntField(const std::string& path, int def) const noexcept
	{
		int i = getIntField(path);;

		if (i <= 0)
			return def;

		return i;
	}

	// -------------------------------------------------------------------------
	xmlNode* Configuration::findNode(xmlNode* node, const std::string& snode, const std::string& sname) const noexcept
	{
		if( !unixml->isOpen() )
			return 0;

		return unixml->findNode(node, snode, sname);
	}
	// -------------------------------------------------------------------------
	string Configuration::getRootDir() const noexcept
	{
		return rootDir;
	}
	// -------------------------------------------------------------------------
	int Configuration::getArgc() const noexcept
	{
		return _argc;
	}
	// -------------------------------------------------------------------------
	const char* const* Configuration::getArgv() const noexcept
	{
		return _argv;
	}
	// -------------------------------------------------------------------------
	ObjectId Configuration::getDBServer() const noexcept
	{
		return localDBServer;    /*!< получение идентификатора DBServer-а */
	}
	// -------------------------------------------------------------------------
	ObjectId Configuration::getLocalNode() const noexcept
	{
		return localNode;    /*!< получение идентификатора локального узла */
	}
	// -------------------------------------------------------------------------
	string Configuration::getLocalNodeName() const noexcept
	{
		return localNodeName;    /*!< получение название локального узла */
	}
	// -------------------------------------------------------------------------
	const string Configuration::getNSName() const noexcept
	{
		return NSName;
	}
	// -------------------------------------------------------------------------
	string Configuration::getRootSection() const noexcept
	{
		return secRoot;
	}
	// -------------------------------------------------------------------------
	string Configuration::getSensorsSection() const noexcept
	{
		return secSensors;
	}
	// -------------------------------------------------------------------------
	string Configuration::getObjectsSection() const noexcept
	{
		return secObjects;
	}
	// -------------------------------------------------------------------------
	string Configuration::getControllersSection() const noexcept
	{
		return secControlles;
	}
	// -------------------------------------------------------------------------
	string Configuration::getServicesSection() const noexcept
	{
		return secServices;
	}

	// -------------------------------------------------------------------------
	void Configuration::createNodesList()
	{
		xmlNode* omapnode = unixml->findNode(unixml->getFirstNode(), "ObjectsMap");

		if( !omapnode )
		{
			ucrit << "(Configuration): <ObjectsMap> not found!!!" << endl;
			throw uniset::SystemError("(Configuration): <ObjectsMap> not found!");
		}


		xmlNode* node = unixml->findNode(omapnode, "nodes");

		if(!node)
		{
			ucrit << "(Configuration): <nodes> section not found!" << endl;
			throw uniset::SystemError("(Configiuration): <nodes> section not found");
		}

		UniXML::iterator it(node);
		(void)it.goChildren();

		// определяем порт
		string defPort(getPort(unixml->getProp(node, "port")));

		lnodes.clear();

		for( ; it; it.goNext() )
		{
			string sname(getProp(it, "name"));

			if(sname.empty())
			{
				ucrit << "Configuration(createNodesList): unknown name='' in <nodes> " << endl;
				throw uniset::SystemError("Configuration(createNodesList: unknown name='' in <nodes>");
			}

			string nodename(sname);
			//         string virtnode = oind->getVirtualNodeName(nodename);
			//         if( virtnode.empty() )
			//             nodename = oind->mkFullNodeName(nodename,nodename);

			NodeInfo ninf;
			ninf.id = oind->getIdByName(nodename);

			if( ninf.id == DefaultObjectId )
			{
				ucrit << "Configuration(createNodesList): Not found ID for node '" << nodename << "'" << endl;
				throw uniset::SystemError("Configuration(createNodesList): Not found ID for node '" + nodename + "'");
			}

			ninf.host = getProp(it, "ip").c_str();
			string tp(getProp(it, "port"));

			if( tp.empty() )
				ninf.port = defPort.c_str();
			else
				ninf.port = tp.c_str();

			string tmp(it.getProp("dbserver"));

			if( tmp.empty() )
				ninf.dbserver = uniset::DefaultObjectId;
			else
			{
				string dname(getServicesSection() + "/" + tmp);
				ninf.dbserver = oind->getIdByName(dname);

				if( ninf.dbserver == DefaultObjectId )
				{
					ucrit << "Configuration(createNodesList): Not found ID for DBServer name='" << dname << "'" << endl;
					throw uniset::SystemError("Configuration(createNodesList: Not found ID for DBServer name='" + dname + "'");
				}
			}

			if( ninf.id == getLocalNode() )
				localDBServer = ninf.dbserver;

			ninf.connected = false;

			initNode(ninf, it);
			uinfo << "Configuration(createNodesList): add to list of nodes: node=" << nodename << " id=" << ninf.id << endl;
			lnodes.emplace_back( std::move(ninf) );
		}

		uinfo << "Configuration(createNodesList): size of node list " << lnodes.size() << endl;
	}
	// -------------------------------------------------------------------------
	void Configuration::initNode( uniset::NodeInfo& ninfo, UniXML::iterator& it ) noexcept
	{
		if( ninfo.id == getLocalNode() )
			ninfo.connected = true;
		else
			ninfo.connected = false;
	}
	// -------------------------------------------------------------------------
	string Configuration::getPropByNodeName(const string& nodename, const string& prop) const noexcept
	{
		xmlNode* node = getNode(nodename);

		if(node == NULL)
			return "";

		return getProp(node, prop);
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::initLogStream( std::shared_ptr<DebugStream> deb, const string& _debname ) noexcept
	{
		if( !deb )
			return NULL;

		return initLogStream( deb.get(), _debname );
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::initLogStream( DebugStream& deb, const string& _debname ) noexcept
	{
		return initLogStream(&deb, _debname);
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::initLogStream( DebugStream* deb, const string& _debname ) noexcept
	{
		if( !deb )
			return nullptr;

		if( _debname.empty() )
		{
			deb->any() << "(Configuration)(initLogStream): INIT DEBUG FAILED!!!" << endl;
			return nullptr;
		}


		string debname(_debname);

		xmlNode* dnode = getNode(_debname);

		if( dnode == NULL )
			deb->any() << "(Configuration)(initLogStream):  WARNING! Not found conf. section for log '" << _debname  << "'" << endl;
		else if( deb->getLogName().empty() )
		{
			if( !getProp(dnode, "name").empty() )
			{
				debname = getProp(dnode, "name");
				deb->setLogName(debname);
			}
		}

		string no_deb("--" + debname + "-no-debug");

		for (int i = 1; i < _argc; i++)
		{
			if( no_deb == _argv[i] )
			{
				deb->addLevel(Debug::NONE);
				return dnode;
			}
		}

		string debug_file("");

		// смотрим настройки файла
		if( dnode )
		{
			string conf_debug_levels(getProp(dnode, "levels"));

			if( !conf_debug_levels.empty() )
				deb->addLevel( Debug::value(conf_debug_levels) );
			else
				deb->addLevel(Debug::NONE);

			debug_file = getProp(dnode, "file");
		}

		// теперь смотрим командную строку
		string logfile("--" + debname + "-logfile");
		string add_level("--" + debname + "-add-levels");
		string del_level("--" + debname + "-del-levels");
		string show_msec("--" + debname + "-show-milliseconds");
		string show_usec("--" + debname + "-show-microseconds");

		// смотрим командную строку
		for (int i = 1; i < (_argc - 1); i++)
		{
			if( logfile == _argv[i] )        // "--debug-logfile"
			{
				debug_file = string(_argv[i + 1]);
			}
			else if( add_level == _argv[i] )    // "--debug-add-levels"
			{
				deb->addLevel(Debug::value(_argv[i + 1]));
			}
			else if( del_level == _argv[i] )    // "--debug-del-levels"
			{
				deb->delLevel(Debug::value(_argv[i + 1]));
			}
			else if( show_usec == _argv[i] )
			{
				deb->showMicroseconds(true);
			}
			else if( show_msec == _argv[i] )
			{
				deb->showMilliseconds(true);
			}
		}

		if( !debug_file.empty() )
			deb->logFile(debug_file);

		return dnode;
	}
	// -------------------------------------------------------------------------
	uniset::ListOfNode::const_iterator Configuration::listNodesBegin() const noexcept
	{
		return lnodes.begin();
	}
	// -------------------------------------------------------------------------
	uniset::ListOfNode::const_iterator Configuration::listNodesEnd() const noexcept
	{
		return lnodes.end();
	}
	// -------------------------------------------------------------------------
	const std::shared_ptr<UniXML> Configuration::getConfXML() const noexcept
	{
		return unixml;
	}
	// -------------------------------------------------------------------------
	CORBA::ORB_ptr Configuration::getORB() const
	{
		return CORBA::ORB::_duplicate(orb);
	}
	// -------------------------------------------------------------------------
	const CORBA::PolicyList Configuration::getPolicy() const noexcept
	{
		return policyList;
	}
	// -------------------------------------------------------------------------
	static std::string makeSecName( const std::string& sec, const std::string& name ) noexcept
	{
		ostringstream n;
		n << sec << "/" << name;
		return n.str();
	}
	// -------------------------------------------------------------------------
	void Configuration::initRepSections()
	{
		// Реализация под жёсткую структуру репозитория
		xmlNode* node( unixml->findNode(unixml->getFirstNode(), "RootSection") );

		if( node == NULL )
		{
			ostringstream msg;
			msg << "Configuration(initRepSections): Not found section <RootSection> in " << fileConfName;
			ucrit << msg.str() << endl;
			throw uniset::SystemError(msg.str());
		}

		secRoot       = unixml->getProp(node, "name");
		secSensors    = makeSecName(secRoot, getRepSectionName("sensors", xmlSensorsSec));
		secObjects    = makeSecName(secRoot, getRepSectionName("objects", xmlObjectsSec));
		secControlles = makeSecName(secRoot, getRepSectionName("controllers", xmlControllersSec));
		secServices   = makeSecName(secRoot, getRepSectionName("services", xmlServicesSec));
	}
	// -------------------------------------------------------------------------
	// второй параметр намеренно передаётся и переопредеяется
	// это просто такой способ вернуть и строку и указатель на узел (одним махом)
	string Configuration::getRepSectionName( const string& sec, xmlNode* secnode )
	{
		secnode = unixml->findNode(unixml->getFirstNode(), sec);

		if( secnode == NULL )
		{
			ostringstream msg;
			msg << "Configuration(initRepSections): Not found section '" << sec << "' in " << fileConfName;
			ucrit << msg.str() << endl;
			throw uniset::SystemError(msg.str());
		}

		string ret(unixml->getProp(secnode, "section"));

		if( ret.empty() )
			ret = unixml->getProp(secnode, "name");

		return ret;
	}
	// -------------------------------------------------------------------------
	void Configuration::setConfFileName( const string& fn )
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
			ucrit << msg.str();
			throw uniset::SystemError(msg.str());
		}
	}
	// -------------------------------------------------------------------------
	string Configuration::getPort( const string& port ) const noexcept
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
	ObjectId Configuration::getSensorID( const std::string& name ) const noexcept
	{
		if( name.empty() )
			return DefaultObjectId;

		return oind->getIdByName(getSensorsSection() + "/" + name);
	}
	// -------------------------------------------------------------------------
	ObjectId Configuration::getControllerID( const std::string& name ) const noexcept
	{
		if( name.empty() )
			return DefaultObjectId;

		return oind->getIdByName(getControllersSection() + "/" + name);
	}
	// -------------------------------------------------------------------------
	ObjectId Configuration::getObjectID( const std::string& name ) const noexcept
	{
		if( name.empty() )
			return DefaultObjectId;

		return oind->getIdByName(getObjectsSection() + "/" + name);
	}
	// -------------------------------------------------------------------------
	ObjectId Configuration::getServiceID( const std::string& name ) const noexcept
	{
		if( name.empty() )
			return DefaultObjectId;

		return oind->getIdByName(getServicesSection() + "/" + name);
	}
	// -------------------------------------------------------------------------
	uniset::ObjectId Configuration::getNodeID( const std::string& name ) const noexcept
	{
		if( name.empty() )
			return DefaultObjectId;

		return oind->getNodeId( name );
	}
	// -------------------------------------------------------------------------
	ObjectId Configuration::getAnyID( const string& name ) const noexcept
	{
		ObjectId id = DefaultObjectId;

		if( uniset::is_digit(name) )
			return uni_atoi(name);

		// ищем в <sensors>
		id = getSensorID(name);

		// ищем в <objects>
		if( id == DefaultObjectId )
			id = getObjectID(name);

		// ищем в <controllers>
		if( id == DefaultObjectId )
			id = getObjectID(name);

		// ищем в <nodes>
		if( id == DefaultObjectId )
			id = getNodeID(name);

		// ищем в <services>
		if( id == DefaultObjectId )
			id = getServiceID(name);

		return id;
	}
	// -------------------------------------------------------------------------
	const string Configuration::getConfFileName() const noexcept
	{
		return fileConfName;
	}
	// -------------------------------------------------------------------------
	string Configuration::getImagesDir() const noexcept
	{
		return imagesDir;    // временно
	}
	// -------------------------------------------------------------------------
	timeout_t Configuration::getHeartBeatTime() const noexcept
	{
		return heartbeat_msec;
	}

	timeout_t Configuration::getNCReadyTimeout() const noexcept
	{
		return ncreadytimeout_msec;
	}

	timeout_t Configuration::getStartupIgnoreTimeout() const noexcept
	{
		return startupIgnoretimeout_msec;
	}
	// -------------------------------------------------------------------------
	const string Configuration::getConfDir() const noexcept
	{
		return confDir;
	}

	const string Configuration::getDataDir() const noexcept
	{
		return dataDir;
	}

	const string Configuration::getBinDir() const noexcept
	{
		return binDir;
	}

	const string Configuration::getLogDir() const noexcept
	{
		return logDir;
	}

	const string Configuration::getLockDir() const noexcept
	{
		return lockDir;
	}

	const string Configuration::getDocDir() const noexcept
	{
		return docDir;
	}

	bool Configuration::isLocalIOR() const noexcept
	{
		return localIOR;
	}

	bool Configuration::isTransientIOR() const noexcept
	{
		return transientIOR;
	}

	// -------------------------------------------------------------------------
	xmlNode* Configuration::getXMLSensorsSection() noexcept
	{
		if( xmlSensorsSec )
			return xmlSensorsSec;

		xmlSensorsSec = unixml->findNode(unixml->getFirstNode(), "sensors");
		return xmlSensorsSec;
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::getXMLObjectsSection() noexcept
	{
		if( xmlObjectsSec )
			return xmlObjectsSec;

		xmlObjectsSec = unixml->findNode(unixml->getFirstNode(), "objects");
		return xmlObjectsSec;
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::getXMLControllersSection() noexcept
	{
		if( xmlControllersSec )
			return xmlControllersSec;

		xmlControllersSec = unixml->findNode(unixml->getFirstNode(), "controllers");
		return xmlControllersSec;

	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::getXMLServicesSection() noexcept
	{
		if( xmlServicesSec )
			return xmlServicesSec;

		xmlServicesSec = unixml->findNode(unixml->getFirstNode(), "services");
		return xmlServicesSec;
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::getXMLNodesSection() noexcept
	{
		if( xmlNodesSec )
			return xmlNodesSec;

		xmlNodesSec = unixml->findNode(unixml->getFirstNode(), "nodes");
		return xmlNodesSec;
	}
	// -------------------------------------------------------------------------
	xmlNode* Configuration::getXMLObjectNode( uniset::ObjectId id ) const noexcept
	{
		const ObjectInfo* i = oind->getObjectInfo(id);

		if( i )
			return i->xmlnode;

		return 0;
	}
	// -------------------------------------------------------------------------
	UniversalIO::IOType Configuration::getIOType( uniset::ObjectId id ) const noexcept
	{
		const ObjectInfo* i = oind->getObjectInfo(id);

		if( i && i->xmlnode )
		{
			UniXML::iterator it(i->xmlnode);
			return uniset::getIOType( it.getProp("iotype") );
		}

		return UniversalIO::UnknownIOType;
	}
	// -------------------------------------------------------------------------
	UniversalIO::IOType Configuration::getIOType( const std::string& name ) const noexcept
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

		if( i && i->xmlnode )
		{
			UniXML::iterator it(i->xmlnode);
			return uniset::getIOType( it.getProp("iotype") );
		}

		return UniversalIO::UnknownIOType;
	}
	// -------------------------------------------------------------------------
	size_t Configuration::getCountOfNet() const noexcept
	{
		return countOfNet;
	}
	// -------------------------------------------------------------------------
	timeout_t Configuration::getRepeatTimeout() const noexcept
	{
		return repeatTimeout;
	}
	// -------------------------------------------------------------------------
	size_t Configuration::getRepeatCount() const noexcept
	{
		return repeatCount;
	}
	// -------------------------------------------------------------------------
	std::shared_ptr<Configuration> uniset_init( int argc, const char* const* argv, const std::string& xmlfile )
	{
		if( uniset::uconf )
		{
			cerr << "Reusable call uniset_init... ignore.." << endl;
			return uniset::uconf;
		}

		atexit( UniSetActivator::normalexit );
		set_terminate( UniSetActivator::normalterminate ); // ловушка для неизвестных исключений

		string confile = uniset::getArgParam( "--confile", argc, argv, xmlfile );
		uniset::uconf = make_shared<Configuration>(argc, argv, confile);

		return uniset::uconf;
	}

	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
} // end of UniSetTypes namespace
