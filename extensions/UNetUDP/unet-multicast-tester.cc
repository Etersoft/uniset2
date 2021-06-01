#include <cstdlib>
#include <errno.h>
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
#include "MulticastTransport.h"
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
    { "show-data", no_argument, 0, 'd' },
    { "check-lost", no_argument, 0, 'l' },
    { "verbode", required_argument, 0, 'v' },
    { "num-cycles", required_argument, 0, 'z' },
    { "prof", required_argument, 0, 'y' },
    { "a-data", required_argument, 0, 'a' },
    { "d-data", required_argument, 0, 'i' },
    { "group", required_argument, 0, 'g' },
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
    int usecpause = 2000000;
    timeout_t tout = UniSetTimer::WaitUpTime;
    int procID = 1;
    int nodeID = 1;
    size_t count = 50;
    bool lost = false;
    bool show = false;
    size_t ncycles = 0;
    unsigned int nprof = 0;
    std::string d_data = "";
    std::string a_data = "";
    std::vector<Poco::Net::IPAddress> groups;

    while(1)
    {
        opt = getopt_long(argc, argv, "hs:c:r:p:n:t:x:blvdz:y:a:i:g:", longopts, &optindex);

        if( opt == -1 )
            break;

        switch (opt)
        {
            case 'h':
                cout << "-h|--help                 - this message" << endl;
                cout << "[-s|--send] host:port     - Send message." << endl;
                cout << "[-c|--data-count] num     - Send num count of value. Default: 50." << endl;
                cout << "[-r|--receive] host:port  - Receive message." << endl;
                cout << "[-g|--group] ip           - Multicast group address (can be specified many times)" << endl;
                cout << "[-p|--proc-id] id         - Set packet header. From 'procID'. Default: 1" << endl;
                cout << "[-n|--node-id] id         - Set packet header. From 'nodeID'. Default: 1" << endl;
                cout << "[-t|--timeout] msec       - timeout for receive. Default: 0 msec (waitup)." << endl;
                cout << "[-x|--send-pause] msec    - pause for send packets. Default: 200 msec." << endl;
                cout << "[-l|--check-lost]         - Check the lost packets." << endl;
                cout << "[-v|--verbose]            - verbose mode." << endl;
                cout << "[-d|--show-data]          - show receive data." << endl;
                cout << "[-z|--num-cycles] num     - Number of cycles of exchange. Default: -1 - infinitely." << endl;
                cout << "[-y|--prof] num           - Print receive statistics every NUM packets (for -r only)" << endl;
                cout << "[-a|--a-data] id1=val1,id2=val2,... - Analog data. Send: id1=id1,id2=id2,.. for analog sensors" << endl;
                cout << "[-i|--d-data] id1=val1,id2=val2,... - Digital data. Send: id1=id1,id2=id2,.. for digital sensors" << endl;
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
                usecpause = atoi(optarg) * 1000;
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

            case 'g':
                groups.emplace_back(Poco::Net::IPAddress(optarg));
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

    if( groups.empty() )
    {
        cerr << "Multicast group address must be define. Use -g or -h for help" << endl;
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

            cout << " msecpause=" << usecpause / 1000
                 << endl;

            cout << " Groups: " << endl;

            for( const auto& g : groups )
                cout << "   " << g << endl;
        }


        switch( cmd )
        {
            case cmdReceive:
            {
                MulticastReceiveTransport udp(s_host, port, groups);

                udp.createConnection(true, 500, true);

                UniSetUDP::UDPMessage pack;
                UniSetUDP::UDPPacket buf;
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

                        if( !udp.isReadyForReceive(tout) )
                        {
                            cout << "(recv): Timeout.." << endl;
                            continue;
                        }

                        size_t ret = udp.receive(&(buf.data), sizeof(buf.data) );
                        size_t sz = UniSetUDP::UDPMessage::getMessage(pack, buf);

                        if( sz == 0 )
                        {
                            if( pack.magic != UniSetUDP::UNETUDP_MAGICNUM )
                                cerr << "(recv): BAD PROTOCOL VERSION! [ need version '" << UniSetUDP::UNETUDP_MAGICNUM << "']" << endl;
                            else
                                cerr << "(recv): FAILED header ret=" << ret
                                     << " sizeof=" << sz << endl;

                            continue;
                        }

                        if( lost )
                        {
                            if( prev_num != (pack.num - 1) )
                                cerr << "WARNING! Incorrect sequence of packets! current=" << pack.num
                                     << " prev=" << prev_num << endl;

                            prev_num = pack.num;
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
                auto udp = std::make_shared<MulticastSendTransport>(s_host, port, groups);

                UniSetUDP::UDPMessage mypack;
                mypack.nodeID = nodeID;
                mypack.procID = procID;

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

                udp->createConnection(true, 500);
                size_t packetnum = 0;

                UniSetUDP::UDPPacket s_buf;

                size_t nc = 1;

                if( ncycles > 0 )
                    nc = ncycles;

                while( nc )
                {
                    mypack.num = packetnum++;

                    // при переходе черех максимум (UniSetUDP::MaxPacketNum)
                    // пакет опять должен иметь номер "1"
                    if( packetnum == 0 )
                        packetnum = 1;

                    try
                    {
                        if( udp->isReadyForSend(tout) )
                        {
                            mypack.transport_msg(s_buf);

                            if( verb )
                                cout << "(send): to addr=" << addr << " d_count=" << mypack.dcount
                                     << " a_count=" << mypack.acount << " bytes=" << s_buf.len << endl;

                            size_t ret = udp->send((char*)&s_buf.data, s_buf.len);

                            if( ret < s_buf.len )
                                cerr << "(send): FAILED ret=" << ret << " < sizeof=" << s_buf.len << endl;
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

                    std::this_thread::sleep_for(std::chrono::microseconds(usecpause));
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
