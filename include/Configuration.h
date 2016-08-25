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

			static std::ostream& help(std::ostream& os);

			/*! конфигурирование xml-файлом ( предпочтительный способ )    */
			Configuration( int argc, const char* const* argv, const std::string& xmlfile = "" );

			/*! конфигурирование xml-файлом    */
			Configuration( int argc, const char* const* argv, std::shared_ptr<ObjectIndex> oind, const std::string& xmlfile = "" );

			/*! устаревший вариант, для поддержки старых проектов */
			Configuration( int argc, const char* const* argv,
						   const std::string& fileConf, UniSetTypes::ObjectInfo* objectsMap );

			/// Получить значение полей с путём path
			std::string getField(const std::string& path) const;
			/// Получить число из поле с путём path
			int getIntField(const std::string& path) const;
			/// Получить число из поле с путём path (или def, если значение <= 0)
			int getPIntField(const std::string& path, int def) const;

			xmlNode* findNode(xmlNode* node, const std::string& searchnode, const std::string& name = "" ) const;

			// Получить узел
			xmlNode* getNode(const std::string& path) const;

			// Получить указанное свойство пути
			std::string getProp(xmlNode*, const std::string& name) const;
			int getIntProp(xmlNode*, const std::string& name) const;
			int getPIntProp(xmlNode*, const std::string& name, int def) const;

			// Получить указанное свойство по имени узла
			std::string getPropByNodeName(const std::string& nodename, const std::string& prop) const;

			std::string getRootDir() const; /*!< Получение каталога, в котором находится выполняющаяся программа */
			int getArgc() const;
			const char* const* getArgv() const;
			ObjectId getDBServer() const;
			ObjectId getLocalNode() const;
			std::string getLocalNodeName() const;
			const std::string getNSName() const;

			// repository
			std::string getRootSection() const;
			std::string getSensorsSection() const;
			std::string getObjectsSection() const;
			std::string getControllersSection() const;
			std::string getServicesSection() const;
			// xml
			xmlNode* getXMLSensorsSection();
			xmlNode* getXMLObjectsSection();
			xmlNode* getXMLControllersSection();
			xmlNode* getXMLServicesSection();
			xmlNode* getXMLNodesSection();
			xmlNode* getXMLObjectNode( UniSetTypes::ObjectId );

			UniversalIO::IOType getIOType( UniSetTypes::ObjectId ) const;
			UniversalIO::IOType getIOType( const std::string& name ) const;

			// net
			size_t getCountOfNet() const;
			size_t getRepeatTimeout() const;
			size_t getRepeatCount() const;

			UniSetTypes::ObjectId getSensorID( const std::string& name ) const;
			UniSetTypes::ObjectId getControllerID( const std::string& name ) const;
			UniSetTypes::ObjectId getObjectID( const std::string& name ) const;
			UniSetTypes::ObjectId getServiceID( const std::string& name ) const;
			UniSetTypes::ObjectId getNodeID( const std::string& name ) const;

			const std::string getConfFileName() const;
			std::string getImagesDir() const;

			timeout_t getHeartBeatTime() const;

			// dirs
			const std::string getConfDir() const;
			const std::string getDataDir() const;
			const std::string getBinDir() const;
			const std::string getLogDir() const;
			const std::string getLockDir() const;
			const std::string getDocDir() const;

			bool isLocalIOR() const;
			bool isTransientIOR() const;

			/*! получить значение указанного параметра, или значение по умолчанию */
			std::string getArgParam(const std::string& name, const std::string& defval = "") const;
			/*! получить значение, если пустое, то defval, если defval="" return defval2 */
			std::string getArg2Param(const std::string& name, const std::string& defval, const std::string& defval2 = "") const;
			/*! получить числовое значение параметра, если не число, то 0. Если параметра нет, используется значение defval */
			int getArgInt(const std::string& name, const std::string& defval = "") const;
			/*! получить числовое значение параметра, но если оно не положительное, вернуть defval */
			int getArgPInt(const std::string& name, int defval) const;
			int getArgPInt(const std::string& name, const std::string& strdefval, int defval) const;

			xmlNode* initLogStream( DebugStream& deb, const std::string& nodename );
			xmlNode* initLogStream( std::shared_ptr<DebugStream> deb, const std::string& nodename );
			xmlNode* initLogStream( DebugStream* deb, const std::string& nodename );

			UniSetTypes::ListOfNode::const_iterator listNodesBegin();
			UniSetTypes::ListOfNode::const_iterator listNodesEnd();

			/*! интерфейс к карте объектов */
			std::shared_ptr<ObjectIndex> oind;

			/*! интерфейс к работе с локальнымми ior-файлами */
			std::shared_ptr<IORFile> iorfile;

			/*! указатель на конфигурационный xml */
			const std::shared_ptr<UniXML> getConfXML() const;

			CORBA::ORB_ptr getORB() const;
			const CORBA::PolicyList getPolicy() const;

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

			std::string getPort( const std::string& port = "" ) const;

			std::string rootDir = { "" };
			std::shared_ptr<UniXML> unixml;

			int _argc = { 0 };
			const char* const* _argv;
			CORBA::ORB_var orb;
			CORBA::PolicyList policyList;

			std::string NSName = { "" };  /*!< имя сервиса именования на данной машине (обычно "NameService") */
			size_t countOfNet = { 1 };    /*!< количество резервных каналов */
			size_t repeatCount = { 3 };   /*!< количество попыток получить доступ к удаленному объекту
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
