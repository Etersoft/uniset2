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
#include "PassiveTimer.h" // for timeout_t
#include "IORFile.h"
#include "Debug.h"
// --------------------------------------------------------------------------
/*
    В функции main нужно обязательно вызывать uniset::uniset_init(argc,argv);
*/
namespace uniset
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
						   const std::string& fileConf, uniset::ObjectInfo* objectsMap );

			/// Получить значение полей с путём path
			std::string getField(const std::string& path) const noexcept;
			/// Получить число из поле с путём path
			int getIntField(const std::string& path) const noexcept;
			/// Получить число из поле с путём path (или def, если значение <= 0)
			int getPIntField(const std::string& path, int def) const noexcept;

			xmlNode* findNode(xmlNode* node, const std::string& searchnode, const std::string& name = "" ) const noexcept;

			// Получить узел
			xmlNode* getNode(const std::string& path) const noexcept;

			// Получить указанное свойство пути
			std::string getProp(xmlNode*, const std::string& name) const noexcept;
			int getIntProp(xmlNode*, const std::string& name) const noexcept;
			int getPIntProp(xmlNode*, const std::string& name, int def) const noexcept;

			// Получить указанное свойство по имени узла
			std::string getPropByNodeName(const std::string& nodename, const std::string& prop) const noexcept;

			std::string getRootDir() const noexcept; /*!< Получение каталога, в котором находится выполняющаяся программа */
			int getArgc() const noexcept;
			const char* const* getArgv() const noexcept;
			ObjectId getDBServer() const noexcept;
			ObjectId getLocalNode() const noexcept;
			std::string getLocalNodeName() const noexcept;
			const std::string getNSName() const noexcept;

			// repository
			std::string getRootSection() const noexcept;
			std::string getSensorsSection() const noexcept;
			std::string getObjectsSection() const noexcept;
			std::string getControllersSection() const noexcept;
			std::string getServicesSection() const noexcept;
			// xml
			xmlNode* getXMLSensorsSection() noexcept;
			xmlNode* getXMLObjectsSection() noexcept;
			xmlNode* getXMLControllersSection() noexcept;
			xmlNode* getXMLServicesSection()  noexcept;
			xmlNode* getXMLNodesSection() noexcept;
			xmlNode* getXMLObjectNode( uniset::ObjectId ) const noexcept;

			UniversalIO::IOType getIOType( uniset::ObjectId ) const noexcept;
			UniversalIO::IOType getIOType( const std::string& name ) const noexcept;

			// net
			size_t getCountOfNet() const noexcept;
			size_t getRepeatTimeout() const noexcept;
			size_t getRepeatCount() const noexcept;

			uniset::ObjectId getSensorID( const std::string& name ) const noexcept;
			uniset::ObjectId getControllerID( const std::string& name ) const noexcept;
			uniset::ObjectId getObjectID( const std::string& name ) const noexcept;
			uniset::ObjectId getServiceID( const std::string& name ) const noexcept;
			uniset::ObjectId getNodeID( const std::string& name ) const noexcept;

			const std::string getConfFileName() const noexcept;
			std::string getImagesDir() const noexcept;

			timeout_t getHeartBeatTime() const noexcept;

			// dirs
			const std::string getConfDir() const noexcept;
			const std::string getDataDir() const noexcept;
			const std::string getBinDir() const noexcept;
			const std::string getLogDir() const noexcept;
			const std::string getLockDir() const noexcept;
			const std::string getDocDir() const noexcept;

			bool isLocalIOR() const noexcept;
			bool isTransientIOR() const noexcept;

			/*! получить значение указанного параметра, или значение по умолчанию */
			std::string getArgParam(const std::string& name, const std::string& defval = "") const noexcept;

			/*! получить значение, если пустое, то defval, если defval="" return defval2 */
			std::string getArg2Param(const std::string& name, const std::string& defval, const std::string& defval2 = "") const noexcept;

			/*! получить числовое значение параметра, если не число, то 0. Если параметра нет, используется значение defval */
			int getArgInt(const std::string& name, const std::string& defval = "") const noexcept;

			/*! получить числовое значение параметра, но если оно не положительное, вернуть defval */
			int getArgPInt(const std::string& name, int defval) const noexcept;
			int getArgPInt(const std::string& name, const std::string& strdefval, int defval) const noexcept;

			xmlNode* initLogStream( DebugStream& deb, const std::string& nodename ) noexcept;
			xmlNode* initLogStream( std::shared_ptr<DebugStream> deb, const std::string& nodename ) noexcept;
			xmlNode* initLogStream( DebugStream* deb, const std::string& nodename ) noexcept;

			uniset::ListOfNode::const_iterator listNodesBegin() const noexcept;
			uniset::ListOfNode::const_iterator listNodesEnd() const noexcept;

			/*! интерфейс к карте объектов */
			std::shared_ptr<ObjectIndex> oind;

			/*! интерфейс к работе с локальнымми ior-файлами */
			std::shared_ptr<IORFile> iorfile;

			/*! указатель на конфигурационный xml */
			const std::shared_ptr<UniXML> getConfXML() const noexcept;

			CORBA::ORB_ptr getORB() const;
			const CORBA::PolicyList getPolicy() const noexcept;

		protected:
			Configuration();

			virtual void initConfiguration(int argc, const char* const* argv);

			void createNodesList();
			virtual void initNode( uniset::NodeInfo& ninfo, UniXML::iterator& it) noexcept;

			void initRepSections();
			std::string getRepSectionName(const std::string& sec, xmlNode* secnode = 0 );
			void setConfFileName( const std::string& fn = "" );
			void initParameters();
			void setLocalNode( const std::string& nodename );

			std::string getPort( const std::string& port = "" ) const noexcept;

			std::string rootDir = { "" };
			std::shared_ptr<UniXML> unixml;

			int _argc = { 0 };
			const char** _argv = { nullptr };
			CORBA::ORB_var orb;
			CORBA::PolicyList policyList;

			std::string NSName = { "" };  /*!< имя сервиса именования на данной машине (обычно "NameService") */
			size_t countOfNet = { 1 };    /*!< количество резервных каналов */
			size_t repeatCount = { 3 };   /*!< количество попыток получить доступ к удаленному объекту
                                            прежде чем будет выработано исключение TimeOut.        */

			timeout_t repeatTimeout = { 50 };    /*!< пауза между попытками [мс] */

			uniset::ListOfNode lnodes;

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

			ObjectId localDBServer = { uniset::DefaultObjectId };
			ObjectId localNode = { uniset::DefaultObjectId };

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
	std::shared_ptr<Configuration> uniset_conf() noexcept;

	/*! Глобальный объект для вывода логов */
	std::shared_ptr<DebugStream> ulog() noexcept;

	/*! инициализация "глобальной" конфигурации */
	std::shared_ptr<Configuration> uniset_init( int argc, const char* const* argv, const std::string& xmlfile = "configure.xml" );
// --------------------------------------------------------------------------
}    // end of uniset namespace
// --------------------------------------------------------------------------
// "синтаксический сахар"..для логов
#define uinfo if( uniset::ulog()->debugging(Debug::INFO) ) uniset::ulog()->info()
#define uwarn if( uniset::ulog()->debugging(Debug::WARN) ) uniset::ulog()->warn()
#define ucrit if( uniset::ulog()->debugging(Debug::CRIT) ) uniset::ulog()->crit()
#define ulog1 if( uniset::ulog()->debugging(Debug::LEVEL1) ) uniset::ulog()->level1()
#define ulog2 if( uniset::ulog()->debugging(Debug::LEVEL2) ) uniset::ulog()->level2()
#define ulog3 if( uniset::ulog()->debugging(Debug::LEVEL3) ) uniset::ulog()->level3()
#define ulog4 if( uniset::ulog()->debugging(Debug::LEVEL4) ) uniset::ulog()->level4()
#define ulog5 if( uniset::ulog()->debugging(Debug::LEVEL5) ) uniset::ulog()->level5()
#define ulog6 if( uniset::ulog()->debugging(Debug::LEVEL6) ) uniset::ulog()->level6()
#define ulog7 if( uniset::ulog()->debugging(Debug::LEVEL7) ) uniset::ulog()->level7()
#define ulog8 if( uniset::ulog()->debugging(Debug::LEVEL8) ) uniset::ulog()->level8()
#define ulog9 if( uniset::ulog()->debugging(Debug::LEVEL9) ) uniset::ulog()->level9()
#define ulogsys if( uniset::ulog()->debugging(Debug::SYSTEM) ) uniset::ulog()->system()
#define ulogrep if( uniset::ulog()->debugging(Debug::REPOSITORY) ) uniset::ulog()->repository()
#define ulogany uniset::ulog()->any()
// --------------------------------------------------------------------------
#endif // Configuration_H_
