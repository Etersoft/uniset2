/*
 * Copyright (c) 2025 Pavel Vainerman.
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
#include <vector>
#include "quickjs/quickjs-libc.h"
#include "Configuration.h"
#include "JSProxy.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
#include <vector>
#include <string>
#include <functional>
// -------------------------------------------------------------------------
#ifndef UNISET_DATADIR
#define UNISET_DATADIR "/usr/share/uniset2/js"
#endif
// -------------------------------------------------------------------------
JSProxy::JSProxy( uniset::ObjectId id, xmlNode* confnode, const std::string& _prefix ):
    JSProxy_SK( id, confnode, string(_prefix + "-") )
{
    if( file.empty() )
        throw SystemError("JS file undefined");

    auto conf = uniset_conf();
    auto jdir = conf->getDataDir();

    if( !jdir.empty() && jdir[jdir.size() - 1] == '/' )
        jdir += "js";
    else
        jdir += "/js";

    std::vector<std::string> searchPaths = { ".", jdir };
    std::string uniset_path(UNISET_DATADIR);

    if( !uniset_path.empty() )
        searchPaths.push_back(uniset_path);

    UniXML::iterator pit(confnode);

    if( pit.find("modules") && pit.goChildren() )
    {
        for( ; pit; pit++ )
        {
            auto p = pit.getProp("path");

            if( !p.empty() )
                searchPaths.push_back(p);
        }
    }

    for( const auto& p : searchPaths )
        myinfo << "(init): added js modules path '" << p << "'" << endl;

    myinfo << "(init): esm module mode " << ( esmModuleMode ? "ENABLED" : "DISABLED") << endl;
    js = make_shared<JSEngine>(file, searchPaths, ui, loopCount, esmModuleMode);

    if( !js )
        throw SystemError("Can't create JSEngine");

    loga->add(js->log());
    loga->add(js->js_log());
    js->log()->addLevel(log()->level());

    {
        ostringstream s;
        s << argprefix << "script-log";
        conf->initLogStream(js->js_log(), s.str());
    }
}
// -------------------------------------------------------------------------
JSProxy::~JSProxy()
{
    js = nullptr;
}
// ----------------------------------------------------------------------------
void JSProxy::help_print()
{
    cout << "--run-lock file         - Запустить с защитой от повторного запуска" << endl;
    cout << "--js-name               - ID for process. Default: JSProxy " << endl;
    cout << "--js-confnode           - configuration section name. Default: <NAME name='NAME'...> " << endl;
    cout << "--js-sm-ready-timeout   - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
    cout << "--js-sm-test-sid        - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;

    cout << endl;
    cout << "JSEngine:" << endl;
    cout << "--js-file main.js         - Запускаемый JS script" << endl;
    cout << "--js-sleep-msec msec      - Пауза между вызовами uniset_on_step(). По умолчанию 150" << endl;
    cout << "--js-loopCount num        - количество JS событий обрабатываемых за один раз. По умолчанию 5" << endl;
    cout << endl;
    cout << "Logs:" << endl;
    cout << "--js-log-...            - log control" << endl;
    cout << "             add-levels ...  " << endl;
    cout << "             del-levels ...  " << endl;
    cout << "             set-levels ...  " << endl;
    cout << "             logfile filename" << endl;
    cout << "             no-debug " << endl;
    cout << "--js-script-log-...            - log control" << endl;
    cout << "             add-levels ...  " << endl;
    cout << "             del-levels ...  " << endl;
    cout << "             set-levels ...  " << endl;
    cout << "             logfile filename" << endl;
    cout << "             no-debug " << endl;
    cout << "LogServer: " << endl;
    cout << "--js-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--js-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--js-logserver-port num - listen port. Default: ID" << endl;
    cout << LogServer::help_print("prefix-logserver") << endl;
}
// ----------------------------------------------------------------------------
std::shared_ptr<JSProxy> JSProxy::init( int argc, const char* const* argv, const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "JSProxy");

    if( name.empty() )
    {
        cerr << "(JSProxy): Unknown name. Usage: --" <<  prefix << "-name" << endl;
        return nullptr;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset::DefaultObjectId )
    {
        cerr << "(JSProxy): Not found ID for '" << name
             << " in '" << conf->getObjectsSection() << "' section" << endl;
        return nullptr;
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", name);
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        cerr << "(JSProxy): " << name << "(init): Not found <" + confname + ">" << endl;
        return nullptr;
    }

    return make_shared<JSProxy>(ID, cnode, prefix);
}
// ----------------------------------------------------------------------------
void JSProxy::callback() noexcept
{
    if( !js->isActive() )
    {
        try
        {
            js->init();
        }
        catch( std::exception& ex )
        {
            mycrit << "(init): " << ex.what() << endl;
            uterminate();
        }
    }

    JSProxy_SK::callback();
}
// ----------------------------------------------------------------------------
void JSProxy::askSensors( UniversalIO::UIOCommand cmd )
{
    js->askSensors(cmd);
}
// ----------------------------------------------------------------------------
void JSProxy::sensorInfo( const uniset::SensorMessage* sm )
{
    js->sensorInfo(sm);
}
// -------------------------------------------------------------------------
void JSProxy::step()
{
    js->step();
    js->updateOutputs();
}
// -------------------------------------------------------------------------
void JSProxy::sysCommand(const uniset::SystemMessage* sm )
{
    JSProxy_SK::sysCommand(sm);

    if( sm->command == SystemMessage::StartUp )
        js->start();
}
// -------------------------------------------------------------------------
bool JSProxy::deactivateObject()
{
    js->stop();
    return JSProxy_SK::deactivateObject();
}
// -------------------------------------------------------------------------
