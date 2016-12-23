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
 *  \brief файл реализации DB-сервера
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------

#include <sys/time.h>
#include <sstream>
#include <iomanip>

#include "ORepHelpers.h"
#include "DBServer.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
#include "DBLogSugar.h"
// ------------------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// ------------------------------------------------------------------------------------------
DBServer::DBServer( ObjectId id, const std::string& _prefix ):
	UniSetObject(id),
	prefix(_prefix)
{
	if( getId() == DefaultObjectId )
	{
		id = uniset_conf()->getDBServer();

		if( id == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(DBServer): Запуск невозможен! НЕ ОПРЕДЕЛЁН ObjectId !!!!!\n";
			throw Exception(msg.str());
		}

		setID(id);
	}

	auto conf = uniset_conf();

	dblog = make_shared<DebugStream>();
	dblog->setLogName(myname);
	conf->initLogStream(dblog, prefix + "-log");

	loga = make_shared<LogAgregator>();
	loga->add(dblog);
	loga->add(ulog());

	xmlNode* cnode = conf->getNode("LocalDBServer");

	UniXML::iterator it(cnode);

	logserv = make_shared<LogServer>(loga);
	logserv->init( prefix + "-logserver", cnode );

	if( findArgParam("--" + prefix + "-run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
	{
		logserv_host = conf->getArg2Param("--" + prefix + "-logserver-host", it.getProp("logserverHost"), "localhost");
		logserv_port = conf->getArgPInt("--" + prefix + "-logserver-port", it.getProp("logserverPort"), getId());
	}
}

DBServer::DBServer( const std::string& prefix ):
	DBServer(uniset_conf()->getDBServer(), prefix )
{
}
//--------------------------------------------------------------------------------------------
DBServer::~DBServer()
{
}
//--------------------------------------------------------------------------------------------
void DBServer::processingMessage( const uniset::VoidMessage* msg )
{
	switch(msg->type)
	{
		case Message::Confirm:
			confirmInfo( reinterpret_cast<const ConfirmMessage*>(msg) );
			break;

		default:
			UniSetObject::processingMessage(msg);
			break;
	}

}
//--------------------------------------------------------------------------------------------
bool DBServer::activateObject()
{
	UniSetObject::activateObject();
	initDBServer();
	return true;
}
//--------------------------------------------------------------------------------------------
void DBServer::sysCommand( const uniset::SystemMessage* sm )
{
	UniSetObject::sysCommand(sm);

	if(  sm->command == SystemMessage::StartUp )
	{
		if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
		{
			dbinfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
			logserv->run(logserv_host, logserv_port, true);
		}
	}
}
//--------------------------------------------------------------------------------------------
std::string DBServer::help_print()
{
	ostringstream h;

	h << " Logs: " << endl;
	h << "--prefix-log-...            - log control" << endl;
	h << "             add-levels ..." << endl;
	h << "             del-levels ..." << endl;
	h << "             set-levels ..." << endl;
	h << "             logfile filaname" << endl;
	h << "             no-debug " << endl;
	h << " LogServer: " << endl;
	h << "--prefix-run-logserver       - run logserver. Default: localhost:id" << endl;
	h << "--prefix-logserver-host ip   - listen ip. Default: localhost" << endl;
	h << "--prefix-logserver-port num  - listen port. Default: ID" << endl;
	h << LogServer::help_print("prefix-logserver") << endl;

	return h.str();
}
//--------------------------------------------------------------------------------------------
