#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "UDPPacket.h"
#include "UDPCore.h"
// -----------------------------------------------------------------------------
// include-ы искплючительно для того, чтобы их обработал gcov (покрытие кода)
#include "UNetReceiver.h"
#include "UNetSender.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static int port = 3000;
static const std::string host("127.255.255.255");
static shared_ptr<UInterface> ui = nullptr;
static ObjectId aid = 2;
static std::shared_ptr<UDPReceiveU> udp_r = nullptr;
static shared_ptr<UDPSocketU> udp_s = nullptr;
static int s_port = 3003; // Node2
static int s_nodeID = 3003;
static int s_procID = 123;
static int s_numpack = 1;
static Poco::Net::SocketAddress s_addr(host, s_port);
static ObjectId node2_respond_s = 12;
static ObjectId node2_lostpackets_as = 13;
static int maxDifferense = 5; // см. unetudp-test-configure.xml --unet-maxdifferense
static int recvTimeout = 1000; // --unet-recv-timeout
static ObjectId node1_numchannel_as = 14;
static ObjectId node1_channelSwitchCount_as = 15;
// -----------------------------------------------------------------------------
void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( ui == nullptr )
    {
        ui = make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(aid, 10000) );
    }

    if( udp_r == nullptr )
        udp_r = make_shared<UDPReceiveU>(host, port);

    if( udp_s == nullptr )
    {
        udp_s = make_shared<UDPSocketU>(); //(host, s_port);
        udp_s->setBroadcast(true);
    }
}
// -----------------------------------------------------------------------------
// pnum - минималный номер ожидаемого пакета ( 0 - любой пришедщий )
// ncycle - сколько пакетов разрешено "пропустить" прежде чем дождёмся нужного. (чтобы не ждать бесконечно)
static UniSetUDP::UDPMessage receive( unsigned int pnum = 0, timeout_t tout = 2000, int ncycle = 20 )
{
    UniSetUDP::UDPMessage pack;

    while( ncycle > 0 )
    {
        if( !udp_r->poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_READ) )
            break;

        size_t ret = udp_r->receiveBytes(&pack, sizeof(pack) );

        if( ret <= 0 || pnum == 0 || ( pnum > 0 && pack.header.num >= pnum ) ) // -V560
            break;

        REQUIRE( pack.header.magic == UniSetUDP::UNETUDP_MAGICNUM );
        ncycle--;
    }

    //  if( pnum > 0 && pack.num < pnum )
    //      return UniSetUDP::UDPMessage(); // empty message

    return pack;
}
// -----------------------------------------------------------------------------
void send( UniSetUDP::UDPMessage& pack, int tout = 2000 )
{
    CHECK( udp_s->poll(UniSetTimer::millisecToPoco(tout), Poco::Net::Socket::SELECT_WRITE) );

    pack.header.nodeID = s_nodeID;
    pack.header.procID = s_procID;
    pack.header.num = s_numpack++;

    size_t ret = udp_s->sendTo(&pack, sizeof(pack), s_addr);
    REQUIRE( ret == sizeof(pack) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: repack", "[unetudp][udp][repack]")
{
    UniSetUDP::UDPMessage pack;

    cerr << "UniSetUDP::UDPMessage size: "
         << sizeof(UniSetUDP::UDPMessage)
         << " [MaxACount=" << UniSetUDP::MaxACount
         << " MaxDCount=" << UniSetUDP::MaxDCount
         << "]"
         << endl;

    REQUIRE( sizeof(UniSetUDP::UDPMessage) < 65507 ); // UDP packet

    pack.header.nodeID = 100;
    pack.header.procID = 100;
    pack.header.num = 1;
    pack.addDData(1, 1);
    pack.addDData(2, 0);
    pack.addAData(3, 30);
    pack.addAData(4, 40);

    REQUIRE(pack.header.magic == UniSetUDP::UNETUDP_MAGICNUM);

    UniSetUDP::UDPMessage pack2(pack);
    pack2.ntoh();

    REQUIRE(pack2.header.nodeID == 100);
    REQUIRE(pack2.header.procID == 100);
    REQUIRE(pack2.header.num == 1);
    REQUIRE(pack2.header.magic == UniSetUDP::UNETUDP_MAGICNUM);
    REQUIRE(pack2.dID(0) == 1);
    REQUIRE(pack2.dValue(0) == true);
    REQUIRE(pack2.dID(1) == 2);
    REQUIRE(pack2.dValue(1) == false);
    REQUIRE(pack2.dID(1) == 2);
    REQUIRE(pack2.a_dat[0].id == 3);
    REQUIRE(pack2.a_dat[0].val == 30);
    REQUIRE(pack2.a_dat[1].id == 4);
    REQUIRE(pack2.a_dat[1].val == 40);
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: UDPMessage", "[unetudp][udp][udpmessage]")
{
    SECTION("UDPMessage::isFull()")
    {
        UniSetUDP::UDPMessage u;

        for( unsigned int i = 0; i < UniSetUDP::MaxACount - 1; i++ )
            u.addAData( i, i );

        REQUIRE( u.asize() == (UniSetUDP::MaxACount - 1) );

        CHECK_FALSE( u.isAFull() );
        u.addAData( 1, 1 );
        CHECK( u.isAFull() );

        for( unsigned int i = 0; i < UniSetUDP::MaxDCount - 1; i++ )
            u.addDData( i, true );

        REQUIRE( u.dsize() == (UniSetUDP::MaxDCount - 1) );

        CHECK_FALSE( u.isDFull() );
        u.addDData( 1, true );
        CHECK( u.isDFull() );

        CHECK( u.isFull() );
    }
}
// -----------------------------------------------------------------------------
#if 0
TEST_CASE("[UNetUDP]: respond sensor", "[unetudp][udp]")
{
    InitTest();

    ObjectId node1_not_respond_s = 1;
    CHECK( ui->getValue(node1_not_respond_s) == 0 );

    // в запускающем файле стоит --unet-recv-timeout 1000
    // ждём больше, чтобы сработал timeout
    msleep(5000);
    REQUIRE( ui->getValue(node1_not_respond_s) == 1 );
}
#endif
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check sender", "[unetudp][udp][sender]")
{
    InitTest();

    SECTION("Test: read default pack..")
    {
        UniSetUDP::UDPMessage p = receive();
        REQUIRE( p.header.num != 0 );
        REQUIRE( p.asize() == 4 );
        REQUIRE( p.dsize() == 2 );

        for( size_t i = 0; i < p.asize(); i++ )
        {
            REQUIRE( p.a_dat[i].val == i + 1 );
        }

        REQUIRE( p.dValue(0) == true );
        REQUIRE( p.dValue(1) == false );

        // т.к. данные в SM не менялись, то должен придти пакет с тем же номером что и был.
        UniSetUDP::UDPMessage p2 = receive();
        REQUIRE( p2.header.num == p.header.num );
    }

    SECTION("Test: change AI data..")
    {
        UniSetUDP::UDPMessage pack0 = receive();
        ui->setValue(2, 100);
        REQUIRE( ui->getValue(2) == 100 );
        msleep(120);
        UniSetUDP::UDPMessage pack = receive( pack0.header.num + 1 );
        REQUIRE( pack.header.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );
        REQUIRE( pack.a_dat[0].val == 100 );

        ui->setValue(2, 250);
        REQUIRE( ui->getValue(2) == 250 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = receive( pack.header.num + 1 );
        REQUIRE( pack2.header.num != 0 );
        REQUIRE( pack2.header.num > pack.header.num );
        REQUIRE( pack2.asize() == 4 );
        REQUIRE( pack2.dsize() == 2 );
        REQUIRE( pack2.a_dat[0].val == 250 );
    }

    SECTION("Test: change DI data..")
    {
        UniSetUDP::UDPMessage pack0 = receive();
        ui->setValue(6, 1);
        REQUIRE( ui->getValue(6) == 1 );
        msleep(120);
        UniSetUDP::UDPMessage pack = receive( pack0.header.num + 1 );
        REQUIRE( pack.header.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );
        REQUIRE( pack.dValue(0) == true );

        ui->setValue(6, 0);
        REQUIRE( ui->getValue(6) == 0 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = receive( pack.header.num + 1 );
        REQUIRE( pack2.header.num != 0 );
        REQUIRE( pack2.header.num > pack.header.num );
        REQUIRE( pack2.asize() == 4 );
        REQUIRE( pack2.dsize() == 2 );
        REQUIRE( pack2.dValue(0) == false );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check receiver", "[unetudp][udp][receiver]")
{
    InitTest();

    SECTION("Test: send data pack..")
    {
        REQUIRE( ui->getValue(node2_respond_s) == 0 );

        UniSetUDP::UDPMessage pack;
        pack.addAData(8, 100);
        pack.addAData(9, -100);
        pack.addDData(10, true);
        pack.addDData(11, false);

        REQUIRE( ui->getValue(8) == 0 );
        REQUIRE( ui->getValue(9) == 0 );
        REQUIRE( ui->getValue(10) == 0 );
        REQUIRE( ui->getValue(11) == 0 );

        send(pack);
        msleep(600);
        REQUIRE( ui->getValue(8) == 100 );
        REQUIRE( ui->getValue(9) == -100 );
        REQUIRE( ui->getValue(10) == 1 );
        REQUIRE( ui->getValue(11) == 0 );

        WARN("check respond sensor DISABLED!");
        //        msleep(1500);
        //        REQUIRE( ui->getValue(node2_respond_s) == 1 );
    }
    SECTION("Test: send data pack2.")
    {
        UniSetUDP::UDPMessage pack;
        pack.addAData(8, 10);
        pack.addAData(9, -10);
        pack.addDData(10, false);
        pack.addDData(11, true);
        send(pack);
        msleep(600);
        REQUIRE( ui->getValue(8) == 10 );
        REQUIRE( ui->getValue(9) == -10 );
        REQUIRE( ui->getValue(10) == 0 );
        REQUIRE( ui->getValue(11) == 1 );
        WARN("check respond sensor DISABLED!");
        //REQUIRE( ui->getValue(node2_respond_s) == 1 );
        //msleep(2000); // в запускающем файле стоит --unet-recv-timeout 2000
        //REQUIRE( ui->getValue(node2_respond_s) == 0 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check packets 'hole'", "[unetudp][udp][udphole]")
{
    InitTest();

    // проверяем обработку "дырок" в пакетах.
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 15);
    send(pack);
    msleep(120);
    REQUIRE( ui->getValue(8) == 15 );

    unsigned long nlost = ui->getValue(node2_lostpackets_as);

    int lastnum = s_numpack - 1;

    // искусственно делаем дырку в два пакета
    s_numpack = lastnum + 3;
    UniSetUDP::UDPMessage pack_hole;
    pack_hole.addAData(8, 30);
    send(pack_hole); // пакет с дыркой

    msleep(80);
    REQUIRE( ui->getValue(8) == 15 );
    REQUIRE( ui->getValue(node2_lostpackets_as) == nlost );

    s_numpack = lastnum + 1;
    UniSetUDP::UDPMessage pack1;
    pack1.addAData(8, 21);
    send(pack1); // заполняем первую дырку.// дырка закроется. пакет тут же обработается
    msleep(100);
    REQUIRE( ui->getValue(8) == 21 );
    REQUIRE( ui->getValue(node2_lostpackets_as) == nlost );

    s_numpack = lastnum + 2;
    UniSetUDP::UDPMessage pack2;
    pack2.addAData(8, 25);
    send(pack2); // заполняем следующую дырку
    msleep(120);

    // тут обработка дойдёт уже до "первого" пакета.
    REQUIRE( ui->getValue(8) == 30 );
    REQUIRE( ui->getValue(node2_lostpackets_as) == nlost );

    // возвращаем к нормальному.чтобы следующие тесты не поломались.
    for( int i = 0; i < 10; i++ )
    {
        send(pack2);
        msleep(100);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check packets 'MaxDifferens'", "[unetudp][udp][maxdifferens]")
{
    InitTest();
    // проверяем обработку "дырок" в пакетах.
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 50);
    send(pack);

    msleep(1000);
    REQUIRE( ui->getValue(8) == 50 );

    unsigned long nlost = ui->getValue(node2_lostpackets_as);

    int need_num = s_numpack;
    // искуственно делаем дырку в два пакета
    s_numpack += maxDifferense + 1;

    UniSetUDP::UDPMessage pack_hole;
    pack_hole.addAData(8, 150);
    send(pack_hole); // пакет с дыркой > maxDifference (должен обработаться)

    msleep(120);
    REQUIRE( ui->getValue(8) == 150 );
    msleep(2000);
    REQUIRE( ui->getValue(node2_lostpackets_as) > nlost );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: bad packet number", "[unetudp][udp][badnumber]")
{
    InitTest();

    // посылаем нормальный пакет
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 60);
    send(pack);
    msleep(150);
    REQUIRE( ui->getValue(8) == 60 );

    int lastpack = s_numpack - 1;

    // посылаем пакет с тем же номером
    s_numpack = lastpack;
    UniSetUDP::UDPMessage pack1;
    pack1.addAData(8, 150);
    send(pack1); // должен быть "откинут"

    msleep(120);
    REQUIRE( ui->getValue(8) == 60 );

    // посылаем пакет с меньшим номером
    s_numpack = lastpack - 2;
    UniSetUDP::UDPMessage pack2;
    pack2.addAData(8, 155);
    send(pack2); // должен быть "откинут"

    msleep(120);
    REQUIRE( ui->getValue(8) == 60 );

    // посылаем нормальный
    s_numpack = lastpack + 1;
    UniSetUDP::UDPMessage pack3;
    pack3.addAData(8, 160);
    send(pack3); // должен быть "обработан"

    msleep(120);
    REQUIRE( ui->getValue(8) == 160 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: switching channels", "[unetudp][udp][chswitch]")
{
    InitTest();
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 70);
    send(pack);
    msleep(120);
    REQUIRE( ui->getValue(8) == 70 );

    // и счётчик переключений каналов в нуле
    REQUIRE( ui->getValue(node1_channelSwitchCount_as) == 0 );

    // К сожалению в текущей реализации тестов
    // обмена по второму каналу нет
    // поэтому проверить переключение нет возможности
    // остаётся только проверить, что мы не "ушли" с первого канала
    // т.к. на втором нет связи и мы не должны на него переключаться
    msleep(recvTimeout * 3);
    REQUIRE( ui->getValue(node1_numchannel_as) == 1 );

    // и счётчик переключений каналов остался в нуле
    REQUIRE( ui->getValue(node1_channelSwitchCount_as) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check undefined value", "[unetudp][udp][sender]")
{
    InitTest();

    UniSetUDP::UDPMessage pack0 = receive();

    ui->setValue(2, 110);
    REQUIRE( ui->getValue(2) == 110 );
    msleep(600);

    UniSetUDP::UDPMessage pack = receive( pack0.header.num + 1, 2000, 40 );

    REQUIRE( pack.header.num != 0 );
    REQUIRE( pack.asize() == 4 );
    REQUIRE( pack.dsize() == 2 );
    REQUIRE( pack.a_dat[0].val == 110 );

    IOController_i::SensorInfo si;
    si.id = 2;
    si.node = uniset_conf()->getLocalNode();
    ui->setUndefinedState(si, true, 6000 /* TestProc */ );
    msleep(600);
    pack = receive(pack.header.num + 1);

    REQUIRE( pack.header.num != 0 );
    REQUIRE( pack.asize() == 4 );
    REQUIRE( pack.dsize() == 2 );
    REQUIRE( pack.a_dat[0].val == 65635 );

    ui->setUndefinedState(si, false, 6000 /* TestProc */ );
    msleep(600);
    pack = receive(pack.header.num + 1);

    REQUIRE( pack.header.num != 0 );
    REQUIRE( pack.asize() == 4 );
    REQUIRE( pack.dsize() == 2 );
    REQUIRE( pack.a_dat[0].val == 110 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: perf test", "[unetudp][zero][perf]")
{
    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.header.nodeID = 100;
    pack.header.procID = 100;
    pack.header.num = 1;

    for( size_t i = 0; i < uniset::UniSetUDP::MaxACount; i++ )
    {
        pack.addAData(i, i);
        pack.addDData(i, true);
    }

    UniSetUDP::UDPMessage pack2;

    PassiveTimer pt;

    for( int i = 0; i < 100000; i++ )
    {
        memcpy(&pack2, &pack, sizeof(UniSetUDP::UDPMessage));
        pack2.ntoh();
    }

    cerr << "perf: " << pt.getCurrent() << " msec" << endl;
}
