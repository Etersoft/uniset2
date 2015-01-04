#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <cc++/socket.h>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "UDPPacket.h"
// -----------------------------------------------------------------------------
// include-ы искплючительно для того, чтобы их обработал gcov (покрытие кода)
#include "UNetReceiver.h"
#include "UNetSender.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static int port = 3000;
static ost::IPV4Host host("127.255.255.255");
static UInterface* ui = nullptr;
static ObjectId aid = 2;
static ost::UDPDuplex* udp_r = nullptr;
static ost::UDPBroadcast* udp_s = nullptr;
static int s_port = 3003; // Node2
static int s_nodeID = 3003;
static int s_procID = 123;
static int s_numpack = 1;
static ObjectId node2_respond_s = 12;
// -----------------------------------------------------------------------------
void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf!=nullptr );

    if( ui == nullptr )
    {
        ui = new UInterface();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(aid,10000) );
    }

    if( udp_r == nullptr )
        udp_r = new ost::UDPDuplex(host,port);

    if( udp_s == nullptr )
        udp_s = new ost::UDPBroadcast(host,s_port);
}
// -----------------------------------------------------------------------------
// pnum - минималный номер ожидаемого пакета ( 0 - любой пришедщий )
// ncycle - сколько пакетов разрещено "пропустить" прежде чем дождёмся нужного.. (чтобы не ждать бесконечно)
static UniSetUDP::UDPMessage receive( unsigned int pnum = 0, timeout_t tout = 2000, int ncycle = 10 )
{
    UniSetUDP::UDPMessage pack;
    UniSetUDP::UDPPacket buf;

    while( ncycle > 0 )
    {
        if( !udp_r->isInputReady(tout) )
            break;

        size_t ret = udp_r->UDPReceive::receive( &(buf.data), sizeof(buf.data) );
        size_t sz = UniSetUDP::UDPMessage::getMessage(pack,buf);
        if( sz == 0 || pnum == 0 || ( pnum > 0 && pack.num >= pnum ) )
            break;

        REQUIRE( pack.magic == UniSetUDP::UNETUDP_MAGICNUM );
        ncycle--;
    }

    return std::move(pack);
}
// -----------------------------------------------------------------------------
void send( UniSetUDP::UDPMessage& pack, int tout=2000 )
{
	CHECK( udp_s->isPending(ost::Socket::pendingOutput,tout) );

    pack.nodeID = s_nodeID;
    pack.procID = s_procID;
	pack.num = s_numpack++;

	UniSetUDP::UDPPacket s_buf;
    pack.transport_msg(s_buf);
	size_t ret = udp_s->send((char*)&s_buf.data, s_buf.len);
    REQUIRE( ret == s_buf.len );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: UDPMessage","[unetudp][udpmessage]")
{
    SECTION("UDPMessage::isFull()")
    {
        UniSetUDP::UDPMessage u;
        for( unsigned int i=0; i<UniSetUDP::MaxACount-1; i++ )
            u.addAData( i, i );

        REQUIRE( u.asize() == (UniSetUDP::MaxACount-1) );

        CHECK_FALSE( u.isAFull() );
        u.addAData( 1, 1 );
        CHECK( u.isAFull() );

        for( unsigned int i=0; i<UniSetUDP::MaxDCount-1; i++ )
            u.addDData( i, true );

        REQUIRE( u.dsize() == (UniSetUDP::MaxDCount-1) );

        CHECK_FALSE( u.isDFull() );
        u.addDData( 1, true );
        CHECK( u.isDFull() );

        CHECK( u.isFull() );
    }

    SECTION("UDPMessage transport..")
    {
        // создаём сообщение, преобразуем к Package.. потом обратно.. проверяём, что информация не исказилась
        UniSetUDP::UDPMessage u;
        int a = u.addAData(100,100);
        int d = u.addDData(110,true);

        UniSetUDP::UDPPacket p;
        size_t len = u.transport_msg(p);
        CHECK( len != 0 );

        UniSetUDP::UDPMessage u2(p);
        REQUIRE( u2.a_dat[a].id == 100 );
        REQUIRE( u2.a_dat[a].val == 100 );
         REQUIRE( u2.dID(d) == 110 );
        REQUIRE( u2.dValue(d) == true );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: respond sensor","[unetudp]")
{
    InitTest();

    // в запускающем файле стоит --unet-recv-timeout 2000
    msleep(2500);
    ObjectId node1_not_respond_s = 1;
    REQUIRE( ui->getValue(node1_not_respond_s) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check sender","[unetudp][sender]")
{
    InitTest();

    SECTION("Test: read default pack...")
    {
        UniSetUDP::UDPMessage pack = receive();
        REQUIRE( pack.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );

        for( int i=0; i<pack.asize(); i++ )
        {
            REQUIRE( pack.a_dat[i].val == i+1 );
        }

        REQUIRE( pack.dValue(0) == 1 );
        REQUIRE( pack.dValue(1) == 0 );

        // т.к. данные в SM не менялись, то должен придти пакет с тем же номером что и был..
        UniSetUDP::UDPMessage pack2 = receive();
        REQUIRE( pack2.num == pack.num );
    }

    SECTION("Test: change AI data...")
    {
        UniSetUDP::UDPMessage pack0 = receive();
        ui->setValue(2,100);
        REQUIRE( ui->getValue(2) == 100 );
        msleep(120);
        UniSetUDP::UDPMessage pack = receive( pack0.num+1 );
        REQUIRE( pack.num != 0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );
        REQUIRE( pack.a_dat[0].val == 100 );

        ui->setValue(2,250);
        REQUIRE( ui->getValue(2) == 250 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = receive( pack.num+1 );
        REQUIRE( pack2.num != 0 );
        REQUIRE( pack2.num > pack.num );
        REQUIRE( pack2.asize() == 4 );
        REQUIRE( pack2.dsize() == 2 );
        REQUIRE( pack2.a_dat[0].val == 250 );
    }

    SECTION("Test: change DI data...")
    {
        UniSetUDP::UDPMessage pack0 = receive();
        ui->setValue(6,1);
        REQUIRE( ui->getValue(6) == 1 );
        msleep(120);
        UniSetUDP::UDPMessage pack = receive( pack0.num+1 );
        REQUIRE( pack.num!=0 );
        REQUIRE( pack.asize() == 4 );
        REQUIRE( pack.dsize() == 2 );
        REQUIRE( pack.dValue(0) == 1 );

        ui->setValue(6,0);
        REQUIRE( ui->getValue(6) == 0 );
        msleep(120);
        UniSetUDP::UDPMessage pack2 = receive( pack.num+1 );
        REQUIRE( pack2.num != 0 );
        REQUIRE( pack2.num > pack.num );
        REQUIRE( pack2.asize() == 4 );
        REQUIRE( pack2.dsize() == 2 );
        REQUIRE( pack2.dValue(0) == 0 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check receiver","[unetudp][receiver]")
{
    InitTest();

    SECTION("Test: send data pack...")
    {
		REQUIRE( ui->getValue(node2_respond_s) == 0 );

		UniSetUDP::UDPMessage pack;
		pack.addAData(8,100);
		pack.addAData(9,-100);
		pack.addDData(10,true);
		pack.addDData(11,false);

		REQUIRE( ui->getValue(8) == 0 );
		REQUIRE( ui->getValue(9) == 0 );
		REQUIRE( ui->getValue(10) == 0 );
		REQUIRE( ui->getValue(11) == 0 );

		send(pack);
		msleep(120);
		REQUIRE( ui->getValue(8) == 100 );
		REQUIRE( ui->getValue(9) == -100 );
		REQUIRE( ui->getValue(10) == 1 );
		REQUIRE( ui->getValue(11) == 0 );

		msleep(1000);
		REQUIRE( ui->getValue(node2_respond_s) == 1 );
	}
	SECTION("Test: send data pack2..")
    {
		UniSetUDP::UDPMessage pack;
		pack.addAData(8,10);
		pack.addAData(9,-10);
		pack.addDData(10,false);
		pack.addDData(11,true);
		send(pack);
		msleep(120);
		REQUIRE( ui->getValue(8) == 10 );
		REQUIRE( ui->getValue(9) == -10 );
		REQUIRE( ui->getValue(10) == 0 );
		REQUIRE( ui->getValue(11) == 1 );
		REQUIRE( ui->getValue(node2_respond_s) == 1 );
		msleep(3000); // в запускающем файле стоит --unet-recv-timeout 2000
		REQUIRE( ui->getValue(node2_respond_s) == 0 );
	}
}
// -----------------------------------------------------------------------------
