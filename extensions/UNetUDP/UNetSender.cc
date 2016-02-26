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
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetSender.h"
#include "UNetLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
UNetSender::UNetSender(const std::string& s_host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi,
					   bool nocheckConnection, const std::string& s_f, const std::string& s_val,
					   const std::string& s_prefix, size_t maxDCount, size_t maxACount ):
	s_field(s_f),
	s_fvalue(s_val),
	prefix(s_prefix),
	shm(smi),
	s_host(s_host),
	sendpause(150),
	packsendpause(5),
	activated(false),
	dlist(100),
	maxItem(0),
	packetnum(1),
	lastcrc(0),
	maxAData(maxACount),
	maxDData(maxDCount)
{

	{
		ostringstream s;
		s << "S(" << setw(15) << s_host << ":" << setw(4) << port << ")";
		myname = s.str();
	}

	unetlog = make_shared<DebugStream>();
	unetlog->setLogName(myname);

	auto conf = uniset_conf();
	conf->initLogStream(unetlog, myname);

	// определяем фильтр
	//    s_field = conf->getArgParam("--udp-filter-field");
	//    s_fvalue = conf->getArgParam("--udp-filter-value");
	unetinfo << myname << "(init): read filter-field='" << s_field
			 << "' filter-value='" << s_fvalue << "'" << endl;

	unetinfo << "(UNetSender): UDP set to " << s_host << ":" << port << endl;

	addr = s_host.c_str();

	ptCheckConnection.setTiming(10000); // default 10 сек
	createConnection(nocheckConnection);

	s_thr = make_shared< ThreadCreator<UNetSender> >(this, &UNetSender::send);

	mypacks[0].resize(1);
	packs_anum[0] = 0;
	packs_dnum[0] = 0;
	UniSetUDP::UDPMessage& mypack(mypacks[0][0]);
	// выставляем поля, которые не меняются
	mypack.nodeID = uniset_conf()->getLocalNode();
	mypack.procID = shm->ID();

	// -------------------------------
	if( shm->isLocalwork() )
	{
		readConfiguration();
		dlist.resize(maxItem);
		unetinfo << myname << "(init): dlist size = " << dlist.size() << endl;
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
			dlist.resize(maxItem);
			unetinfo << myname << "(init): dlist size = " << dlist.size() << endl;
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
	ost::Thread::setException(ost::Thread::throwException);

	unetinfo << myname << "(createConnection): .." << endl;

	try
	{
		udp = make_shared<ost::UDPBroadcast>(addr, port);
	}
	catch( const std::exception& e )
	{
		ostringstream s;
		s << myname << "(createConnection): " << e.what();
		unetcrit << s.str() << std::endl;
		if( throwEx )
			throw SystemError(s.str());

		udp = nullptr;
	}
	catch( ... )
	{
		ostringstream s;
		s << myname << "(createConnection): catch...";
		unetcrit << s.str() << std::endl;
		if( throwEx )
			throw SystemError(s.str());

		udp = nullptr;
	}

	return (udp != nullptr);
}
// -----------------------------------------------------------------------------
void UNetSender::updateFromSM()
{
	auto it = dlist.begin();

	for( ; it != dlist.end(); ++it )
	{
		long value = shm->localGetValue(it->ioit, it->id);
		updateItem(it, value);
	}
}
// -----------------------------------------------------------------------------
void UNetSender::updateSensor( UniSetTypes::ObjectId id, long value )
{
	if( !shm->isLocalwork() )
		return;

	//    cerr << myname << ": UPDATE SENSOR id=" << id << " value=" << value << endl;
	auto it = dlist.begin();

	for( ; it != dlist.end(); ++it )
	{
		if( it->id == id )
		{
			updateItem( it, value );
			break;
		}
	}
}
// -----------------------------------------------------------------------------
void UNetSender::updateItem( DMap::iterator& it, long value )
{
	if( it == dlist.end() )
		return;

	if( it->iotype == UniversalIO::DI || it->iotype == UniversalIO::DO )
	{
		UniSetTypes::uniset_rwmutex_wrlock l(pack_mutex);
		auto& pk = mypacks[it->pack_sendfactor];
		UniSetUDP::UDPMessage& mypack(pk[it->pack_num]);
		mypack.setDData(it->pack_ind, value);
	}
	else if( it->iotype == UniversalIO::AI || it->iotype == UniversalIO::AO )
	{
		UniSetTypes::uniset_rwmutex_wrlock l(pack_mutex);
		auto& pk = mypacks[it->pack_sendfactor];
		UniSetUDP::UDPMessage& mypack(pk[it->pack_num]);
		mypack.setAData(it->pack_ind, value);
	}
}
// -----------------------------------------------------------------------------
void UNetSender::setCheckConnectionPause( int msec )
{
	if( msec > 0 )
		ptCheckConnection.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetSender::send()
{
	dlist.resize(maxItem);
	unetinfo << myname << "(send): dlist size = " << dlist.size() << endl;
	ncycle = 0;

	ptCheckConnection.reset();

	while( activated )
	{
		if( !udp )
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

		try
		{
			if( !shm->isLocalwork() )
				updateFromSM();

			for( auto && it : mypacks )
			{
				if( it.first > 1 && (ncycle % it.first) != 0 )
					continue;

				if( !activated )
					break;

				auto& pk = it.second;
				int size = pk.size();

				for(int i = 0; i < size; ++i)
				{
					if( !activated )
						break;

					real_send(pk[i]);
					msleep(packsendpause);
				}
			}

			ncycle++;
		}
		catch( ost::SockException& e )
		{
			unetwarn << myname << "(send): " << e.getString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
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
// #define UNETUDP_DISABLE_OPTIMIZATION_N1

void UNetSender::real_send(UniSetUDP::UDPMessage& mypack)
{
	UniSetTypes::uniset_rwmutex_rlock l(pack_mutex);
#ifdef UNETUDP_DISABLE_OPTIMIZATION_N1
	mypack.num = packetnum++;
#else
	unsigned short crc = mypack.getDataCRC();

	if( crc != lastcrc )
	{
		mypack.num = packetnum++;
		lastcrc = crc;
	}

#endif

	// при переходе через ноль (когда счётчик перевалит через UniSetUDP::MaxPacketNum..
	// делаем номер пакета "1"
	if( packetnum == 0 )
		packetnum = 1;

	if( !udp || !udp->isPending(ost::Socket::pendingOutput) )
		return;

	mypack.transport_msg(s_msg);
	size_t ret = udp->send( (char*)s_msg.data, s_msg.len );

	if( ret < s_msg.len )
		unetcrit << myname << "(real_send): FAILED ret=" << ret << " < sizeof=" << s_msg.len << endl;
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
	if( UniSetTypes::check_filter(it, s_field, s_fvalue) )
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
		sid = UniSetTypes::uni_atoi(tid);

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

	int priority = it.getPIntProp(prefix + "_sendfactor", 0);

	auto pk = mypacks[priority];

	UItem p;
	p.iotype = UniSetTypes::getIOType(it.getProp("iotype"));
	p.pack_sendfactor = priority;

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

		UniSetUDP::UDPMessage& mypack(pk[dnum]);

		p.pack_ind = mypack.addDData(sid, 0);

		if( p.pack_ind >= maxDData )
		{
			dnum++;

			if( dnum >= pk.size() )
				pk.resize(dnum + 1);

			UniSetUDP::UDPMessage& mypack( pk[dnum] );
			p.pack_ind = mypack.addDData(sid, 0);
			mypack.nodeID = uniset_conf()->getLocalNode();
			mypack.procID = shm->ID();
		}

		p.pack_num = dnum;
		packs_anum[priority] = dnum;

		if ( p.pack_ind >= UniSetUDP::MaxDCount )
		{
			unetcrit << myname
					 << "(readItem): OVERFLOW! MAX UDP DIGITAL DATA LIMIT! max="
					 << UniSetUDP::MaxDCount << endl;

			raise(SIGTERM);
			return false;
		}
	}
	else if( p.iotype == UniversalIO::AI || p.iotype == UniversalIO::AO )
	{
		size_t anum = packs_anum[priority];

		if( pk.size() <= anum )
			pk.resize(anum + 1);

		UniSetUDP::UDPMessage& mypack(pk[anum]);

		p.pack_ind = mypack.addAData(sid, 0);

		if( p.pack_ind >= maxAData )
		{
			anum++;

			if( anum >= pk.size() )
				pk.resize(anum + 1);

			UniSetUDP::UDPMessage& mypack(pk[anum]);
			p.pack_ind = mypack.addAData(sid, 0);
			mypack.nodeID = uniset_conf()->getLocalNode();
			mypack.procID = shm->ID();
		}

		p.pack_num = anum;
		packs_anum[priority] = anum;

		if ( p.pack_ind >= UniSetUDP::MaxACount )
		{
			unetcrit << myname
					 << "(readItem): OVERFLOW! MAX UDP ANALOG DATA LIMIT! max="
					 << UniSetUDP::MaxACount << endl;

			raise(SIGTERM);
			return false;
		}
	}

	mypacks[priority] = pk;

	if( maxItem >= dlist.size() )
		dlist.resize(maxItem + 10);

	dlist[maxItem] = p;
	maxItem++;

	unetinfo << myname << "(initItem): add " << p << endl;
	return true;
}

// ------------------------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, UNetSender::UItem& p )
{
	return os << " sid=" << p.id;
}
// -----------------------------------------------------------------------------
void UNetSender::initIterators()
{
	for( auto && it : dlist )
		shm->initIterator(it.ioit);
}
// -----------------------------------------------------------------------------
void UNetSender::askSensors( UniversalIO::UIOCommand cmd )
{
	for( auto && it : dlist  )
		shm->askSensor(it.id, cmd);
}
// -----------------------------------------------------------------------------
size_t UNetSender::getDataPackCount() const
{
	return mypacks.size();
}
// -----------------------------------------------------------------------------
const std::string UNetSender::getShortInfo() const
{
	// warning: будет вызываться из другого потока
	// (считаем что чтение безопасно)

	ostringstream s;

	s << setw(15) << std::right << getAddress() << ":" << std::left << setw(6) << getPort()
	  << " lastpacknum=" << packetnum
	  << " lastcrc=" << setw(6) << lastcrc
	  << " items=" << maxItem << " maxAData=" << getADataSize() << " maxDData=" << getDDataSize()
	  << endl
	  << "\t   packs([sendfactor]=num): "
	  << endl;

	for( auto i = mypacks.begin(); i != mypacks.end(); ++i )
		s << "        \t\t[" << i->first << "]=" << i->second.size() << endl;

	return std::move(s.str());
}
// -----------------------------------------------------------------------------
