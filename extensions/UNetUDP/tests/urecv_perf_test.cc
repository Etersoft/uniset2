#include <memory>
#include <chrono>
#include <string>
#include <Poco/Net/NetException.h>
#include "Debug.h"
#include "UNetReceiver.h"
#include "SMInterface.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
static shared_ptr<SMInterface> smi;
static shared_ptr<UInterface> ui;
static ObjectId myID = 6000;
static ObjectId shmID = 5003;
static int begPort = 50000;
// --------------------------------------------------------------------------
shared_ptr<SMInterface> smiInstance()
{
	if( smi == nullptr )
	{
		if( ui == nullptr )
			ui = make_shared<UInterface>();

		smi = make_shared<SMInterface>(shmID, ui, myID );
	}

	return smi;
}
// --------------------------------------------------------------------------
static void run_senders( size_t max, const std::string& s_host, size_t count = 50, timeout_t usecpause = 50 )
{
	std::vector< std::shared_ptr<UDPSocketU> > vsend;
	vsend.reserve(max);

	cout << "Run " << max << " senders (" << s_host << ")" << endl;

	// make sendesrs..
	for( size_t i = 0; i < max; i++ )
	{
		try
		{
			auto s = make_shared<UDPSocketU>(s_host, begPort + i);
			vsend.emplace_back(s);
		}
		catch( Poco::Net::NetException& e )
		{
			cerr << "(run_senders): " << e.displayText() << " (" << s_host << ")" << endl;
			throw;
		}
		catch( std::exception& ex)
		{
			cerr << "(run_senders): " << ex.what() << endl;
			throw;
		}
	}

	UniSetUDP::UDPMessage mypack;
	mypack.nodeID = 100;
	mypack.procID = 100;

	for( size_t i = 0; i < count; i++ )
	{
		UniSetUDP::UDPAData d(i, i);
		mypack.addAData(d);
	}

	for( size_t i = 0; i < count; i++ )
		mypack.addDData(i, i);

	for( size_t i = 0; i < max; i++ )
	{
		try
		{
			if( vsend[i] )
				vsend[i]->connect( Poco::Net::SocketAddress(s_host, begPort + i) );
		}
		catch( Poco::Net::NetException& e )
		{
			cerr << "(run_senders): " << e.message() << " (" << s_host << ")" << endl;
			throw;
		}
		catch( std::exception& ex)
		{
			cerr << "(run_senders): " << ex.what() << endl;
			throw;
		}
	}

	size_t packetnum = 0;
	UniSetUDP::UDPPacket s_buf;

	size_t nc = 1;

	while( nc )
	{
		mypack.num = packetnum++;

		// при переходе черех максимум (UniSetUDP::MaxPacketNum)
		// пакет опять должен иметь номер "1"
		if( packetnum == 0 )
			packetnum = 1;

		for( auto && udp : vsend )
		{
			try
			{
				if( udp->poll(100,Poco::Net::Socket::SELECT_WRITE) )
				{
					mypack.transport_msg(s_buf);
					size_t ret = udp->sendBytes((char*)&s_buf.data, s_buf.len);

					if( ret < s_buf.len )
						cerr << "(send): FAILED ret=" << ret << " < sizeof=" << s_buf.len << endl;
				}
			}
			catch( Poco::Net::NetException& e )
			{
				cerr << "(send): " << e.message() << " (" << s_host << ")" << endl;
			}
			catch( ... )
			{
				cerr << "(send): catch ..." << endl;
			}

		}

		usleep(usecpause);
	}
}
// --------------------------------------------------------------------------
static void run_test( size_t max, const std::string& host )
{
	std::vector< std::shared_ptr<UNetReceiver> > vrecv;
	vrecv.reserve(max);

	// make receivers..
	for( size_t i = 0; i < max; i++ )
	{
		auto r = make_shared<UNetReceiver>(host, begPort + i, smiInstance());
		r->setLockUpdate(true);
		vrecv.emplace_back(r);
	}

	size_t count = 0;

	// Run receivers..
	for( auto && r : vrecv )
	{
		if( r )
		{
			count++;
			r->start();
		}
	}

	cerr << "RUn " << count << " receivers..." << endl;

	// wait..
	pause();

	for( auto && r : vrecv )
	{
		if(r)
			r->stop();
	}
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[] )
{
	std::string host = "127.255.255.255";

	try
	{
		auto conf = uniset_init(argc, argv);

		if( argc > 1 && !strcmp(argv[1], "s") )
			run_senders(10, host);
		else
			run_test(10, host);

		return 0;
	}
	catch( const SystemError& err )
	{
		cerr << "(urecv-perf-test): " << err << endl;
	}
	catch( const Exception& ex )
	{
		cerr << "(urecv-perf-test): " << ex << endl;
	}
	catch( const std::exception& e )
	{
		cerr << "(tests_with_sm): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(urecv-perf-test): catch(...)" << endl;
	}

	return 1;
}
