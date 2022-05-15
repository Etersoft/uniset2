#include <cstdlib>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <Poco/Net/NetException.h>
#include "UDPPacket.h"
#include "PassiveTimer.h"
#include "UDPCore.h"
// --------------------------------------------------------------------------
static struct option longopts[] =
{
    { "help", no_argument, 0, 'h' },
    { "send", required_argument, 0, 's' },
    { "receive", required_argument, 0, 'r' },
    { "proc-id", required_argument, 0, 'p' },
    { "node-id", required_argument, 0, 'n' },
    { "send-pause", required_argument, 0, 'x' },
    { "timeout", required_argument, 0, 't' },
    { "data-count", required_argument, 0, 'c' },
    { "disable-broadcast", no_argument, 0, 'b' },
    { "show-data", no_argument, 0, 'd' },
    { "check-lost", no_argument, 0, 'l' },
    { "verbode", required_argument, 0, 'v' },
    { "num-cycles", required_argument, 0, 'z' },
    { "prof", required_argument, 0, 'y' },
    { "a-data", required_argument, 0, 'a' },
    { "d-data", required_argument, 0, 'i' },
    { "pack-num", required_argument, 0, 'u' },
    { NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::UniSetUDP;
using namespace std::chrono;
// --------------------------------------------------------------------------
enum Command
{
    cmdNOP,
    cmdSend,
    cmdReceive
};
// --------------------------------------------------------------------------
static bool split_addr( const string& addr, string& host, int& port )
{
    string::size_type pos = addr.rfind(':');

    if(  pos != string::npos )
    {
        host = addr.substr(0, pos);
        string s_port(addr.substr(pos + 1, addr.size() - 1));
        port = atoi(s_port.c_str());
        return true;
    }

    return false;
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int optindex = 0;
    int opt = 0;
    Command cmd = cmdNOP;
    int verb = 0;
    std::string addr = "";
    int port = 0;
    int msecpause = 200;
    timeout_t tout = UniSetTimer::WaitUpTime;
    bool broadcast = true;
    int procID = 1;
    int nodeID = 1;
    size_t count = 50;
    bool lost = false;
    bool show = false;
    size_t ncycles = 0;
    unsigned int nprof = 0;
    std::string d_data = "";
    std::string a_data = "";
    size_t packetnum = 1;

    while(1)
    {
        opt = getopt_long(argc, argv, "hs:c:r:p:n:t:x:blvdz:y:a:i:u:", longopts, &optindex);

        if( opt == -1 )
            break;

        switch (opt)
        {
            case 'h':
                cout << "-h|--help                - this message" << endl;
                cout << "[-s|--send] host:port    - Send message." << endl;
                cout << "[-c|--data-count] num    - Send num count of value. Default: 50." << endl;
                cout << "[-r|--receive] host:port - Receive message." << endl;
                cout << "[-p|--proc-id] id        - Set packet header. From 'procID'. Default: 1" << endl;
                cout << "[-n|--node-id] id        - Set packet header. From 'nodeID'. Default: 1" << endl;
                cout << "[-t|--timeout] msec      - timeout for receive. Default: 0 msec (waitup)." << endl;
                cout << "[-x|--send-pause] msec   - pause for send packets. Default: 200 msec." << endl;
                cout << "[-b|--disable-broadcast] - Disable broadcast mode." << endl;
                cout << "[-l|--check-lost]        - Check the lost packets." << endl;
                cout << "[-v|--verbose]           - verbose mode." << endl;
                cout << "[-d|--show-data]         - show receive data." << endl;
                cout << "[-z|--num-cycles] num    - Number of cycles of exchange. Default: -1 - infinitely." << endl;
                cout << "[-y|--prof] num          - Print receive statistics every NUM packets (for -r only)" << endl;
                cout << "[-a|--a-data] id1=val1,id2=val2,... - Analog data. Send: id1=id1,id2=id2,.. for analog sensors" << endl;
                cout << "[-i|--d-data] id1=val1,id2=val2,... - Digital data. Send: id1=id1,id2=id2,.. for digital sensors" << endl;
                cout << "[-u|--pack-num] num      - first packet number (default: 1)" << endl;
                cout << endl;
                return 0;

            case 'r':
                cmd = cmdReceive;
                addr = string(optarg);
                break;

            case 's':
                addr = string(optarg);
                cmd = cmdSend;
                break;

            case 'a':
                a_data = string(optarg);
                break;

            case 'i':
                d_data = string(optarg);
                break;

            case 't':
                tout = atoi(optarg);
                break;

            case 'x':
                msecpause = atoi(optarg);
                break;

            case 'y':
                nprof = atoi(optarg);
                break;

            case 'c':
                count = atoi(optarg);
                break;

            case 'p':
                procID = atoi(optarg);
                break;

            case 'n':
                nodeID = atoi(optarg);
                break;

            case 'b':
                broadcast = false;
                break;

            case 'd':
                show = true;
                break;

            case 'l':
                lost = true;
                break;

            case 'v':
                verb = 1;
                break;

            case 'z':
                ncycles = atoi(optarg);
                break;

            case 'u':
                packetnum = atoi(optarg);
                break;

            case '?':
            default:
                cerr << "? argumnet" << endl;
                return 0;
        }
    }

    if( cmd == cmdNOP )
    {
        cerr << "No command... Use -h for help" << endl;
        return -1;
    }

    try
    {
        string s_host;

        if( !split_addr(addr, s_host, port) )
        {
            cerr << "(main): Unknown 'host:port' for '" << addr << "'" << endl;
            return 1;
        }

        if( verb )
        {
            cout << " host=" << s_host
                 << " port=" << port
                 << " timeout=";

            if( tout == UniSetTimer::WaitUpTime )
                cout << "Waitup";
            else
                cout << tout;

            cout << " msecpause=" << msecpause << endl;
        }


        switch( cmd )
        {
            case cmdReceive:
            {
                UDPReceiveU udp(s_host, port);

                UniSetUDP::UDPMessage pack;
                unsigned long prev_num = 1;

                int nc = 1;

                if( ncycles > 0 )
                    nc = ncycles;

                auto t_start = high_resolution_clock::now();

                unsigned int npack = 0;

                while( nc )
                {
                    try
                    {
                        if( nprof > 0 && npack >= nprof )
                        {
                            auto t_end = high_resolution_clock::now();
                            float sec = duration_cast<duration<float>>(t_end - t_start).count();
                            cout << "Receive " << setw(5) << npack << " packets for " << setw(8) << sec << " sec "
                                 << " [ 1 packet per " << setw(10) << ( sec / (float)npack ) << " sec ]" << endl;
                            t_start = t_end;
                            npack = 0;
                        }

                        if( !udp.poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_READ) )
                        {
                            cout << "(recv): Timeout.." << endl;
                            continue;
                        }

                        size_t ret = udp.receiveBytes(&pack, sizeof(pack));

                        if( ret < 0 )
                        {
                            cerr << "(recv): no data?!" << endl;
                            continue;
                        }

                        if( ret == 0 )
                        {
                            cerr << "(recv): connection closed?!" << endl;
                            continue;
                        }

                        pack.ntoh();

                        if( pack.header.magic != UniSetUDP::UNETUDP_MAGICNUM )
                        {
                            cerr << "(recv): BAD PROTOCOL VERSION! [ need version '" << UniSetUDP::UNETUDP_MAGICNUM << "']" << endl;
                            continue;
                        }

                        if( lost )
                        {
                            if( prev_num != (pack.header.num - 1) )
                                cerr << "WARNING! Incorrect sequence of packets! current=" << pack.header.num
                                     << " prev=" << prev_num
                                     << " lost: " << std::labs(pack.header.num - prev_num)
                                     << endl;

                            prev_num = pack.header.num;
                        }

                        npack++;

                        if( verb )
                            cout << "receive OK:  "
                                 << " bytes: " << ret << endl;

                        if( show )
                            cout << "receive data: " << pack << endl;
                    }
                    catch( Poco::Net::NetException& e )
                    {
                        cerr << "(recv): " << e.displayText() << " (" << addr << ")" << endl;
                    }
                    catch( ... )
                    {
                        cerr << "(recv): catch ..." << endl;
                    }

                    if( ncycles > 0 )
                    {
                        nc--;

                        if( nc <= 0 )
                            break;
                    }
                }
            }
            break;

            case cmdSend:
            {
                std::shared_ptr<UDPSocketU> udp = make_shared<UDPSocketU>(s_host, port);
                udp->setBroadcast(broadcast);

                UniSetUDP::UDPMessage mypack;
                mypack.header.nodeID = nodeID;
                mypack.header.procID = procID;

                if( !a_data.empty() )
                {
                    auto vlist = uniset::getSInfoList(a_data, nullptr);

                    for( const auto& v : vlist )
                    {
                        UDPAData d(v.si.id, v.val);
                        mypack.addAData(d);
                    }
                }
                else
                {
                    for( size_t i = 0; i < count; i++ )
                    {
                        UDPAData d(i, i);
                        mypack.addAData(d);
                    }
                }

                if( !d_data.empty() )
                {
                    auto vlist = uniset::getSInfoList(d_data, nullptr);

                    for( const auto& v : vlist )
                        mypack.addDData(v.si.id, v.val);
                }
                else
                {
                    for( size_t i = 0; i < count; i++ )
                        mypack.addDData(i, i);
                }

                //                mypack.updatePacketCrc();

                Poco::Net::SocketAddress sa(s_host, port);
                udp->connect(sa);

                size_t nc = 1;

                if( ncycles > 0 )
                    nc = ncycles;

                while( nc )
                {
                    mypack.header.num = packetnum++;

                    // при переходе через максимум (UniSetUDP::MaxPacketNum)
                    // пакет опять должен иметь номер "1"
                    if( packetnum == 0 )
                        packetnum = 1;

                    try
                    {
                        if( udp->poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_WRITE) )
                        {
                            if( verb )
                                cout << "(send): to addr=" << addr << " d_count=" << mypack.header.dcount
                                     << " a_count=" << mypack.header.acount << endl;

                            size_t ret = udp->sendBytes(&mypack, sizeof(mypack) );

                            if( ret < sizeof(mypack) )
                                cerr << "(send): FAILED ret=" << ret << " < sizeof=" << sizeof(mypack) << endl;
                        }
                    }
                    catch( Poco::Net::NetException& e )
                    {
                        cerr << "(send): " << e.message() << " (" << addr << ")" << endl;
                    }
                    catch( ... )
                    {
                        cerr << "(send): catch ..." << endl;
                    }

                    if( ncycles > 0 )
                    {
                        nc--;

                        if( nc <= 0 )
                            break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(msecpause));
                }
            }
            break;

            default:
                cerr << endl << "Unknown command: '" << cmd << "'. Use -h for help" << endl;
                return -1;
                break;
        }
    }
    catch( const std::exception& e )
    {
        cerr << "(main): " << e.what() << endl;
    }
    catch( ... )
    {
        cerr << "(main): catch ..." << endl;
        return 1;
    }

    return 0;
}
// --------------------------------------------------------------------------
