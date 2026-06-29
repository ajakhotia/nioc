@0xdca7a00b00000005;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::example");

# A resource message: monotonic id plus the name of the driver that produced it.
struct Ore
{
    id @0 : UInt64;
    producer @1 : Text;
}
