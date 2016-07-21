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
#ifndef Configuration_H_
#define Configuration_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include <ostream>
#include "UniXML.h"
#include "UniSetTypes.h"
#include "ObjectIndex.h"
#include "IORFile.h"
#include "Debug.h"
// --------------------------------------------------------------------------
/*
    В функции main нужно обязательно вызывать UniSetTypes::uniset_init(argc,argv);
*/
namespace UniSetTypes
{
	/*!
	     Конфигуратор системы
	     \note В случае обнаружения критической ошибки в настроечном файле файле.
	     Вырабатывает исключение и прекращает работу.
	*/
	class Configuration
	{
		public:
			virtual ~Configuration();

			/*! конфигурирование xml-файлом ( предпочтительный способ )    */
			Configuration( int argc, const char* const* argv, const std::string& xmlfile = "" );

			/*! конфигурирование xml-файлом    */
			Configuration( int argc, const char* const* argv, std::shared_ptr<ObjectIndex> oind, const std::string& xmlfile = "" );

			/*! устаревший вариант, для поддержки старых проектов */
			Configuration( int argc, const char* const* argv,
						   const std::string& fileConf, UniSetTypes::ObjectInfo* objectsMap );

			/// Получить значение полей с путём path
			std::string getField(const std::string& path);
			/// Получить число из поле с путём path
			int getIntField(const std::string& path);
			/// Получить число из поле с путём path (или def, если значение <= 0)
			int getPIntField(const std::string& path, int def);

			xmlNode* findNode(xmlNode* node, const std::string& searchnode, const std::string& name = "" );

			// Получить узел
			xmlNode* getNode(const std::string& path);
			// Получить указанное свойство пути
			std::string getProp(xmlNode*, const std::string& name);
			int getIntProp(xmlNode*, const std::string& name);
			int getPIntProp(xmlNode*, const std::string& name, int def);
			// Получить указанное свойство по имени узла
			std::string getPropByNodeName(const std::string& nodename, const std::string& prop);

			static std::ostream& help(std::ostream& os);

			std::string getRootDir(); /*!< Получение каталога, в котором находится выполняющаяся программа */
			inline int getArgc()const
			{
				return _argc;
			}
			inline const char* const* getArgv() const
			{
				return _argv;
			}
			inline ObjectId getDBServer() const
			{
				return localDBServer;    /*!< получение идентификатора DBServer-а */
			}
			inline ObjectId getLocalNode() const
			{
				return localNode;    /*!< получение идентификатора локального узла */
			}
			inline std::string getLocalNodeName() const
			{
				return localNodeName;    /*!< получение название локального узла */
			}
			inline const std::string getNSName() const
			{
				return NSName;
			}

			// repository
			inline std::string getRootSection() const
			{
				return secRoot;
			}
			inline std::string getSensorsSection() const
			{
				return secSensors;
			}
			inline std::string getObjectsSection() const
			{
				return secObjects;
			}
			inline std::string getControllersSection() const
			{
				return secControlles;
			}
			inline std::string getServicesSection() const
			{
				return secServices;
			}
			// xml
			xmlNode* getXMLSensorsSection();
			xmlNode* getXMLObjectsSection();
			xmlNode* getXMLControllersSection();
			xmlNode* getXMLServicesSection();
			xmlNode* getXMLNodesSection();
			xmlNode* getXMLObjectNode( UniSetTypes::ObjectId );
			UniversalIO::IOType getIOType( UniSetTypes::ObjectId );
			UniversalIO::IOType getIOType( const std::string& name );

			// net
			inline size_t getCountOfNet() const
			{
				return countOfNet;
			}
			inline size_t getRepeatTimeout() const
			{
				return repeatTimeout;
			}
			inline size_t getRepeatCount() const
			{
				return repeatCount;
			}

			UniSetTypes::ObjectId getSensorID( const std::string& name );
			UniSetTypes::ObjectId getControllerID( const std::string& name );
			UniSetTypes::ObjectId getObjectID( const std::string& name );
			UniSetTypes::ObjectId getServiceID( const std::string& name );
			UniSetTypes::ObjectId getNodeID( const std::string& name );

			inline const std::string getConfFileName() const
			{
				return fileConfName;
			}
			inline std::string getImagesDir() const
			{
				return imagesDir;    // временно
			}

			inline timeout_t getHeartBeatTime()
			{
				return heartbeat_msec;
			}

			// dirs
			inline const std::string getConfDir() const
			{
				return confDir;
			}
			inline const std::string getDataDir() const
			{
				return dataDir;
			}
			inline const std::string getBinDir() const
			{
				return binDir;
			}
			inline const std::string getLogDir() const
			{
				return logDir;
			}
			inline const std::string getLockDir() const
			{
				return lockDir;
			}
			inline const std::string getDocDir() const
			{
				return docDir;
			}

			inline bool isLocalIOR() const
			{
				return localIOR;
			}
			inline bool isTransientIOR() const
			{
				return transientIOR;
			}

			/*! получить значение указанного параметра, или значение по умолчанию */
			std::string getArgParam(const std::string& name, const std::string& defval = "");
			/*! получить значение, если пустое, то defval, если defval="" return defval2 */
			std::string getArg2Param(const std::string& name, const std::string& defval, const std::string& defval2 = "");
			/*! получить числовое значение параметра, если не число, то 0. Если параметра нет, используется значение defval */
			int getArgInt(const std::string& name, const std::string& defval = "");
			/*! получить числовое значение параметра, но если оно не положительное, вернуть defval */
			int getArgPInt(const std::string& name, int defval);
			int getArgPInt(const std::string& name, const std::string& strdefval, int defval);

			xmlNode* initLogStream( DebugStream& deb, const std::string& nodename );
			xmlNode* initLogStream( std::shared_ptr<DebugStream> deb, const std::string& nodename );
			xmlNode* initLogStream( DebugStream* deb, const std::string& nodename );

			UniSetTypes::ListOfNode::const_iterator listNodesBegin()
			{
				return lnodes.begin();
			}

			inline UniSetTypes::ListOfNode::const_iterator listNodesEnd()
			{
				return lnodes.end();
			}

			/*! интерфейс к карте объектов */
			std::shared_ptr<ObjectIndex> oind;

			/*! интерфейс к работе с локальнымми ior-файлами */
			std::shared_ptr<IORFile> iorfile;

			/*! указатель на конфигурационный xml */
			inline const std::shared_ptr<UniXML> getConfXML() const
			{
				return unixml;
			}

			inline CORBA::ORB_ptr getORB() const
			{
				return CORBA::ORB::_duplicate(orb);
			}
			inline const CORBA::PolicyList getPolicy() const
			{
				return policyList;
			}

		protected:
			Configuration();

			virtual void initConfiguration(int argc, const char* const* argv);

			void createNodesList();
			virtual void initNode( UniSetTypes::NodeInfo& ninfo, UniXML::iterator& it);

			void initRepSections();
			std::string getRepSectionName(const std::string& sec, xmlNode* secnode = 0 );
			void setConfFileName( const std::string& fn = "" );
			void initParameters();
			void setLocalNode( const std::string& nodename );

			std::string getPort( const std::string& port = "" );

			std::string rootDir = { "" };
			std::shared_ptr<UniXML> unixml;

			int _argc = { 0 };
			const char* const* _argv;
			CORBA::ORB_var orb;
			CORBA::PolicyList policyList;

			std::string NSName = { "" };        /*!< имя сервиса именования на ланной машине (обычно "NameService") */
			size_t countOfNet = { 1 };    /*!< количество резервных каналов */
			size_t repeatCount = { 3 };    /*!< количество попыток получить доступ к удаленному объекту
                                            прежде чем будет выработано исключение TimeOut.        */

			timeout_t repeatTimeout = { 50 };    /*!< пауза между попытками [мс] */

			UniSetTypes::ListOfNode lnodes;

			// repository
			std::string secRoot = { "" };
			std::string secSensors = { "" };
			std::string secObjects = { "" };
			std::string secControlles = { "" };
			std::string secServices = { "" };

			// xml
			xmlNode* xmlSensorsSec = { 0 };
			xmlNode* xmlObjectsSec = { 0 };
			xmlNode* xmlControllersSec = { 0 };
			xmlNode* xmlServicesSec = { 0 };
			xmlNode* xmlNodesSec = { 0 };

			ObjectId localDBServer = { UniSetTypes::DefaultObjectId };
			ObjectId localNode = { UniSetTypes::DefaultObjectId };

			std::string localNodeName = { "" };
			std::string fileConfName = { "" };
			std::string imagesDir = { "" };

			std::string confDir = { "" };
			std::string dataDir = { "" };
			std::string binDir = { "" };
			std::string logDir = { "" };
			std::string docDir = { "" };
			std::string lockDir = { "" };
			bool localIOR = { false };
			bool transientIOR = { false };

			timeout_t heartbeat_msec = { 3000 };
	};

	/*! Глобальный указатель на конфигурацию (singleton) */
	std::shared_ptr<Configuration> uniset_conf();

	/*! Глобальный объект для вывода логов */
	std::shared_ptr<DebugStream> ulog();

	/*! инициализация "глобальной" конфигурации */
	std::shared_ptr<Configuration> uniset_init( int argc, const char* const* argv, const std::string& xmlfile = "configure.xml" );

}    // end of UniSetTypes namespace
// --------------------------------------------------------------------------
// "синтаксический сахар"..для логов
#define uinfo if( UniSetTypes::ulog()->debugging(Debug::INFO) ) UniSetTypes::ulog()->info()
#define uwarn if( UniSetTypes::ulog()->debugging(Debug::WARN) ) UniSetTypes::ulog()->warn()
#define ucrit if( UniSetTypes::ulog()->debugging(Debug::CRIT) ) UniSetTypes::ulog()->crit()
#define ulog1 if( UniSetTypes::ulog()->debugging(Debug::LEVEL1) ) UniSetTypes::ulog()->level1()
#define ulog2 if( UniSetTypes::ulog()->debugging(Debug::LEVEL2) ) UniSetTypes::ulog()->level2()
#define ulog3 if( UniSetTypes::ulog()->debugging(Debug::LEVEL3) ) UniSetTypes::ulog()->level3()
#define ulog4 if( UniSetTypes::ulog()->debugging(Debug::LEVEL4) ) UniSetTypes::ulog()->level4()
#define ulog5 if( UniSetTypes::ulog()->debugging(Debug::LEVEL5) ) UniSetTypes::ulog()->level5()
#define ulog6 if( UniSetTypes::ulog()->debugging(Debug::LEVEL6) ) UniSetTypes::ulog()->level6()
#define ulog7 if( UniSetTypes::ulog()->debugging(Debug::LEVEL7) ) UniSetTypes::ulog()->level7()
#define ulog8 if( UniSetTypes::ulog()->debugging(Debug::LEVEL8) ) UniSetTypes::ulog()->level8()
#define ulog9 if( UniSetTypes::ulog()->debugging(Debug::LEVEL9) ) UniSetTypes::ulog()->level9()
#define ulogsys if( UniSetTypes::ulog()->debugging(Debug::SYSTEM) ) UniSetTypes::ulog()->system()
#define ulogrep if( UniSetTypes::ulog()->debugging(Debug::REPOSITORY) ) UniSetTypes::ulog()->repository()
#define ulogany UniSetTypes::ulog()->any()
// --------------------------------------------------------------------------
#endif // Configuration_H_
