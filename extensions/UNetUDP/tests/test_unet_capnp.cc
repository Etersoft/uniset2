#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include <iostream>
#include "UniSetTypes.h"
#include "UDPPacket.h"
#include "PassiveTimer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: capnp UDPMessage", "[unetudp][capnp][message]")
{
    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.setNodeID(99);
    pack.setProcID(100);
    pack.setNum(1);

    for( int i = 0; i < UniSetUDP::MaxACount; i++ )
        pack.addAData(i, i);

    for( int i = 0; i < UniSetUDP::MaxDCount; i++ )
        pack.addDData(i, true);


    REQUIRE(pack.asize() == UniSetUDP::MaxACount);
    REQUIRE(pack.dsize() == UniSetUDP::MaxDCount);
    REQUIRE(pack.isOk());
    REQUIRE(pack.procID() == 100);
    REQUIRE(pack.nodeID() == 99);
    REQUIRE(pack.aValue(1) == 1);

    uint8_t buf[UniSetUDP::MessageBufSize];
    size_t ret = pack.serializeToArray(buf, sizeof(buf));
    REQUIRE( ret <= UniSetUDP::MessageBufSize );

    UniSetUDP::UDPMessage pack2;
    REQUIRE(pack2.initFromBuffer(buf, ret));
    REQUIRE(pack2.isOk());
    REQUIRE(pack2.procID() == 100);
    REQUIRE(pack2.nodeID() == 99);
    REQUIRE(pack2.asize() == UniSetUDP::MaxACount);
    REQUIRE(pack2.dsize() == UniSetUDP::MaxDCount);
    REQUIRE(pack2.aValue(1) == 1);
    REQUIRE(pack2.aValue(999) == 999);
    REQUIRE(pack2.dID(1) == 1);
    REQUIRE(pack2.dID(999) == 999);
}

// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: crc", "[unetudp][capnp][crc]")
{
    uint8_t buf[uniset::UniSetUDP::MessageBufSize];

    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.setNodeID(100);
    pack.setProcID(100);
    pack.setNum(1);

    uint16_t crc = pack.dataCRC();

    UniSetUDP::UDPMessage pack2; // the same data
    REQUIRE(pack2.isOk());
    pack2.setNodeID(100);
    pack2.setProcID(100);
    pack2.setNum(1);
    uint16_t crc2 = pack2.dataCRC();
    REQUIRE( crc == crc2 );

    //
    auto d = pack.addDData(1, 1);
    auto a = pack.addAData(2, 100);
    crc = pack.dataCRC();

    pack.setAData(a, 0);
    REQUIRE( pack.dataCRC() != crc );
    crc = pack.dataCRC();

    pack.setDData(d, 0);
    REQUIRE( pack.dataCRC() != crc );
}

// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: perf test", "[unetudp][capnp][perf]")
{
    uint8_t buf[uniset::UniSetUDP::MessageBufSize];

    UniSetUDP::UDPMessage pack;
    REQUIRE(pack.isOk());
    pack.setNodeID(100);
    pack.setProcID(100);
    pack.setNum(1);

    for( size_t i = 0; i < uniset::UniSetUDP::MaxACount; i++ )
    {
        pack.addAData(i, i);
        pack.addDData(i, true);
    }

    UniSetUDP::UDPMessage pack2;

    PassiveTimer pt;

    for( int i = 0; i < 100000; i++ )
    {
        // pack
        //       uint16_t crc = pasck.dataCRC();
        auto ret = pack.serializeToArray(buf, sizeof(buf));

        // unpack
        pack2.initFromBuffer(buf, ret);
    }

    cerr << "perf: " << pt.getCurrent() << " msec" << endl;
}
