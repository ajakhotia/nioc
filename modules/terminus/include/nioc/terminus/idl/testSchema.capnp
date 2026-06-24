@0xd0aee78e71b9724d;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");

struct TestSchema @0x9bdd32ecb9bd982f
{
    value @0 : Int64;
    text @1 : Text;
    numbers @2 : List(Int64);
}
