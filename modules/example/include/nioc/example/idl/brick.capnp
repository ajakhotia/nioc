@0xdca7a00b00000001;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::example");

# A resource message: monotonic id plus the name of the driver that produced it.
struct Brick
{
    id @0 : UInt64;
    producer @1 : Text;
}
