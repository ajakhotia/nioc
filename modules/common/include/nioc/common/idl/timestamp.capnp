@0xfddb771c230afa0a;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::common");


struct Timestamp @0xb2cac35f6f565bc8
{
    nanosecondSinceEpoch @0 : Int64;
    reference            @1 : Text;
    # Clock reference e.g. "UTC", "GPS", "system".
}
