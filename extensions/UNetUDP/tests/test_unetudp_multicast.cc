#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "UDPPacket.h"
#include "UDPCore.h"
#include "MulticastTransport.h"
// -----------------------------------------------------------------------------
// include-ы искплючительно для того, чтобы их обработал gcov (покрытие кода)
#include "UNetReceiver.h"
#include "UNetSender.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui = nullptr;
static ObjectId aid = 2;
static std::unique_ptr<MulticastReceiveTransport> udp_r = nullptr;
static std::unique_ptr<MulticastSendTransport> udp_s = nullptr;
static int s_port = 3003; // Node2
static int s_nodeID = 3003;
static int s_procID = 123;
static int s_numpack = 1;
static ObjectId node2_respond_s = 12;
static ObjectId node2_lostpackets_as = 13;
static int maxDifferense = 5; // см. unetudp-test-configure.xml --unet-maxdifferense
static int recvTimeout = 1000; // --unet-recv-timeout
static ObjectId node1_numchannel_as = 14;
static ObjectId node1_channelSwitchCount_as = 15;
static ObjectId localnode_sendMode_S = 18;
static ObjectId node1_recvMode_S = 16;
// -----------------------------------------------------------------------------
static void initHelpers()
{
    if( udp_r && udp_s )
        return;

    UniXML xml("unetudp-test-configure.xml");
    UniXML::iterator it = xml.findNode(xml.getFirstNode(), "nodes");
    REQUIRE( it.getCurrent() );
    REQUIRE( it.goChildren() );
    REQUIRE( it.findName("item", "localhost", false) );

    REQUIRE( it.getName() == "item" );
    REQUIRE( it.getProp("name") == "localhost" );


    if( !udp_r )
    {
        std::vector<Poco::Net::IPAddress> groups;
        groups.emplace_back("224.0.0.1");
        udp_r = make_unique<MulticastReceiveTransport>("224.0.0.1", 3000, groups, "");
        REQUIRE( udp_r->toString() == "224.0.0.1:3000" );
        REQUIRE( udp_r->createConnection(false, 5000, true) );
        // pause for igmp message
        msleep(3000);
    }

    if( !udp_s )
    {
        udp_s = make_unique<MulticastSendTransport>("127.0.0.1", 3002, "224.0.0.2", 3002);
        REQUIRE( udp_s->toString() == "127.0.0.1:3002" );
        REQUIRE( udp_s->createConnection(false, 5000) );
        // pause for igmp message
        msleep(3000);
    }
}
// -----------------------------------------------------------------------------
void InitMulticastTest()
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
        initHelpers();

    if( udp_s == nullptr )
        initHelpers();

    REQUIRE_NOTHROW( ui->setValue(localnode_sendMode_S, (long) UNetSender::Mode::mEnabled) );
    REQUIRE_NOTHROW( ui->setValue(node1_recvMode_S, (long) UNetReceiver::Mode::mEnabled) );
}
// -----------------------------------------------------------------------------
// pnum - минималный номер ожидаемого пакета ( 0 - любой пришедщий )
// ncycle - сколько пакетов разрешено "пропустить" прежде чем дождёмся нужного.. (чтобы не ждать бесконечно)
static UniSetUDP::UDPMessage mreceive( unsigned int pnum = 0, timeout_t tout = 2000, int ncycle = 20 )
{
    UniSetUDP::UDPMessage pack;

    while( ncycle > 0 )
    {
        if( !udp_r->isReadyForReceive(tout) )
            break;

        size_t ret = udp_r->receive(&pack, sizeof(pack) );

        if( ret <= 0 )
            break;

        pack.ntoh();

        if( pnum > 0 && pack.header.num >= pnum ) // -V560
            break;

        REQUIRE( pack.header.magic == UniSetUDP::UNETUDP_MAGICNUM );
        ncycle--;
    }

    //  if( pnum > 0 && pack.num < pnum )
    //      return UniSetUDP::UDPMessage(); // empty message

    return pack;
}
// -----------------------------------------------------------------------------
void msend( UniSetUDP::UDPMessage& pack, int tout = 2000 )
{
    CHECK( udp_s->isReadyForSend(tout) );

    pack.header.nodeID = s_nodeID;
    pack.header.procID = s_procID;
    pack.header.num = s_numpack++;
    pack.updatePacketCrc();

    size_t ret = udp_s->send(&pack, sizeof(pack));
    REQUIRE( ret == sizeof(pack) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check multicast sender", "[unetudp][multicast][sender]")
{
    InitMulticastTest();

    SECTION("Test: read default pack...")
    {
        UniSetUDP::UDPMessage pack = mreceive();
        REQUIRE( pack.header.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );

        for( size_t i = 0; i < pack.asize(); i++ )
        {
            REQUIRE( pack.a_dat[i].val == i + 1 );
        }

        REQUIRE( pack.dValue(0) == 1 );
        REQUIRE( pack.dValue(1) == 0 );

        // т.к. данные в SM не менялись, то должен придти пакет с теми же crc
        UniSetUDP::UDPMessage pack2 = mreceive();
        REQUIRE( pack2.header.dcrc == pack.header.dcrc );
        REQUIRE( pack2.header.acrc == pack.header.acrc );
    }

    SECTION("Test: change AI data...")
    {
        UniSetUDP::UDPMessage pack0 = mreceive();
        ui->setValue(2, 100);
        REQUIRE( ui->getValue(2) == 100 );
        msleep(120);
        UniSetUDP::UDPMessage pack = mreceive( pack0.header.num + 1 );
        REQUIRE( pack.header.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );
        REQUIRE( pack.a_dat[0].val == 100 );

        ui->setValue(2, 250);
        REQUIRE( ui->getValue(2) == 250 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = mreceive( pack.header.num + 1 );
        REQUIRE( pack2.header.num != 0 );
        REQUIRE( pack2.header.num > pack.header.num );
        REQUIRE( pack2.asize() == 4 );
        REQUIRE( pack2.dsize() == 2 );
        REQUIRE( pack2.a_dat[0].val == 250 );
    }

    SECTION("Test: change DI data...")
    {
        UniSetUDP::UDPMessage pack0 = mreceive();
        ui->setValue(6, 1);
        REQUIRE( ui->getValue(6) == 1 );
        msleep(120);
        UniSetUDP::UDPMessage pack = mreceive( pack0.header.num + 1 );
        REQUIRE( pack.header.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );
        REQUIRE( pack.dValue(0) == 1 );

        ui->setValue(6, 0);
        REQUIRE( ui->getValue(6) == 0 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = mreceive( pack.header.num + 1 );
        REQUIRE( pack2.header.num != 0 );
        REQUIRE( pack2.header.num > pack.header.num );
        REQUIRE( pack2.asize() == 4 );
        REQUIRE( pack2.dsize() == 2 );
        REQUIRE( pack2.dValue(0) == 0 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check multicast receiver", "[unetudp][multicast][receiver]")
{
    InitMulticastTest();

    SECTION("Test: send data pack...")
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

        msend(pack);
        msleep(600);
        REQUIRE( ui->getValue(8) == 100 );
        REQUIRE( ui->getValue(9) == -100 );
        REQUIRE( ui->getValue(10) == 1 );
        REQUIRE( ui->getValue(11) == 0 );
    }
    SECTION("Test: send data pack2..")
    {
        UniSetUDP::UDPMessage pack;
        pack.addAData(8, 10);
        pack.addAData(9, -10);
        pack.addDData(10, false);
        pack.addDData(11, true);
        msend(pack);
        msleep(900);
        REQUIRE( ui->getValue(8) == 10 );
        REQUIRE( ui->getValue(9) == -10 );
        REQUIRE( ui->getValue(10) == 0 );
        REQUIRE( ui->getValue(11) == 1 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check send mode", "[unetudp][multicast][sendmode]")
{
    InitMulticastTest();

    ui->setValue(localnode_sendMode_S, (long)UNetSender::Mode::mEnabled);

    size_t lastPackNum = 0;
    SECTION("Test: read default pack..")
    {
        UniSetUDP::UDPMessage p = mreceive(0, 500, 2);
        REQUIRE( p.header.num != 0 );
        REQUIRE( p.asize() == 4 );

        // т.к. данные в SM не менялись, то должен прийти пакет с теми же crc что и были
        UniSetUDP::UDPMessage p2 = mreceive();
        REQUIRE( p2.header.dcrc == p.header.dcrc );
        REQUIRE( p2.header.acrc == p.header.acrc );
        lastPackNum = p2.header.num;
    }

    SECTION("Test: change AI data..")
    {
        UniSetUDP::UDPMessage pack0 = mreceive(lastPackNum + 1);
        ui->setValue(2, 100);
        REQUIRE( ui->getValue(2) == 100 );
        msleep(120);
        UniSetUDP::UDPMessage pack = mreceive( pack0.header.num + 1 );
        REQUIRE( pack.a_dat[0].val == 100 );

        ui->setValue(2, 250);
        REQUIRE( ui->getValue(2) == 250 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = mreceive( pack.header.num + 1 );
        REQUIRE( pack2.a_dat[0].val == 250 );

        lastPackNum = pack2.header.num;
    }

    SECTION("Test: disable exchange..")
    {
        UniSetUDP::UDPMessage pack0 = mreceive(lastPackNum + 1);
        REQUIRE( pack0.a_dat[0].val == 250 );
        ui->setValue(localnode_sendMode_S, (long)UNetSender::Mode::mDisabled);
        msleep(120);
        // read all messages
        UniSetUDP::UDPMessage pack1 = mreceive(pack0.header.num + 1);
        // update data
        ui->setValue(2, 100);
        REQUIRE( ui->getValue(2) == 100 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = mreceive();
        REQUIRE( pack1.header.num == 0 ); // no changes
        msleep(120);
        UniSetUDP::UDPMessage pack3 = mreceive( pack2.header.num + 1 );
        REQUIRE( pack2.header.num == 0 ); // no changes
    }

    SECTION("Test: enable exchange..")
    {
        ui->setValue(2, 100);
        REQUIRE( ui->getValue(2) == 100 );
        ui->setValue(localnode_sendMode_S, (long)UNetSender::Mode::mEnabled);
        msleep(120);
        UniSetUDP::UDPMessage pack1 = mreceive();
        REQUIRE( pack1.a_dat[0].val == 100 );

        ui->setValue(2, 150);
        REQUIRE( ui->getValue(2) == 150 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = mreceive( pack1.header.num + 1 );
        REQUIRE( pack2.a_dat[0].val == 150 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check recv mode", "[unetudp][multicast][recvmode]")
{
    InitMulticastTest();

    ui->setValue(node1_recvMode_S, (long) UNetReceiver::Mode::mEnabled);

    SECTION("Test: send data pack..")
    {
        REQUIRE( ui->getValue(8) != 99 );
        UniSetUDP::UDPMessage pack;
        pack.addAData(8, 99);
        msend(pack);
        msleep(300);
        REQUIRE( ui->getValue(8) == 99 );
    }

    SECTION("Test: disable receive from node2..")
    {
        ui->setValue(node1_recvMode_S, (long) UNetReceiver::Mode::mDisabled);
        msleep(200);
        UniSetUDP::UDPMessage pack;
        pack.addAData(8, 98);
        REQUIRE( ui->getValue(8) != 98 );
        msend(pack);
        msleep(120);
        REQUIRE( ui->getValue(8) != 98 );
        // again
        msend(pack);
        msleep(120);
        REQUIRE( ui->getValue(8) != 98 );
    }

    SECTION("Test: enable receive from node2..")
    {
        ui->setValue(node1_recvMode_S, (long) UNetReceiver::Mode::mEnabled);
        msleep(200);
        UniSetUDP::UDPMessage pack;
        pack.addAData(8, 97);
        REQUIRE( ui->getValue(8) != 97 );
        msend(pack);
        msend(pack);
        msleep(200);
        REQUIRE( ui->getValue(8) == 97 );
        // again
        UniSetUDP::UDPMessage pack2;
        pack2.addAData(8, 96);
        msend(pack2);
        msleep(120);
        REQUIRE( ui->getValue(8) == 96 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check multicast packets 'hole'", "[unetudp][multicast][udphole]")
{
    InitMulticastTest();

    // проверяем обработку "дырок" в пакетах..
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 15);
    msend(pack);
    msleep(120);
    REQUIRE( ui->getValue(8) == 15 );

    unsigned long nlost = ui->getValue(node2_lostpackets_as);

    int lastnum = s_numpack - 1;

    // искусственно делаем дырку в два пакета
    s_numpack = lastnum + 3;
    UniSetUDP::UDPMessage pack_hole;
    pack_hole.addAData(8, 30);
    msend(pack_hole); // пакет с дыркой

    msleep(80);
    REQUIRE( ui->getValue(8) == 15 );
    //    REQUIRE( ui->getValue(node2_lostpackets_as) == nlost );

    s_numpack = lastnum + 1;
    UniSetUDP::UDPMessage pack1;
    pack1.addAData(8, 21);
    msend(pack1); // заполняем первую дырку..// дырка закроется.. пакет тут же обработается
    msleep(100);
    REQUIRE( ui->getValue(8) == 21 );
    //    REQUIRE( ui->getValue(node2_lostpackets_as) == nlost );

    s_numpack = lastnum + 2;
    UniSetUDP::UDPMessage pack2;
    pack2.addAData(8, 25);
    msend(pack2); // заполняем следующую дырку
    msleep(120);

    // тут обработка дойдёт уже до "первого" пакета..
    REQUIRE( ui->getValue(8) == 30 );
    //    REQUIRE( ui->getValue(node2_lostpackets_as) == nlost );

    // возвращаем к нормальному..чтобы следующие тесты не поломались..
    for( int i = 0; i < 10; i++ )
    {
        msend(pack2);
        msleep(100);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check multicast packets 'MaxDifferens'", "[unetudp][multicast][maxdifferens]")
{
    InitMulticastTest();

    // проверяем обработку "дырок" в пакетах..
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 50);
    msend(pack);

    msleep(1000);
    REQUIRE( ui->getValue(8) == 50 );

    unsigned long nlost = ui->getValue(node2_lostpackets_as);

    int need_num = s_numpack;
    // искуственно делаем дырку в два пакета
    s_numpack += maxDifferense + 1;

    UniSetUDP::UDPMessage pack_hole;
    pack_hole.addAData(8, 150);
    msend(pack_hole); // пакет с дыркой > maxDifference (должен обработаться)

    msleep(120);
    REQUIRE( ui->getValue(8) == 150 );
    //    msleep(2000);
    //    REQUIRE( ui->getValue(node2_lostpackets_as) > nlost );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: multicast bad packet number", "[unetudp][multicast][badnumber]")
{
    InitMulticastTest();

    // посылаем нормальный пакет
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 60);
    msend(pack);
    msleep(150);
    REQUIRE( ui->getValue(8) == 60 );

    int lastpack = s_numpack - 1;

    // посылаем пакет с тем же номером
    s_numpack = lastpack;
    UniSetUDP::UDPMessage pack1;
    pack1.addAData(8, 150);
    msend(pack1); // должен быть "откинут"

    msleep(120);
    REQUIRE( ui->getValue(8) == 60 );

    // посылаем пакет с меньшим номером
    s_numpack = lastpack - 2;
    UniSetUDP::UDPMessage pack2;
    pack2.addAData(8, 155);
    msend(pack2); // должен быть "откинут"

    msleep(120);
    REQUIRE( ui->getValue(8) == 60 );

    // посылаем нормальный
    s_numpack = lastpack + 1;
    UniSetUDP::UDPMessage pack3;
    pack3.addAData(8, 160);
    msend(pack3); // должен быть "обработан"

    msleep(120);
    REQUIRE( ui->getValue(8) == 160 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: [multicast] switching channels", "[unetudp][multicast][chswitch]")
{
    InitMulticastTest();
    UniSetUDP::UDPMessage pack;
    pack.addAData(8, 70);
    msend(pack);
    msleep(120);
    REQUIRE( ui->getValue(8) == 70 );

    // и счётчик переключений каналов в нуле
    REQUIRE( ui->getValue(node1_channelSwitchCount_as) == 0 );

    // К сожалению в текущей реализации тестов
    // обмена по второму каналу нет
    // поэтому проверить переключение нет возможности
    // остаётся только проверить, что мы не "ушли" с первого канала
    // т.к. на втором нет связи и мы не должны на него переключаться
    msleep(recvTimeout * 2);
    REQUIRE( ui->getValue(node1_numchannel_as) == 1 );

    // и счётчик переключений каналов остался в нуле
    REQUIRE( ui->getValue(node1_channelSwitchCount_as) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: mulsicat check undefined value", "[unetudp][multicast][undefvalue]")
{
    InitMulticastTest();

    UniSetUDP::UDPMessage pack0 = mreceive();

    ui->setValue(2, 110);
    REQUIRE( ui->getValue(2) == 110 );
    msleep(600);

    UniSetUDP::UDPMessage pack = mreceive( pack0.header.num + 1, 2000, 40 );

    REQUIRE( pack.header.num != 0 );
    REQUIRE( pack.asize() == 4 );
    REQUIRE( pack.dsize() == 2 );
    REQUIRE( pack.a_dat[0].val == 110 );

    IOController_i::SensorInfo si;
    si.id = 2;
    si.node = uniset_conf()->getLocalNode();
    ui->setUndefinedState(si, true, 6000 /* TestProc */ );
    msleep(600);
    pack = mreceive(pack.header.num + 1);

    REQUIRE( pack.header.num != 0 );
    REQUIRE( pack.asize() == 4 );
    REQUIRE( pack.dsize() == 2 );
    REQUIRE( pack.a_dat[0].val == 65635 );

    ui->setUndefinedState(si, false, 6000 /* TestProc */ );
    msleep(600);
    pack = mreceive(pack.header.num + 1);

    REQUIRE( pack.header.num != 0 );
    REQUIRE( pack.asize() == 4 );
    REQUIRE( pack.dsize() == 2 );
    REQUIRE( pack.a_dat[0].val == 110 );
}
// -----------------------------------------------------------------------------
