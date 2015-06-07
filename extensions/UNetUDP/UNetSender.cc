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
UNetSender::UNetSender( const std::string& s_host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi,
						const std::string& s_f, const std::string& s_val ):
	s_field(s_f),
	s_fvalue(s_val),
	shm(smi),
	s_host(s_host),
	sendpause(150),
	activated(false),
	dlist(100),
	maxItem(0),
	packetnum(1),
	lastcrc(0)
{

	{
		ostringstream s;
		s << "S(" << setw(15) << s_host << ":" << setw(4) << port << ")";
		myname = s.str();
	}

	unetlog = make_shared<DebugStream>();
	unetlog->setLogName(myname);


	// определяем фильтр
	//    s_field = conf->getArgParam("--udp-filter-field");
	//    s_fvalue = conf->getArgParam("--udp-filter-value");
	unetinfo << myname << "(init): read filter-field='" << s_field
			 << "' filter-value='" << s_fvalue << "'" << endl;

	unetinfo << "(UNetSender): UDP set to " << s_host << ":" << port << endl;

	ost::Thread::setException(ost::Thread::throwException);

	try
	{
		addr = s_host.c_str();
		udp = make_shared<ost::UDPBroadcast>(addr, port);
	}
	catch( const std::exception& e )
	{
		ostringstream s;
		s << myname << ": " << e.what();
		unetcrit << s.str() << std::endl;
		throw SystemError(s.str());
	}
	catch( ... )
	{
		ostringstream s;
		s << myname << ": catch...";
		unetcrit << s.str() << std::endl;
		throw SystemError(s.str());
	}

	s_thr = make_shared< ThreadCreator<UNetSender> >(this, &UNetSender::send);

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


	// выставляем поля, которые не меняются
	mypack.nodeID = uniset_conf()->getLocalNode();
	mypack.procID = shm->ID();
}
// -----------------------------------------------------------------------------
UNetSender::~UNetSender()
{
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
		mypack.setDData(it->pack_ind, value);
	}
	else if( it->iotype == UniversalIO::AI || it->iotype == UniversalIO::AO )
	{
		UniSetTypes::uniset_rwmutex_wrlock l(pack_mutex);
		mypack.setAData(it->pack_ind, value);
	}
}
// -----------------------------------------------------------------------------
void UNetSender::send()
{
	dlist.resize(maxItem);
	unetinfo << myname << "(send): dlist size = " << dlist.size() << endl;

	/*
	    ost::IPV4Broadcast h = s_host.c_str();
	    try
	    {
	        udp->setPeer(h,port);
	    }
	    catch( ost::SockException& e )
	    {
	        ostringstream s;
	        s << e.getString() << ": " << e.getSystemErrorString();
			unetcrit << myname << "(poll): " << s.str() << endl;
	        throw SystemError(s.str());
	    }
	*/
	while( activated )
	{
		try
		{
			if( !shm->isLocalwork() )
				updateFromSM();

			real_send();
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

		msleep(sendpause);
	}

	unetinfo << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
// #define UNETUDP_DISABLE_OPTIMIZATION_N1

void UNetSender::real_send()
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

	if( packetnum > UniSetUDP::MaxPacketNum )
		packetnum = 1;

	if( !udp->isPending(ost::Socket::pendingOutput) )
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

	UItem p;
	p.iotype = UniSetTypes::getIOType(it.getProp("iotype"));

	if( p.iotype == UniversalIO::UnknownIOType )
	{
		unetcrit << myname << "(readItem): Unknown iotype for sid=" << sid << endl;
		return false;
	}

	p.id = sid;

	if( p.iotype == UniversalIO::DI || p.iotype == UniversalIO::DO )
	{
		p.pack_ind = mypack.addDData(sid, 0);

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
		p.pack_ind = mypack.addAData(sid, 0);

		if ( p.pack_ind >= UniSetUDP::MaxACount )
		{
			unetcrit << myname
					 << "(readItem): OVERFLOW! MAX UDP ANALOG DATA LIMIT! max="
					 << UniSetUDP::MaxACount << endl;
			raise(SIGTERM);
			return false;
		}
	}

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
const std::string UNetSender::getShortInfo() const
{
	// warning: будет вызываться из другого потока
	// (считаем что чтение безопасно)

	ostringstream s;

	s << setw(15) << std::right << getAddress() << ":" << std::left << setw(6) << getPort()
	  << " packetnum=" << packetnum
	  << " lastcrc=" << setw(6) << lastcrc;

	return std::move(s.str());
}
// -----------------------------------------------------------------------------

