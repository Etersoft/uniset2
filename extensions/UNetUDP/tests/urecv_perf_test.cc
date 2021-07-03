#include <memory>
#include <chrono>
#include <string>
#include <Poco/Net/NetException.h>
#include "Debug.h"
#include "UDPCore.h"
#include "unisetstd.h"
#include "UDPTransport.h"
#include "UNetReceiver.h"
#include "SMInterface.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
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
static void run_senders( size_t max, const std::string& s_host, size_t count = 3000, timeout_t usecpause = 50 )
{
    std::vector< std::shared_ptr<UDPSocketU> > vsend;
    vsend.reserve(max);

    cout << "Run " << max << " senders[" << max << "](" << s_host << ")[data=" << count << ", pause=" << usecpause << " usec]" << endl;

    // make sendesrs..
    for( size_t i = 0; i < max; i++ )
    {
        try
        {
            cout << "create sender: " << s_host << ":" << begPort + i << endl;
            auto s = make_shared<UDPSocketU>(s_host, begPort + i);
            s->setBroadcast(true);
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

    unet::UNetPacket mypack;
    mypack.set_magic(UniSetUDP::UNETUDP_MAGICNUM);
    mypack.set_nodeid(100);
    mypack.set_procid(100);

    for( size_t i = 0; i < count; i++ )
    {
        mypack.mutable_data()->add_aid(i);
        mypack.mutable_data()->add_avalue(i);
    }

    for( size_t i = 0; i < count; i++ )
    {
        mypack.mutable_data()->add_did(i);
        mypack.mutable_data()->add_dvalue(i);
    }

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
    size_t nc = 1;
    std::string s;

    while( nc ) // -V654
    {
        mypack.set_num(packetnum++);

        // при переходе черех максимум (UniSetUDP::MaxPacketNum)
        // пакет опять должен иметь номер "1"
        if( packetnum == 0 )
            packetnum = 1;

        s = mypack.SerializeAsString();

        for( auto&& udp : vsend )
        {
            try
            {
                if( udp->poll(100000, Poco::Net::Socket::SELECT_WRITE) )
                {
                    size_t ret = udp->sendBytes(s.data(), s.size());

                    if( ret < s.size() )
                        cerr << "(send): FAILED ret=" << ret << " < sizeof=" << s.size() << endl;
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

        std::this_thread::sleep_for(std::chrono::microseconds(usecpause));
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
        auto transport = unisetstd::make_unique<UDPReceiveTransport>(host, begPort + i);
        cout << "create receiver: " << host << ":" << begPort + i << endl;
        auto r = make_shared<UNetReceiver>(std::move(transport), smiInstance());
        r->setLockUpdate(true);
        r->setReceiveTimeout(5);
//        r->setBufferSize(1000);
        r->setUpdatePause(5);
        vrecv.emplace_back(r);
    }

    size_t count = 0;

    // Run receivers..
    for( auto&& r : vrecv )
    {
        if( r )
        {
            count++;
            r->start();
        }
    }

    cerr << "RUN " << count << " receivers..." << endl;

    // wait..
    pause();

    for( auto&& r : vrecv )
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

        int n = uniset::getArgInt("--num", argc, argv, "1");
        int dataCount = uniset::getArgInt("--count", argc, argv, "1000");

        if( n <= 0 )
        {
            cerr << "Process number must be > 0" << endl;
            return 1;
        }


        if( dataCount <= 0 )
        {
            cerr << "data count must be > 0" << endl;
            return 1;
        }

        if( argc > 1 && !strcmp(argv[1], "s") )
            run_senders(n, host, dataCount);
        else
            run_test(n, host);

        return 0;
    }
    catch( const SystemError& err )
    {
        cerr << "(urecv-perf-test): " << err << endl;
    }
    catch( const uniset::Exception& ex )
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
