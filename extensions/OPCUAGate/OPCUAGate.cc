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
#include <open62541/server.h>
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

//    UniXML::iterator it(cnode);

    uaServer = UA_Server_new();
    if( uaServer == nullptr )
        throw SystemError("don't create UA_Server");

    auto srvconfig = UA_Server_getConfig(uaServer);
    auto ret = UA_ServerConfig_setDefault(srvconfig);
    if( ret == EXIT_FAILURE )
        throw SystemError("don't condif UA_Server");

    srvconfig->shutdownDelay = 5000.0; /* 5s */

    serverThread = unisetstd::make_unique<ThreadCreator<OPCUAGate>>(this, &OPCUAGate::serverLoop);
    serverThread->setFinalAction(this, &OPCUAGate::serverLoopTerminate);
}
// -----------------------------------------------------------------------------
OPCUAGate::~OPCUAGate()
{
    UA_Server_delete(uaServer);
}
// -----------------------------------------------------------------------------
void OPCUAGate::step()
{
}
//--------------------------------------------------------------------------------
void OPCUAGate::sysCommand( const uniset::SystemMessage* sm )
{
    UObject_SK::sysCommand(sm);
    if( sm->command == SystemMessage::StartUp )
        serverThread->start();
}
// -----------------------------------------------------------------------------
void OPCUAGate::serverLoopTerminate()
{
    running = true;
}
// -----------------------------------------------------------------------------
void OPCUAGate::serverLoop()
{
    if( uaServer == nullptr )
        return;

    running = true;
    UA_StatusCode retval = UA_Server_run(uaServer, &running);
    dinfo << myname << "(serverLoop): finished [" << (retval == UA_STATUSCODE_GOOD ? "OK": "FAILED") << "]" << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAGate::deactivateObject()
{
    running = false;
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
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset::DefaultObjectId )
    {
        cerr << "(OPCUAGate): Not found ID for '" << name
              << " in '" << conf->getObjectsSection() << "' section" << endl;
        return 0;
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", name);
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        cerr << "(OPCUAGate): " << name << "(init): Not found <" + confname + ">" << endl;
        return 0;
    }

    dinfo << "(OPCUAGate): name = " << name << "(" << ID << ")" << endl;
    return make_shared<OPCUAGate>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
