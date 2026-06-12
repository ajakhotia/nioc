@0x9a0aff86f6178f9f;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::example");

struct Sample3 @0xc80e29df80e6e3da
{
    label @0 : Text;
    flag @1 : Bool;
}
