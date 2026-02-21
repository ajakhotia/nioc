@0x907347fe40a016ff;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::primitives");


struct Vector3 @0x8e0ed3ea29f55494
{
    x @0 : Float64;
    y @1 : Float64;
    z @2 : Float64;
}

struct Quaternion @0xab643615acc4a096
{
    x @0 : Float64;
    y @1 : Float64;
    z @2 : Float64;
    w @3 : Float64;
}
