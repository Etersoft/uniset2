// -----------------------------------------------------------------------------
extern "C" {
#include <rrd.h>
}
#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "RRDServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
RRDServer::RRDServer( UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmId, const std::shared_ptr<SharedMemory> ic,
            const string& prefix, DebugStream& log ):
UObject_SK(objId,cnode),
prefix(prefix)
{
    shm = make_shared<SMInterface>(shmId,ui,objId,ic);
    mylog = log;
    UniXML::iterator it(cnode);

    UniXML::iterator it1(cnode);
    if( !it1.goChildren() )
    {
        ostringstream err;
        err << myname << "(init): empty rrd list...";
        mylog << err.str() << endl;
        throw NameNotFound(err.str());
    }

    int tmID=1;
    for( ;it1.getCurrent(); it1++,++tmID )
        initRRD(it1,tmID);
}
// -----------------------------------------------------------------------------
RRDServer::~RRDServer()
{
}
// -----------------------------------------------------------------------------
void RRDServer::step()
{
}
//--------------------------------------------------------------------------------
void RRDServer::initRRD( xmlNode* cnode, int tmID )
{
    UniXML::iterator it(cnode);

    string fname( it.getProp("filename") );
    string ff( it.getProp("filter_field") );
    string fv( it.getProp("filter_value") );
    string cf( it.getProp("ds_field") );
    if( cf.empty() )
        cf = ff+fv+"_ds";

    int rrdstep = it.getPIntProp("step",5);
    int lastup =  it.getPIntProp("lastup",0);
    bool overwrite = it.getPIntProp("overwrite",0);

    myinfo << myname << "(init): add rrd: file='" << fname
            << " " << ff << "='" << fv
            << "' create='" << cf << "'"
            << " step=" << rrdstep
            << endl;


    std::list<std::string> rralist;
    UniXML::iterator it_rra(cnode);
    if( !it_rra.goChildren() )
    {
        ostringstream err;
        err << myname << "(init): rrd='" << fname << "' Unknown RRA list";
        mycrit << err.str();
        throw SystemError(err.str());
    }

    for(;it_rra.getCurrent(); it_rra++ )
    {
        string rra( it_rra.getProp("rra") );
        if( rra.empty() )
        {
            ostringstream err;
            err << myname << "(init): rrd='" << fname << "' Unkown RRA item.. <item rra='...'";
            mycrit << err.str();
            throw SystemError(err.str());
        }

        rralist.push_back(rra);
    }

    if( rralist.empty() )
    {
        ostringstream err;
        err << myname << "(init): Not found RRA items...";
        mycrit << err.str() << endl;
        throw SystemError(err.str());
    }

//    try
    {
        auto conf = uniset_conf();

        xmlNode* snode = conf->getXMLSensorsSection();
        if(!snode)
        {
            ostringstream err;
            err << myname << "(init): Not found section <sensors>";
            mycrit << err.str();
            throw SystemError(err.str());
        }

        UniXML::iterator it1(snode);
        if( !it1.goChildren() )
        {
            ostringstream err;
            err << myname << "(init): section <sensors> empty?!";
            mycrit << err.str();
            throw SystemError(err.str());
        }

        DSMap dsmap;

        // список параметров входящих в RRD
        std::list<std::string> dslist;

        for(;it1.getCurrent(); it1.goNext() )
        {
            if( !UniSetTypes::check_filter(it1,ff,fv) )
                continue;

            std::string a(it1.getProp(cf));
            if( a.empty() )
            {
                ostringstream err;
                err << myname << "(init): Unknown create parameters ('" << cf << "')";
                mycrit << err.str();
                throw SystemError(err.str());
            }

            std::string dsname(it1.getProp("name"));
            ostringstream nm;
            nm << "DS:" << dsname << ":" << a;
            dslist.push_back(nm.str());

            ObjectId sid = conf->getSensorID( dsname );
            if( sid == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): Unknown SensorID for '" << dsname << "'";
                mycrit << err.str();
                throw SystemError(err.str());
            }

            DSInfo ds(dsname,it1.getIntProp("default"));
            dsmap.insert( DSMap::value_type(sid,ds) );
        }

        if( dslist.empty() )
        {
            ostringstream err;
            err << myname << "(init): Not found RRD items...";
            mycrit << err.str() << endl;
            throw SystemError(err.str());
        }

        char argc = dslist.size() + rralist.size();
        char** argv = new char*[ argc ];

        int k=0;
        for( auto &i: dslist )
            argv[k++] = strdup(i.c_str());

        for( auto &i: rralist )
            argv[k++] = strdup(i.c_str());

//         for( k=0; k<argc; k++ )
//             cout << "*** argv[" << k << "]='" << argv[k] << "'" << endl;

        // Собственно создаём RRD
        if( !overwrite && file_exist(fname) )
        {
            myinfo << myname << "(init): ignore create file='" << fname
                << "'. File exist... overwrite=0." << endl;
        }
        else
        {
             rrd_clear_error();
            if( rrd_create_r(fname.c_str(),rrdstep,lastup,argc,(const char**)argv) < 0 )
            {
                ostringstream err;
                err << myname << "(init): Can`t create RRD ('" << fname << "'): err: " << string(rrd_get_error());
                mycrit << err.str() << endl;
                throw SystemError(err.str());
            }
        }

        // Чистим выделенную память
        for( k=0; k<argc; k++ )
            free( argv[k] );

        delete[] argv;

        rrdlist.emplace_back(fname,tmID,rrdstep,dsmap);
    }
/*    catch( Exception& ex )
    {
        mycrit << myname << "(init) " << ex << std::endl;
    }
    catch( ...  )
    {
        mycrit << myname << "(init): catch ..." << std::endl;
    }
*/
}
//--------------------------------------------------------------------------------
void RRDServer::help_print( int argc, const char* const* argv )
{
    cout << " Default prefix='rrd'" << endl;
    cout << "--prefix-name        - ID for rrdstorage. Default: RRDServer1. " << endl;
    cout << "--prefix-confnode    - configuration section name. Default: <NAME name='NAME'...> " << endl;
    cout << "--prefix-heartbeat-id name   - ID for heartbeat sensor." << endl;
    cout << "--prefix-heartbeat-max val   - max value for heartbeat sensor." << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<RRDServer> RRDServer::init_rrdstorage( int argc, const char* const* argv,
                                            UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory> ic,
                                            const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name","RRDServer");
    if( name.empty() )
    {
        dcrit << "(RRDServer): Unknown name. Usage: --" <<  prefix << "-name" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);
    if( ID == UniSetTypes::DefaultObjectId )
    {
        dcrit << "(RRDServer): Not found ID for '" << name
            << " in '" << conf->getObjectsSection() << "' section" << endl;
        return 0;
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode",name);
    xmlNode* cnode = conf->getNode(confname);
    if( !cnode )
    {
        dcrit << "(RRDServer): " << name << "(init): Not found <" + confname + ">" << endl;
        return 0;
    }

    dinfo << "(RRDServer): name = " << name << "(" << ID << ")" << endl;
    return make_shared<RRDServer>(ID,cnode,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
void RRDServer::askSensors( UniversalIO::UIOCommand cmd )
{
    UObject_SK::askSensors(cmd);

    for( auto &it: rrdlist )
    {
        for( auto &s: it.dsmap )
        {
            try
            {
                shm->askSensor(s.first,cmd);
            }
            catch( std::exception& ex )
            {
                mycrit << myname << "(askSensors): " << ex.what() << endl;
            }
        }
    }
}
// -----------------------------------------------------------------------------
void RRDServer::sysCommand( const UniSetTypes::SystemMessage* sm )
{
    UObject_SK::sysCommand(sm);
    if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
    {
        for( auto &it: rrdlist )
        {
            try
            {
                askTimer(it.tid,it.sec*1000);
            }
            catch( std::exception& ex )
            {
                mycrit << myname << "(askTimer): " << ex.what() << endl;
            }
        }
    }
}
// -----------------------------------------------------------------------------
void RRDServer::sensorInfo( const UniSetTypes::SensorMessage* sm )
{
    for( auto &it: rrdlist )
    {
        auto s = it.dsmap.find(sm->id);
        if( s!=it.dsmap.end() )
            s->second.value = sm->value;

        // продолжаем искать по другим rrd, т.к. датчик может входить в несколько..
    }
}
// -----------------------------------------------------------------------------
void RRDServer::timerInfo( const UniSetTypes::TimerMessage* tm )
{
    for( auto &it: rrdlist )
    {
        if( it.tid == tm->id )
        {
            ostringstream v;
            v << time(0);

            for( auto &s: it.dsmap )
                v << ":" << s.second.value;

            myinfo << myname << "(update): '" << it.filename << "' " << v.str() << endl;

            rrd_clear_error();
            const char* argv = v.str().c_str();

            if( rrd_update_r(it.filename.c_str(),NULL,1,&argv) < 0 )
            {
                ostringstream err;
                err << myname << "(update): Can`t update RRD ('" << it.filename << "'): err: " << string(rrd_get_error());
                mycrit << err.str() << endl;
            }

            break;
        }
    }
}
// -----------------------------------------------------------------------------
