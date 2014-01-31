#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UDPNReceiver.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
UDPNReceiver::UDPNReceiver( ost::tpport_t p, ost::IPV4Host h, UniSetTypes::ObjectId shmId, IONotifyController* ic ):
shm(0),
ui(conf),
activate(false),
udp(0),
host(h),
port(p),
recvTimeout(5000),
conn(false)
{
	{
		ostringstream s;
		s << host << ":" << port;
		myname = s.str();
	}

	shm = new SMInterface(shmId,&ui,DefaultObjectId,ic);

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(UDPNReceiver): UDP set to " << host << ":" << port << endl;

	try
	{
		udp = new ost::UDPDuplex(host,port);
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString() << endl;
		throw SystemError(s.str());
	}

	thr = new ThreadCreator<UDPNReceiver>(this, &UDPNReceiver::poll);
	thr->start();
}
// -----------------------------------------------------------------------------
UDPNReceiver::~UDPNReceiver()
{
	delete udp;
	delete shm;
	delete thr;
}
// -----------------------------------------------------------------------------
void UDPNReceiver::poll()
{
	while( 1 )
	{
		if( !activate )
		{
			msleep(1000);
			continue;
		}

		try
		{
			recv();
		}
		catch( ost::SockException& e )
		{
			cerr  << e.getString() << ": " << e.getSystemErrorString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
		{
			cerr << myname << "(step): " << ex << std::endl;
		}
		catch(...)
		{
			cerr << myname << "(step): catch ..." << std::endl;
		}
	}

	cerr << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
void UDPNReceiver::recv()
{
	cout << myname << ": recv....(timeout=" << recvTimeout << ")" << endl;
//	UniSetUDP::UDPHeader h;
	// receive
	if( udp->isInputReady(recvTimeout) )
	{
/*
		ssize_t ret = udp->UDPReceive::receive(&h,sizeof(h));
		if( ret<(ssize_t)sizeof(h) )
		{
			cerr << myname << "(receive): ret=" << ret << " sizeof=" << sizeof(h) << endl;
			return;
		}

		cout << myname << "(receive): header: " << h << endl;
		if( h.dcount <=0 )
		{
			cout << " data=0" << endl;
			return;
		}
*/
		UniSetUDP::UDPData d;
		// ignore echo...
#if 0
		if( h.nodeID == conf->getLocalNode() && h.procID == getId() )
		{
			for( int i=0; i<h.dcount;i++ )
			{
				ssize_t ret = udp->UDPReceive::receive(&d,sizeof(d));
				if( ret < (ssize_t)sizeof(d) )
					return;
			}
			return;
		}
#endif
		cout << "***** request: " << udp->UDPSocket::getIPV4Peer() << endl;
		for( int i=0; i<100;i++ )
		{
			ssize_t ret = udp->UDPReceive::receive(&d,sizeof(d));
			if( ret<(ssize_t)sizeof(d) )
			{
				cerr << myname << "(receive data " << i << "): ret=" << ret << " sizeof=" << sizeof(d) << endl;
				break;
			}

			cout << myname << "(receive data " << i << "): " << d << endl;
		}
	}
//	else
//	{
//		cout << "no InputReady.." << endl;
//	}
}
// -----------------------------------------------------------------------------
