@0xdca7a00b00000004;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::example");

# A resource message: monotonic id plus the name of the driver that produced it.
struct Grain
{
    id @0 : UInt64;
    producer @1 : Text;
}
