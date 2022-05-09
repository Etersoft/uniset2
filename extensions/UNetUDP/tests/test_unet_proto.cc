#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UDPPacket.h"
#include "PassiveTimer.h"
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

    for( size_t i = 0; i < UniSetUDP::MaxACount; i++ )
    {
        pack.mutable_data()->add_aid(1);
        pack.mutable_data()->add_avalue(i);
    }

    REQUIRE( pack.data().aid_size() == UniSetUDP::MaxACount );

    for( size_t i = 0; i < UniSetUDP::MaxDCount; i++ )
    {
        pack.mutable_data()->add_did(1);
        pack.mutable_data()->add_dvalue(i);
    }

    REQUIRE( pack.data().did_size() == UniSetUDP::MaxDCount );

    const std::string s = pack.SerializeAsString();
    unet::UNetPacket pack2;
    pack2.ParseFromArray(s.data(), s.size());
    REQUIRE(pack2.magic() == pack.magic() );
    REQUIRE( pack2.data().aid_size() == UniSetUDP::MaxACount );
    REQUIRE( pack2.data().did_size() == UniSetUDP::MaxDCount );
    REQUIRE( pack2.data().avalue(100) == 100 );

    cerr << "UniSetUDP::UDPMessage size: "
         << s.size()
         << " [MaxACount=" << UniSetUDP::MaxACount
         << " MaxDCount=" << UniSetUDP::MaxDCount
         << "]"
         << endl;

    REQUIRE( s.size() < 65507 ); // UDP packet
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

    const std::string s = pack.serializeAsString();

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

    UniSetUDP::UDPMessage pack2; // the same data
    REQUIRE(pack2.isOk());
    pack2.setNodeID(100);
    pack2.setProcID(100);
    pack2.setNum(1);
    auto changes = pack.dataChanges();

    auto d = pack.addDData(1, 1);
    auto a = pack.addAData(2, 100);
    REQUIRE( pack.dataChanges() != changes );
    changes = pack.dataChanges();

    pack.setAData(a, 0);
    REQUIRE( pack.dataChanges() != changes );
    changes = pack.dataChanges();

    pack.setDData(d, 0);
    REQUIRE( pack.dataChanges() != changes );
}

// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: perf test", "[unetudp][protobuf][perf]")
{
    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.setNodeID(100);
    pack.setProcID(100);
    pack.setNum(1);

    for( size_t i = 0; i < uniset::UniSetUDP::MaxACount; i++ )
        pack.addAData(i, i);

    for( size_t i = 0; i < uniset::UniSetUDP::MaxDCount; i++ )
        pack.addDData(i, true);

    REQUIRE( pack.isOk() );

    UniSetUDP::UDPMessage pack2;

    PassiveTimer pt;
    uint8_t buf[uniset::UniSetUDP::MessageBufSize];

    // pack
    for( int i = 0; i < 100000; i++ )
    {
        auto r = pack.serializeToArray(buf, sizeof(buf));
    }
    cerr << "send perf[100k msg]: " << pt.getCurrent() << " msec" << endl;

    auto ret = pack.serializeToArray(buf, sizeof(buf));
    pt.reset();
    for( int i = 0; i < 100000; i++ )
    {
        // unpack
        pack2.initFromBuffer(buf, ret);
    }
    cerr << "recv perf[100k msg]: " << pt.getCurrent() << " msec" << endl;
}
