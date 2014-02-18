#include <cmath>
#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "MBTCPMultiSlave.h"
#include "modbus/ModbusRTUSlaveSlot.h"
#include "modbus/ModbusTCPServerSlot.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
using namespace ModbusRTU;
// -----------------------------------------------------------------------------
MBTCPMultiSlave::MBTCPMultiSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic, const string& prefix ):
MBSlave(objId,shmId,ic,prefix)
{
    cnode = conf->getNode(myname);
    if( cnode == NULL )
        throw UniSetTypes::SystemError("(MBSlave): Not found conf-node for " + myname );

    UniXML::iterator it(cnode);

    waitTimeout = conf->getArgInt("--" + prefix + "-wait-timeout",it.getProp("waitTimeout"));
    if( waitTimeout == 0 )
        waitTimeout = 4000;

    UniXML::iterator cit(it);

    if( cit.find("clients") && cit.goChildren() )
    {
        for( ;cit; cit++ )
        {
            ClientInfo c;
            c.iaddr = cit.getProp("ip");
            if( c.iaddr.empty() )
            {
                ostringstream err;
                err << myname << "(init): Unknown ip=''";
                dcrit << err.str() << endl;
                throw SystemError(err.str());
            }

            // resolve (если получиться)
            ost::InetAddress ia(c.iaddr.c_str());
            c.iaddr = string( ia.getHostname() );

            if( !cit.getProp("respond").empty() )
            {
                c.respond_s = conf->getSensorID(cit.getProp("respond"));
                if( c.respond_s == DefaultObjectId )
                {
                    ostringstream err;
                    err << myname << "(init): Not found sensor ID for " << cit.getProp("respond");
                    dcrit << err.str() << endl;
                    throw SystemError(err.str());
                }
            }

            if( !cit.getProp("askcount").empty() )
            {
                c.askcount_s = conf->getSensorID(cit.getProp("askcount"));
                if( c.askcount_s == DefaultObjectId )
                {
                    ostringstream err;
                    err << myname << "(init): Not found sensor ID for " << cit.getProp("askcount");
                    dcrit << err.str() << endl;
                    throw SystemError(err.str());
                }
            }

            c.invert = cit.getIntProp("invert");

            if( !cit.getProp("timeout").empty() )
            {
                c.tout = cit.getIntProp("timeout");
                c.ptTimeout.setTiming(c.tout);
            }

            cmap[c.iaddr] = c;
            dinfo << myname << "(init): add client: " << c.iaddr << " respond=" << c.respond_s << " askcount=" << c.askcount_s << endl;
        }
    }
}
// -----------------------------------------------------------------------------
MBTCPMultiSlave::~MBTCPMultiSlave()
{
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::help_print( int argc, const char* const* argv )
{
   MBSlave::help_print(argc,argv);
}
// -----------------------------------------------------------------------------
MBTCPMultiSlave* MBTCPMultiSlave::init_mbslave( int argc, const char* const* argv, UniSetTypes::ObjectId icID, SharedMemory* ic,
                                const string& prefix )
{
    string name = conf->getArgParam("--" + prefix + "-name","MBSlave1");
    if( name.empty() )
    {
        cerr << "(mbslave): Не задан name'" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);
    if( ID == UniSetTypes::DefaultObjectId )
    {
        cerr << "(mbslave): идентификатор '" << name
            << "' не найден в конф. файле!"
            << " в секции " << conf->getObjectsSection() << endl;
        return 0;
    }

    dinfo << "(mbslave): name = " << name << "(" << ID << ")" << endl;
    return new MBTCPMultiSlave(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::execute_tcp()
{
    ModbusTCPServerSlot* sslot = dynamic_cast<ModbusTCPServerSlot*>(mbslot);

    if( !sslot )
    {
        dcrit << myname << "(execute_tcp): DYNAMIC CAST ERROR (mbslot --> ModbusTCPServerSlot)" << std::endl;
        raise(SIGTERM);
        return;
    }

    if( dlog.debugging(Debug::LEVEL9) )
        sslot->setLog(dlog);

    for( auto &i: cmap )
        i.second.ptTimeout.reset();

    while(1)
    {
        try
        {
            sslot->waitQuery( addr, waitTimeout );

            // Обновляем информацию по соединениям
            sess.clear();
            sslot->getSessions(sess);
            for( auto& s: sess )
            {
                cerr << " find " << s.iaddr << endl;
                auto i = cmap.find( s.iaddr );
                if( i!=cmap.end() )
                {
                    // если ещё в списке, значит отвечает (т.е. сбрасываем таймер)
                    if( i->second.tout == 0 )
                        i->second.ptTimeout.setTiming( waitTimeout );
                    else
                        i->second.ptTimeout.reset();

                    i->second.askCount = s.askCount;
                }
            }

            // а теперь проходим по списку и выставляем датчики..
            for( auto &it: cmap )
            {
                ClientInfo& c(it.second);

                if( dlog.is_level4() )
                    dlog4 << myname << "(work): " << c.iaddr << " resp=" << (c.invert ? c.ptTimeout.checkTime() : !c.ptTimeout.checkTime())
                        << " askcount=" << c.askCount
                        << endl;


                if( c.respond_s != DefaultObjectId )
                {
                    try
                    {
                        bool st = c.invert ? c.ptTimeout.checkTime() : !c.ptTimeout.checkTime();
                        shm->localSetValue(c.respond_it,c.respond_s,st,getId());
                    }
                    catch(Exception& ex)
                    {
                        dcrit << myname << "(execute_tcp): " << ex << std::endl;
                    }
               }

               if( c.askcount_s != DefaultObjectId )
               {
                   try
                   {
                       shm->localSetValue(c.askcount_it,c.askcount_s, c.askCount,getId());
                   }
                   catch(Exception& ex)
                   {
                       dcrit << myname << "(execute_tcp): " << ex << std::endl;
                   }
               }
            }

#if 0
            if( res!=ModbusRTU::erTimeOut )
                ptTimeout.reset();

            // собираем статистику обмена
            if( prev!=ModbusRTU::erTimeOut )
            {
                //  с проверкой на переполнение
                askCount = askCount>=numeric_limits<long>::max() ? 0 : askCount+1;
                if( res!=ModbusRTU::erNoError )
                    errmap[res]++;
            }

            prev = res;

            if( res!=ModbusRTU::erNoError && res!=ModbusRTU::erTimeOut )
                dlog[Debug::WARN] << myname << "(execute_tcp): " << ModbusRTU::mbErr2Str(res) << endl;
#endif
            if( !activated )
                continue;

            if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
            {
                try
                {
                    shm->localSetValue(itHeartBeat,sidHeartBeat,maxHeartBeat,getId());
                    ptHeartBeat.reset();
                }
                catch(Exception& ex)
                {
                    dcrit << myname << "(execute_tcp): (hb) " << ex << std::endl;
                }
            }

            if( respond_id != DefaultObjectId )
            {
                bool state = ptTimeout.checkTime() ? false : true;
                if( respond_invert )
                    state ^= true;
                try
                {
                    shm->localSetValue(itRespond,respond_id,(state ? 1 : 0),getId());
                }
                catch(Exception& ex)
                {
                    dcrit << myname << "(execute_rtu): (respond) " << ex << std::endl;
                }
            }

            if( askcount_id!=DefaultObjectId )
            {
                try
                {
                    shm->localSetValue(itAskCount,askcount_id,askCount,getId());
                }
                catch(Exception& ex)
                {
                    dcrit << myname << "(execute_rtu): (askCount) " << ex << std::endl;
                }
            }
        }
        catch(...){}
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiSlave::initIterators()
{
    MBSlave::initIterators();
    for( auto &i: cmap )
        i.second.initIterators(shm);
}
// -----------------------------------------------------------------------------
