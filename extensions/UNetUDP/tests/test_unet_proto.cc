#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UDPPacket.h"
// -----------------------------------------------------------------------------
#include "proto/unet.pb.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: protobuf UNetPacket", "[unetudp][protobuf][packet]")
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    unet::UNetPacket pack;
    pack.set_magic(0x13463C4);
    REQUIRE(pack.magic() == 0x13463C4 );

    pack.set_nodeid(100);
    pack.set_procid(100);
    pack.set_num(1);

    for (int i = 0; i < 1000; i++ )
    {
        pack.mutable_data()->add_aid(1);
        pack.mutable_data()->add_avalue(i);
    }

    REQUIRE( pack.data().aid_size() == 1000 );

    //    cerr << pack.ByteSizeLong() << endl;

    //    uniset::UniSetUDP::UDPMessage p1;
    //    p1.procID = 100;
    //    p1.nodeID = 100;
    //    p1.num = 1;
    //    for (int i=0; i< 1000; i++ ) {
    //        p1.addDData(i,1);
    //    }

    //    cerr << sizeof(p1) << endl;

    std::string s = pack.SerializeAsString();
    unet::UNetPacket pack2;
    pack2.ParseFromArray(s.data(), s.size());
    REQUIRE(pack2.magic() == pack.magic() );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: protobuf UDPMessage", "[unetudp][protobuf][message]")
{
    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.setNodeID(99);
    pack.setProcID(100);
    pack.setNum(1);

    for( int i = 0; i < 1000; i++ )
        pack.addAData(i, i);

    REQUIRE( pack.asize() == 1000 );
    REQUIRE(pack.isOk());

    const std::string s = pack.getDataAsString();

    UniSetUDP::UDPMessage pack2;
    pack2.initFromBuffer((uint8_t*)s.data(), s.size());
    REQUIRE(pack2.isOk());
    REQUIRE(pack2.asize() == 1000);
    REQUIRE(pack2.aValue(1) == 1);
    REQUIRE(pack2.procID() == 100);
    REQUIRE(pack2.nodeID() == 99);
}


// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: crc", "[unetudp][protobuf][crc]")
{
    uint8_t buf[uniset::UniSetUDP::MessageBufSize];

    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.setNodeID(100);
    pack.setProcID(100);
    pack.setNum(1);

    uint16_t crc = pack.dataCRC();
    uint16_t crc_b = pack.dataCRCWithBuf(buf, sizeof(buf));
    REQUIRE( crc == crc_b );

    UniSetUDP::UDPMessage pack2; // the same data
    REQUIRE(pack2.isOk());
    pack2.setNodeID(100);
    pack2.setProcID(100);
    pack2.setNum(1);
    uint16_t crc2 = pack2.dataCRC();
    uint16_t crc2_b = pack.dataCRCWithBuf(buf, sizeof(buf));
    REQUIRE( crc2 == crc2_b );
    REQUIRE( crc == crc2 );

    //
    auto d = pack.addDData(1, 1);
    auto a = pack.addAData(2, 100);
    crc = pack.dataCRC();

    pack.setAData(a, 0);
    REQUIRE( pack.dataCRC() != crc );
    REQUIRE( pack.dataCRCWithBuf(buf, sizeof(buf)) != crc );
    crc = pack.dataCRC();
    crc_b = pack.dataCRCWithBuf(buf, sizeof(buf));

    pack.setDData(d, 0);
    REQUIRE( pack.dataCRC() != crc );
    REQUIRE( pack.dataCRCWithBuf(buf, sizeof(buf)) != crc_b );
}
