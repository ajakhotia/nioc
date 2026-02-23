@0xce5af619be087419;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::common");

using Timestamp = import "timestamp.capnp".Timestamp;


struct Header @0xc4820e31dabe8a6a
{
    timestamp @0 : Timestamp;
    identity  @1 : Text;
}
