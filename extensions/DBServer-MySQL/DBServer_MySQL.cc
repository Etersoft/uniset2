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
#include <cmath>

#include "unisetstd.h"
#include "ORepHelpers.h"
#include "DBServer_MySQL.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
#include "DBLogSugar.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
DBServer_MySQL::DBServer_MySQL(ObjectId id, xmlNode* cnode, const std::string& prefix ):
    USingleProcess(cnode, uniset_conf()->getArgc(), uniset_conf()->getArgv(),""),
    DBServer(id, prefix)
{

    if( getId() == DefaultObjectId )
    {
        ostringstream msg;
        msg << "(DBServer_MySQL): init failed! Unknown ID!" << endl;
        throw Exception(msg.str());
    }

    db = unisetstd::make_unique<MySQLInterface>();

    mqbuf.setName(myname  + "_qbufMutex");
}

DBServer_MySQL::DBServer_MySQL( const std::string& prefix ):
    DBServer_MySQL(uniset_conf()->getDBServer(), uniset_conf()->getNode("LocalDBServer"), prefix)
{
}
//--------------------------------------------------------------------------------------------
DBServer_MySQL::~DBServer_MySQL()
{
    if( db )
        db->close();
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::sysCommand( const uniset::SystemMessage* sm )
{
    DBServer::sysCommand(sm);

    switch( sm->command )
    {
        case SystemMessage::StartUp:
            break;

        case SystemMessage::Finish:
        {
            activate = false;
            db->close();
        }
        break;

        case SystemMessage::FoldUp:
        {
            activate = false;
            db->close();
        }
        break;

        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------
void DBServer_MySQL::confirmInfo( const uniset::ConfirmMessage* cem )
{
    try
    {
        ostringstream data;

        data << "UPDATE " << tblName(cem->type)
             << " SET confirm='" << cem->confirm_time.tv_sec << "'"
             << " WHERE sensor_id='" << cem->sensor_id << "'"
             << " AND date='" << dateToString(cem->sensor_time.tv_sec, "-") << " '"
             << " AND time='" << timeToString(cem->sensor_time.tv_sec, ":") << " '"
             << " AND time_usec='" << cem->sensor_time.tv_nsec / 1000 << " '";

        dbinfo << myname << "(update_confirm): " << data.str() << endl;

        if( !writeToBase(data.str()) )
        {
            dbcrit << myname << "(update_confirm):  db error: " << db->error() << endl;
        }
    }
    catch( const uniset::Exception& ex )
    {
        dbcrit << myname << "(update_confirm): " << ex << endl;
    }
    catch( const std::exception& ex )
    {
        dbcrit << myname << "(update_confirm): exception: " << ex.what() << endl;
    }
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::onTextMessage( const TextMessage* msg )
{
    try
    {
        // если время не было выставлено (указываем время сохранения в БД)
        if( !msg->tm.tv_sec )
        {
            // Выдаём CRIT, но тем не менее сохраняем в БД

            dbcrit << myname << "(insert_main_messages): UNKNOWN TIMESTAMP! (tm.tv_sec=0)"
                   << " for msg='" << msg->txt << "'"
                   << " supplier=" << uniset_conf()->oind->getMapName(msg->supplier)
                   << endl;
        }

        ostringstream data;
        data << "INSERT INTO " << tblName(msg->type)
             << "(date, time, time_usec, text, mtype, node) VALUES( '"
             << dateToString(msg->tm.tv_sec, "-") << "','"   //  date
             << timeToString(msg->tm.tv_sec, ":") << "','"   //  time
             << msg->tm.tv_nsec << "','"                //  time_usec
             << msg->txt << "','"                    // text
             << msg->mtype << "','"   // mtype
             << msg->node << "')";                //  node

        dbinfo << myname << "(insert_main_messages): " << data.str() << endl;

        if( !writeToBase(data.str()) )
        {
            dbcrit << myname << "(insert_main_messages): error: " << db->error() << endl;
        }
    }
    catch( const uniset::Exception& ex )
    {
        dbcrit << myname << "(insert_main_messages): " << ex << endl;
    }
    catch( const std::exception& ex )
    {
        dbcrit << myname << "(insert_main_messages): catch: " << ex.what() << endl;
    }
}
//--------------------------------------------------------------------------------------------
bool DBServer_MySQL::writeToBase( const std::string& query )
{
    dbinfo << myname << "(writeToBase): " << query << endl;

    //    cout << "DBServer_MySQL: " << query << endl;
    if( !db || !connect_ok )
    {
        uniset_rwmutex_wrlock l(mqbuf);
        qbuf.push(query);

        if( qbuf.size() > qbufSize )
        {
            std::string qlost;

            if( lastRemove )
                qlost = qbuf.back();
            else
                qlost = qbuf.front();

            qbuf.pop();

            dbcrit << myname << "(writeToBase): DB not connected! buffer(" << qbufSize
                   << ") overflow! lost query: " << qlost << endl;
        }

        return false;
    }

    // На всякий скидываем очередь
    flushBuffer();

    // А теперь собственно запрос..
    db->query(query);

    // Дело в том что на INSERT И UPDATE запросы
    // db->query() может возвращать false и надо самому
    // отдельно проверять действительно ли произошла ошибка
    // см. MySQLInterface::query.
    string err(db->error());

    if( err.empty() )
        return true;

    return false;
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::flushBuffer()
{
    uniset_rwmutex_wrlock l(mqbuf);

    // Сперва пробуем очистить всё что накопилось в очереди до этого...
    while( !qbuf.empty() )
    {
        db->query( qbuf.front() );

        // Дело в том что на INSERT И UPDATE запросы
        // db->query() может возвращать false и надо самому
        // отдельно проверять действительно ли произошла ошибка
        // см. MySQLInterface::query.
        string err(db->error());

        if( !err.empty() )
            dbcrit << myname << "(writeToBase): error: " << err <<
                   " lost query: " << qbuf.front() << endl;

        qbuf.pop();
    }
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::sensorInfo( const uniset::SensorMessage* si )
{
    try
    {
        // если время не было выставлено (указываем время сохранения в БД)
        if( !si->tm.tv_sec )
        {
            // Выдаём CRIT, но тем не менее сохраняем в БД

            dbcrit << myname << "(insert_main_history): UNKNOWN TIMESTAMP! (tm.tv_sec=0)"
                   << " for sid=" << si->id
                   << " supplier=" << uniset_conf()->oind->getMapName(si->supplier)
                   << endl;
        }

        float val = (float)si->value / (float)pow(10.0, si->ci.precision);

        // см. DBTABLE AnalogSensors, DigitalSensors
        ostringstream data;
        data << "INSERT INTO " << tblName(si->type)
             << "(date, time, time_usec, sensor_id, value, node) VALUES( '"
             // Поля таблицы
             << dateToString(si->sm_tv.tv_sec, "-") << "','"   //  date
             << timeToString(si->sm_tv.tv_sec, ":") << "','"   //  time
             << si->sm_tv.tv_nsec << "','"                //  time_usec
             << si->id << "','"                    //  sensor_id
             << val << "','"                //  value
             << si->node << "')";                //  node

        dbinfo << myname << "(insert_main_history): " << data.str() << endl;

        if( !writeToBase(data.str()) )
        {
            dbcrit << myname << "(insert) sensor msg error: " << db->error() << endl;
        }
    }
    catch( const uniset::Exception& ex )
    {
        dbcrit << myname << "(insert_main_history): " << ex << endl;
    }
    catch( const std::exception& ex )
    {
        dbcrit << myname << "(insert_main_history): catch: " << ex.what() << endl;
    }
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::initDBServer()
{
    DBServer::initDBServer();
    dbinfo << myname << "(init): ..." << endl;

    if( connect_ok )
    {
        onReconnect(db);
        return;
    }

    auto conf = uniset_conf();

    if( conf->getDBServer() == uniset::DefaultObjectId )
    {
        ostringstream msg;
        msg << myname << "(init): DBServer OFF for this node.."
            << " In " << conf->getConfFileName()
            << " for this node dbserver=''";
        throw NameNotFound(msg.str());
    }

    xmlNode* node = conf->getNode("LocalDBServer");

    if( !node )
        throw NameNotFound(string(myname + "(init): section <LocalDBServer> not found.."));

    UniXML::iterator it(node);

    dbinfo << myname << "(init): init connection.." << endl;
    string dbname(conf->getProp(node, "dbname"));
    string dbnode(conf->getProp(node, "dbnode"));
    string user(conf->getProp(node, "dbuser"));
    string password(conf->getProp(node, "dbpass"));

    tblMap[uniset::Message::SensorInfo] = "main_history";
    tblMap[uniset::Message::Confirm] = "main_history";
    tblMap[uniset::Message::TextMessage] = "main_messages";

    PingTime = conf->getPIntProp(node, "pingTime", PingTime);
    ReconnectTime = conf->getPIntProp(node, "reconnectTime", ReconnectTime);
    qbufSize = conf->getArgPInt("--dbserver-buffer-size", it.getProp("bufferSize"), qbufSize);

    if( findArgParam("--dbserver-buffer-last-remove", conf->getArgc(), conf->getArgv()) != -1 )
        lastRemove = true;
    else if( it.getIntProp("bufferLastRemove" ) != 0 )
        lastRemove = true;
    else
        lastRemove = false;

    if( dbnode.empty() )
        dbnode = "localhost";

    dbinfo << myname << "(init): connect dbnode=" << dbnode
           << "\tdbname=" << dbname
           << " pingTime=" << PingTime
           << " ReconnectTime=" << ReconnectTime << endl;

    if( !db->nconnect(dbnode, user, password, dbname) )
    {
        //        ostringstream err;
        dbcrit << myname
               << "(init): DB connection error: "
               << db->error() << endl;
        //        throw Exception( string(myname+"(init): не смогли создать соединение с БД "+db->error()) );
        askTimer(DBServer_MySQL::ReconnectTimer, ReconnectTime);
    }
    else
    {
        dbinfo << myname << "(init): connect [OK]" << endl;
        connect_ok = true;
        askTimer(DBServer_MySQL::ReconnectTimer, 0);
        askTimer(DBServer_MySQL::PingTimer, PingTime);
        onReconnect(db);
        flushBuffer();
    }
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::timerInfo( const uniset::TimerMessage* tm )
{
    DBServer::timerInfo(tm);

    switch( tm->id )
    {
        case DBServer_MySQL::PingTimer:
        {
            if( !db->ping() )
            {
                dbwarn << myname << "(timerInfo): DB lost connection.." << endl;
                connect_ok = false;
                askTimer(DBServer_MySQL::PingTimer, 0);
                askTimer(DBServer_MySQL::ReconnectTimer, ReconnectTime);
            }
            else
            {
                connect_ok = true;
                dbinfo << myname << "(timerInfo): DB ping ok" << endl;
            }
        }
        break;

        case DBServer_MySQL::ReconnectTimer:
        {
            dbinfo << myname << "(timerInfo): reconnect timer" << endl;

            if( db->isConnection() )
            {
                if( db->ping() )
                {
                    connect_ok = true;
                    askTimer(DBServer_MySQL::ReconnectTimer, 0);
                    askTimer(DBServer_MySQL::PingTimer, PingTime);
                }
                else
                {
                    connect_ok = false;
                    dbwarn << myname << "(timerInfo): DB no connection.." << endl;
                }
            }
            else
                initDBServer();
        }
        break;

        default:
            dbwarn << myname << "(timerInfo): Unknown TimerID=" << tm->id << endl;
            break;
    }
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<DBServer_MySQL> DBServer_MySQL::init_dbserver( int argc, const char* const* argv, const std::string& prefix )
{
    auto conf = uniset_conf();

    ObjectId ID = conf->getDBServer();

    string name = conf->getArgParam("--" + prefix + "-name", "");

    if( !name.empty() )
    {
        ObjectId ID = conf->getServiceID(name);

        if( ID == uniset::DefaultObjectId )
        {
            cerr << "(DBServer_MySQL): Unknown ServiceID for '" << name << endl;
            return nullptr;
        }
    }

    string cname = conf->getArgParam("--" + prefix + "-confnode", "LocalDBServer");
    auto confnode = conf->getNode(cname);
    if( !confnode )
    {
        cerr << "(DBServer_MySQL): Not found confnode '" << cname << "' in config file" << endl;
        return nullptr;
    }

    uinfo << "(DBServer_MySQL): name = " << name << "(" << ID << ")" << endl;
    return make_shared<DBServer_MySQL>(ID, confnode, prefix);
}
// -----------------------------------------------------------------------------
void DBServer_MySQL::help_print( int argc, const char* const* argv )
{
    cout << "Default: prefix='mysql'" << endl;
    cout << "--prefix-name objectID     - ObjectID. Default: 'conf->getDBServer()'" << endl;
    cout << "--run-lock file            - Запустить с защитой от повторного запуска" << endl;
    cout << endl;
    cout << DBServer::help_print() << endl;
}
// -----------------------------------------------------------------------------
std::string DBServer_MySQL::getMonitInfo( const std::string& params )
{
    ostringstream inf;

    inf << "Database: "
        << "[ ping=" << PingTime
        << " reconnect=" << ReconnectTime
        << " qbufSize=" << qbufSize
        << " ]" << endl
        << "  connection: " << (connect_ok ? "OK" : "FAILED") << endl;
    {
        uniset_rwmutex_rlock lock(mqbuf);
        inf << " buffer size: " << qbuf.size() << endl;
    }
    inf << "   lastError: " << db->error() << endl;

    return inf.str();
}
// -----------------------------------------------------------------------------
