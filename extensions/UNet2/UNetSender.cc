#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetSender.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
UNetSender::UNetSender( const std::string s_host, const ost::tpport_t port, SMInterface* smi,
						const std::string s_f, const std::string s_val, SharedMemory* ic ):
s_field(s_f),
s_fvalue(s_val),
shm(smi),
s_host(s_host),
sendpause(150),
activated(false),
dlist(100),
maxItem(0),
packetnum(1),
s_thr(0)
{

	{
		ostringstream s;
		s << "S(" << s_host << ":" << port << ")";
		myname = s.str();
	}

	// определяем фильтр
//	s_field = conf->getArgParam("--udp-filter-field");
//	s_fvalue = conf->getArgParam("--udp-filter-value");
	dlog[Debug::INFO] << myname << "(init): read filter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(UNetSender): UDP set to " << s_host << ":" << port << endl;

	ost::Thread::setException(ost::Thread::throwException);
	try
	{
		addr = s_host.c_str();
		udp = new ost::UDPBroadcast(addr,port);
	}
	catch( std::exception& e )
	{
		ostringstream s;
		s << myname << ": " << e.what();
		dlog[Debug::CRIT] << s.str() << std::endl;
		throw SystemError(s.str());
	}
	catch( ... )
	{
		ostringstream s;
		s << myname << ": catch...";
		dlog[Debug::CRIT] << s.str() << std::endl;
		throw SystemError(s.str());
	}

	s_thr = new ThreadCreator<UNetSender>(this, &UNetSender::send);

	// -------------------------------
	if( shm->isLocalwork() )
	{
		readConfiguration();
		dlist.resize(maxItem);
		dlog[Debug::INFO] << myname << "(init): dlist size = " << dlist.size() << endl;
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&UNetSender::readItem) );


	// выставляем поля, которые не меняются
	mypack.nodeID = conf->getLocalNode();
	mypack.procID = shm->ID();
}
// -----------------------------------------------------------------------------
UNetSender::~UNetSender()
{
	delete s_thr;
	delete udp;
	delete shm;
}
// -----------------------------------------------------------------------------
void UNetSender::updateFromSM()
{
	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); ++it )
	{
		long value = 0;

		if( it->iotype == UniversalIO::DigitalInput || it->iotype == UniversalIO::DigitalOutput )
			value = shm->localGetState(it->dit,it->id) ? 1 : 0;
		else if( it->iotype == UniversalIO::AnalogInput || it->iotype == UniversalIO::AnalogOutput )
			value = shm->localGetValue(it->ait,it->id);
		else
		{
			dlog[Debug::CRIT] << myname << "(update): Unknown iotype for sid=" << it->id << endl;
			continue;
		}

		updateItem(it,value);
	}
}
// -----------------------------------------------------------------------------
void UNetSender::updateSensor( UniSetTypes::ObjectId id, long value )
{
//	cerr << myname << ": UPDATE SENSOR id=" << id << " value=" << value << endl;
	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); ++it )
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

	if( it->iotype == UniversalIO::DigitalInput || it->iotype == UniversalIO::DigitalOutput )
	{
		UniSetTypes::uniset_mutex_lock l(pack_mutex,100);
		mypack.setDData(it->pack_ind,value);
	}
	else if( it->iotype == UniversalIO::AnalogInput || it->iotype == UniversalIO::AnalogOutput )
	{
		UniSetTypes::uniset_mutex_lock l(pack_mutex,100);
		mypack.setAData(it->pack_ind,value);
	}
}
// -----------------------------------------------------------------------------
void UNetSender::send()
{
	dlist.resize(maxItem);
	dlog[Debug::INFO] << myname << "(send): dlist size = " << dlist.size() << endl;
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
		dlog[Debug::CRIT] << myname << "(poll): " << s.str() << endl;
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
			dlog[Debug::WARN]  << myname << "(send): " << e.getString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
		{
			dlog[Debug::WARN] << myname << "(send): " << ex << std::endl;
		}
		catch( std::exception& e )
		{
			dlog[Debug::WARN] << myname << "(send): " << e.what() << std::endl;
		}
		catch(...)
		{
			dlog[Debug::WARN] << myname << "(send): catch ..." << std::endl;
		}

		msleep(sendpause);
	}

	dlog[Debug::INFO] << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
void UNetSender::real_send()
{
	UniSetTypes::uniset_mutex_lock l(pack_mutex,300);
	mypack.num = packetnum++;

	if( packetnum > UniSetUDP::MaxPacketNum )
		packetnum = 1;

	if( !udp->isPending(ost::Socket::pendingOutput) )
		return;


	mypack.transport_msg(s_msg);
	size_t ret = udp->send( (char*)s_msg.data, s_msg.len );
	if( ret < s_msg.len )
		dlog[Debug::CRIT] << myname << "(real_send): FAILED ret=" << ret << " < sizeof=" << s_msg.len << endl;
}
// -----------------------------------------------------------------------------
void UNetSender::stop()
{
	activated = false;
//	s_thr->stop();
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
	xmlNode* root = conf->getXMLSensorsSection();
	if(!root)
	{
		ostringstream err;
		err << myname << "(readConfiguration): not found <sensors>";
		throw SystemError(err.str());
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		std::cerr << myname << "(readConfiguration): empty <sensors>?!!" << endl;
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( check_filter(it,s_field,s_fvalue) )
			initItem(it);
	}
}
// ------------------------------------------------------------------------------------------
bool UNetSender::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
		initItem(it);
	return true;
}
// ------------------------------------------------------------------------------------------
bool UNetSender::initItem( UniXML_iterator& it )
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
		sid = conf->getSensorID(sname);

	if( sid == DefaultObjectId )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(readItem): ID not found for "
							<< sname << endl;
		return false;
	}

	UItem p;
	p.iotype = UniSetTypes::getIOType(it.getProp("iotype"));

	if( p.iotype == UniversalIO::UnknownIOType )
	{
		dlog[Debug::CRIT] << myname << "(readItem): Unknown iotype for sid=" << sid << endl;
		return false;
	}

	p.id = sid;

	if( p.iotype == UniversalIO::DigitalInput || p.iotype == UniversalIO::DigitalOutput )
	{
		p.pack_ind = mypack.addDData(sid,0);
		if ( p.pack_ind >= UniSetUDP::MaxDCount )
		{
			dlog[Debug::CRIT] << myname
					<< "(readItem): OVERFLOW! MAX UDP DIGITAL DATA LIMIT! max="
					<< UniSetUDP::MaxDCount << endl;
			return false;
		}
	}
	else if( p.iotype == UniversalIO::AnalogInput || p.iotype == UniversalIO::AnalogOutput )
	{
		p.pack_ind = mypack.addAData(sid,0);
		if ( p.pack_ind >= UniSetUDP::MaxACount )
		{
			dlog[Debug::CRIT] << myname
					<< "(readItem): OVERFLOW! MAX UDP ANALOG DATA LIMIT! max="
					<< UniSetUDP::MaxACount << endl;
			return false;
		}
	}

	if( maxItem >= dlist.size() )
		dlist.resize(maxItem+10);

	dlist[maxItem] = p;
	maxItem++;

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;

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
	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); it++ )
	{
		shm->initDIterator(it->dit);
		shm->initAIterator(it->ait);
	}
}
// -----------------------------------------------------------------------------
void UNetSender::askSensors( UniversalIO::UIOCommand cmd )
{
	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); it++ )
		shm->askSensor(it->id,cmd);
}
// -----------------------------------------------------------------------------
