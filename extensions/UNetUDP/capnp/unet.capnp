@0xba68c0f5b757e0c5;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("uniset");

struct UNetPacket {
  magic @0 :UInt32;
  num @1 :UInt64;
  nodeID @2 :Int64;
  procID @3 :Int64;
  ddata @4 :List(DData);
  dnum @5 :UInt64;
  adata @6 :List(AData);
  anum @7 :UInt64;

  struct DData {
    id @0 :Int64;
    value @1 :Bool;
  }

  struct AData {
    id @0 :Int64;
    value @1 :Int64;
  }
}
