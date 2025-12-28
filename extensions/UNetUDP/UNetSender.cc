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
// -------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <Poco/Net/NetException.h>
#include "unisetstd.h"
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetSender.h"
#include "UNetLogSugar.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    using namespace std;
    using namespace uniset::extensions;
    // -----------------------------------------------------------------------------
    UNetSender::UNetSender( std::unique_ptr<UNetSendTransport>&& _transport, const std::shared_ptr<SMInterface>& smi,
                            bool nocheckConnection, const std::string& s_f, const std::string& s_val,
                            const std::string& s_prefix,
                            const std::string& prefix,
                            size_t maxDCount, size_t maxACount ):
        s_field(s_f),
        s_fvalue(s_val),
        prop_prefix(s_prefix),
        shm(smi),
        transport(std::move(_transport)),
        sendpause(150),
        packsendpause(5),
        packsendpauseFactor(1),
        activated(false),
        packetnum(0),
        maxAData(maxACount),
        maxDData(maxDCount)
    {
        items.reserve(100);

        {
            ostringstream s;
            s << "S(" << setw(15) << transport->toString() << ")";
            myname = s.str();
        }

        ostringstream logname;
        logname << prefix << "-S-" << transport->toString();

        unetlog = make_shared<DebugStream>();
        unetlog->setLogName(logname.str());

        auto conf = uniset_conf();
        conf->initLogStream(unetlog, prefix + "-log");

        unetinfo << myname << "(init): read filter-field='" << s_field
                 << "' filter-value='" << s_fvalue << "'" << endl;

        unetinfo << "(UNetSender): UDP set to " << transport->toString() << endl;

        ptCheckConnection.setTiming(10000); // default 10 сек
        createConnection(nocheckConnection);

        s_thr = unisetstd::make_unique< ThreadCreator<UNetSender> >(this, &UNetSender::send);

        mypacks[0].resize(1);
        packs_anum[0] = 0;
        packs_dnum[0] = 0;
        auto& mypack(mypacks[0][0]);

        // выставляем поля, которые не меняются
        {
            uniset_rwmutex_wrlock l(mypack.mut);
            mypack.msg.header.nodeID = uniset_conf()->getLocalNode();
            mypack.msg.header.procID = shm->ID();
        }

        // -------------------------------
        if( shm->isLocalwork() )
        {
            readConfiguration();
            unetinfo << myname << "(init): dlist size = " << items.size() << endl;
        }
        else
        {
            auto ic = std::dynamic_pointer_cast<SharedMemory>(shm->SM());

            if( ic )
                ic->addReadItem( sigc::mem_fun(this, &UNetSender::readItem) );
            else
            {
                unetwarn << myname << "(init): Failed to convert the pointer 'IONotifyController' -> 'SharedMemory'" << endl;
                readConfiguration();
                unetinfo << myname << "(init): dlist size = " << items.size() << endl;
            }
        }
    }
    // -----------------------------------------------------------------------------
    UNetSender::~UNetSender()
    {
    }
    // -----------------------------------------------------------------------------
    bool UNetSender::createConnection( bool throwEx )
    {
        unetinfo << myname << "(createConnection): .." << endl;

        try
        {
            return transport->createConnection(throwEx, writeTimeout);
        }
        catch( const std::exception& ex )
        {
            unetcrit << ex.what() << std::endl;

            if( throwEx )
                throw ex;
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    void UNetSender::updateFromSM()
    {
        try
        {
            mode = (Mode) shm->localGetValue(itMode, sidMode);
        }
        catch( std::exception& ex )
        {
            unetwarn << myname << "(updateFromSM): mode update error: " << ex.what() << endl;
        }

        for( auto&& it : items )
        {
            UItem& i = it.second;

            try
            {
                long value = shm->localGetValue(i.ioit, i.id);
                updateItem(i, value);
            }
            catch( IOController_i::Undefined& ex )
            {
                unetwarn << myname << "(updateFromSM): sid=" << i.id
                         << " undefined state (value=" << ex.value << ")." << endl;
                updateItem( i, ex.value );
            }
            catch( std::exception& ex )
            {
                unetwarn << myname << "(updateFromSM): " << ex.what() << endl;

                if( i.undefined_value != not_specified_value )
                    updateItem( i, i.undefined_value );
            }
        }
    }

    // -----------------------------------------------------------------------------
    void UNetSender::updateSensor( uniset::ObjectId id, long value )
    {
        if( !shm->isLocalwork() )
            return;

        if( id == sidMode )
            mode = (Mode)value;

        auto it = items.find(id);

        if( it != items.end() )
            updateItem( it->second, value );
    }
    // -----------------------------------------------------------------------------
    void UNetSender::updateItem( const UItem& it, long value )
    {
        auto& pk = mypacks[it.pack_sendfactor];

        auto& mypack(pk[it.pack_num]);
        uniset::uniset_rwmutex_wrlock l(mypack.mut);

        if( it.iotype == UniversalIO::DI || it.iotype == UniversalIO::DO )
            mypack.msg.setDData(it.pack_ind, value);
        else if( it.iotype == UniversalIO::AI || it.iotype == UniversalIO::AO )
            mypack.msg.setAData(it.pack_ind, value);
    }
    // -----------------------------------------------------------------------------
    void UNetSender::setModeID( uniset::ObjectId id ) noexcept
    {
        sidMode = id;
    }
    // -----------------------------------------------------------------------------
    void UNetSender::setCheckConnectionPause( int msec ) noexcept
    {
        if( msec > 0 )
            ptCheckConnection.setTiming(msec);
    }
    // -----------------------------------------------------------------------------
    void UNetSender::send() noexcept
    {
        unetinfo << myname << "(send): dlist size = " << items.size() << endl;
        ncycle = 0;

        ptCheckConnection.reset();

        while( activated )
        {
            if( !transport->isConnected() )
            {
                if( !ptCheckConnection.checkTime() )
                {
                    msleep(sendpause);
                    continue;
                }

                unetinfo << myname << "(send): check connection event.." << endl;

                if( !createConnection(false) )
                {
                    ptCheckConnection.reset();
                    msleep(sendpause);
                    continue;
                }
            }

            if( !shm->isLocalwork() )
            {
                try
                {
                    mode = (Mode) shm->localGetValue(itMode, sidMode);
                }
                catch (std::exception& ex)
                {
                    unetwarn << myname << "(updateFromSM): mode update error: " << ex.what() << endl;
                }
            }

            if( mode == Mode::mDisabled )
            {
                if( !activated )
                    break;

                msleep(sendpause);
                continue;
            }

            try
            {
                if( !shm->isLocalwork() )
                    updateFromSM();

                for( auto&& it : mypacks )
                {
                    if( it.first > 1 && (ncycle % it.first) != 0 )
                        continue;

                    if( !activated )
                        break;

                    auto& pk = it.second;
                    size_t size = pk.size();

                    for(size_t i = 0; i < size; ++i)
                    {
                        if( !activated )
                            break;

                        real_send(pk[i]);

                        if( packsendpause > 0 && size > 1 )
                        {
                            if( packsendpauseFactor <= 0 )
                            {
                                msleep(packsendpause);
                            }
                            else if( i > 0 && (i % packsendpauseFactor) == 0 )
                            {
                                msleep(packsendpause);
                            }
                        }
                    }
                }

                ncycle++;
            }
            catch( Poco::Net::NetException& e )
            {
                unetwarn << myname << "(send): " << e.displayText() << endl;
            }
            catch( uniset::Exception& ex)
            {
                unetwarn << myname << "(send): " << ex << std::endl;
            }
            catch( const std::exception& e )
            {
                unetwarn << myname << "(send): " << e.what() << std::endl;
            }
            catch(...)
            {
                unetwarn << myname << "(send): catch ..." << std::endl;
            }

            if( !activated )
                break;

            msleep(sendpause);
        }

        unetinfo << "************* execute FINISH **********" << endl;
    }
    // -----------------------------------------------------------------------------
    void UNetSender::real_send( PackMessage& mypack ) noexcept
    {
        try
        {
            packetnum++;

            // при переходе через ноль (когда счётчик перевалит через UniSetUDP::MaxPacketNum..
            // делаем номер пакета "1"
            if( packetnum == 0 )
                packetnum = 1;

            uniset::uniset_rwmutex_rlock l(mypack.mut);
            mypack.msg.header.num = packetnum;
            mypack.msg.updatePacketCrc();

            if( !transport->isReadyForSend(writeTimeout) )
                return;

            size_t ret = transport->send(&mypack.msg, sizeof(mypack.msg));

            if( ret < sizeof(mypack.msg) )
                unetcrit << myname << "(real_send): FAILED ret=" << ret << " < sizeof=" << sizeof(mypack.msg) << endl;
        }
        catch( Poco::Net::NetException& ex )
        {
            unetcrit << myname << "(real_send): sz=" << sizeof(mypack.msg) << " error: " << ex.displayText() << endl;
        }
        catch( std::exception& ex )
        {
            unetcrit << myname << "(real_send): sz=" << sizeof(mypack.msg) << " error: " << ex.what() << endl;
        }
    }
    // -----------------------------------------------------------------------------
    void UNetSender::stop()
    {
        activated = false;

        //    s_thr->stop();
        if( s_thr )
            s_thr->join();
    }
    // -----------------------------------------------------------------------------
    void UNetSender::start()
    {
        if( !activated )
        {
            activated = true;
            s_thr->start();
        }
    }
    // -----------------------------------------------------------------------------
    void UNetSender::readConfiguration()
    {
        xmlNode* root = uniset_conf()->getXMLSensorsSection();

        if(!root)
        {
            ostringstream err;
            err << myname << "(readConfiguration): not found <sensors>";
            throw SystemError(err.str());
        }

        UniXML::iterator it(root);

        if( !it.goChildren() )
        {
            std::cerr << myname << "(readConfiguration): empty <sensors>?!!" << endl;
            return;
        }

        for( ; it.getCurrent(); it.goNext() )
        {
            if( check_filter(it, s_field, s_fvalue) )
                initItem(it);
        }
    }
    // ------------------------------------------------------------------------------------------
    bool UNetSender::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
    {
        if( uniset::check_filter(it, s_field, s_fvalue) )
            initItem(it);

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool UNetSender::initItem( UniXML::iterator& it )
    {
        string sname( it.getProp("name") );

        string tid(it.getProp("id"));

        ObjectId sid;

        if( !tid.empty() )
        {
            sid = uniset::uni_atoi(tid);

            if( sid <= 0 )
                sid = DefaultObjectId;
        }
        else
            sid = uniset_conf()->getSensorID(sname);

        if( sid == DefaultObjectId )
        {
            unetcrit << myname << "(readItem): ID not found for "
                     << sname << endl;
            return false;
        }


        int priority = it.getPIntProp(prop_prefix + "_sendfactor", 0);

        auto& pk = mypacks[priority];

        UItem p;
        p.iotype = uniset::getIOType(it.getProp("iotype"));
        p.pack_sendfactor = priority;
        long defval = it.getIntProp("default");

        if( !it.getProp("undefined_value").empty() )
            p.undefined_value = it.getIntProp("undefined_value");

        if( p.iotype == UniversalIO::UnknownIOType )
        {
            unetcrit << myname << "(readItem): Unknown iotype for sid=" << sid << endl;
            return false;
        }

        p.id = sid;

        if( p.iotype == UniversalIO::DI || p.iotype == UniversalIO::DO )
        {
            size_t dnum = packs_dnum[priority];

            if( pk.size() <= dnum )
                pk.resize(dnum + 1);

            auto& mypack = pk[dnum];

            {
                uniset_rwmutex_wrlock l(mypack.mut);
                p.pack_ind = mypack.msg.addDData(sid, defval);
            } // unlock mutex....

            if( p.pack_ind >= maxDData )
            {
                dnum++;

                if( dnum >= pk.size() )
                    pk.resize(dnum + 1);

                auto& mypack2 = pk[dnum];
                uniset_rwmutex_wrlock l2(mypack2.mut);
                p.pack_ind = mypack2.msg.addDData(sid, defval);
                mypack2.msg.header.nodeID = uniset_conf()->getLocalNode();
                mypack2.msg.header.procID = shm->ID();
            }

            p.pack_num = dnum;
            packs_dnum[priority] = dnum;

            if ( p.pack_ind >= UniSetUDP::MaxDCount )
            {
                unetcrit << myname
                         << "(readItem): OVERFLOW! MAX UDP DIGITAL DATA LIMIT! max="
                         << UniSetUDP::MaxDCount << endl << flush;

                std::terminate();
            }
        }
        else if( p.iotype == UniversalIO::AI || p.iotype == UniversalIO::AO ) // -V560
        {
            size_t anum = packs_anum[priority];

            if( pk.size() <= anum )
                pk.resize(anum + 1);

            auto& mypack = pk[anum];

            {
                uniset_rwmutex_wrlock l(mypack.mut);
                p.pack_ind = mypack.msg.addAData(sid, defval);
            }

            if( p.pack_ind >= maxAData )
            {
                anum++;

                if( anum >= pk.size() )
                    pk.resize(anum + 1);

                auto& mypack2 = pk[anum];
                uniset_rwmutex_wrlock l2(mypack2.mut);
                p.pack_ind = mypack2.msg.addAData(sid, defval);
                mypack2.msg.header.nodeID = uniset_conf()->getLocalNode();
                mypack2.msg.header.procID = shm->ID();
            }

            p.pack_num = anum;
            packs_anum[priority] = anum;

            if ( p.pack_ind >= UniSetUDP::MaxACount )
            {
                unetcrit << myname
                         << "(readItem): OVERFLOW! MAX UDP ANALOG DATA LIMIT! max="
                         << UniSetUDP::MaxACount << endl << flush;

                std::terminate();
            }
        }

        unetinfo << myname << "(initItem): add " << p << endl;
        auto i = items.find(p.id);

        if( i != items.end() )
        {
            unetcrit << myname
                     << "(readItem): Sensor (" << p.id << ")" << sname << " ALREADY ADDED!!  ABORT!" << endl;
            std::terminate();
        }

        items.emplace(p.id, std::move(p));
        return true;
    }

    // ------------------------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, UNetSender::UItem& p )
    {
        return os << " sid=" << p.id;
    }
    // -----------------------------------------------------------------------------
    void UNetSender::initIterators() noexcept
    {
        shm->initIterator(itMode);

        for( auto&& it : items )
            shm->initIterator(it.second.ioit);
    }
    // -----------------------------------------------------------------------------
    void UNetSender::askSensors( UniversalIO::UIOCommand cmd )
    {
        shm->askSensor(sidMode, cmd);

        for( const auto& it : items  )
            shm->askSensor(it.second.id, cmd);
    }
    // -----------------------------------------------------------------------------
    size_t UNetSender::getDataPackCount() const noexcept
    {
        return mypacks.size();
    }
    // -----------------------------------------------------------------------------
    std::string UNetSender::getShortInfo() const noexcept
    {
        // warning: будет вызываться из другого потока
        // (считаем что чтение безопасно)

        ostringstream s;

        s << setw(15) << std::right << transport->toString()
          << "[ " << setw(7) << to_string(mode) << " ]"
          << " lastpacknum=" << packetnum
          << " items=" << items.size() << " maxAData=" << getADataSize() << " maxDData=" << getDDataSize()
          << " packsendpause[factor=" << packsendpauseFactor << "]=" << packsendpause
          << " sendpause=" << sendpause
          << endl
          << "\t   packs([sendfactor]=num): "
          << endl;

        for( const auto& p : mypacks )
        {
            s << "        \t[" << p.first << "]=" << p.second.size() << endl;
            size_t n = 0;

            for( const auto& pack : p.second )
            {
                //uniset_rwmutex_rlock l(p->mut);
                s << "        \t\t[" << (n++) << "]=" << sizeof(pack.msg) << " bytes"
                  << " (dataID=" << setw(5) << pack.msg.getDataID()
                  << " numA=" << setw(5) << pack.msg.asize()
                  << " numD=" << setw(5) << pack.msg.dsize()
                  << ")"
                  << endl;
            }
        }

        return s.str();
    }
    // -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr UNetSender::httpInfo( Poco::JSON::Object::Ptr root ) const
    {
        Poco::JSON::Object::Ptr json = root;

        if( !json )
            json = new Poco::JSON::Object();

        json->set("transport", transport->toString());
        json->set("mode", to_string(mode));
        json->set("lastpacknum", (int)packetnum);
        json->set("items", (int)items.size());
        json->set("maxAData", (int)getADataSize());
        json->set("maxDData", (int)getDDataSize());

        // params
        Poco::JSON::Object::Ptr params = new Poco::JSON::Object();
        params->set("sendpause", (int)sendpause);
        params->set("packsendpause", (int)packsendpause);
        params->set("packsendpauseFactor", (int)packsendpauseFactor);
        json->set("params", params);

        // packs info
        Poco::JSON::Array::Ptr packs = new Poco::JSON::Array();

        for( const auto& p : mypacks )
        {
            Poco::JSON::Object::Ptr packInfo = new Poco::JSON::Object();
            packInfo->set("sendfactor", (int)p.first);
            packInfo->set("count", (int)p.second.size());

            Poco::JSON::Array::Ptr packList = new Poco::JSON::Array();

            for( const auto& pack : p.second )
            {
                Poco::JSON::Object::Ptr pi = new Poco::JSON::Object();
                pi->set("dataID", (int)pack.msg.getDataID());
                pi->set("numA", (int)pack.msg.asize());
                pi->set("numD", (int)pack.msg.dsize());
                pi->set("bytes", (int)sizeof(pack.msg));
                packList->add(pi);
            }

            packInfo->set("packs", packList);
            packs->add(packInfo);
        }

        json->set("packGroups", packs);

        return json;
    }
#endif
    // -----------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
namespace std
{
    std::string to_string( const uniset::UNetSender::Mode& m )
    {
        if( m == uniset::UNetSender::Mode::mEnabled )
            return "Enabled";

        if( m == uniset::UNetSender::Mode::mDisabled )
            return "Disabled";

        return "";
    }
}
// -----------------------------------------------------------------------------
