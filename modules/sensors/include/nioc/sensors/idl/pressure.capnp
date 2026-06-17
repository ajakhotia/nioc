@0x83b4e21eca41091e;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::sensors");

using Header = import "header.capnp".Header;


struct Pressure @0xeae0b366d6cb1322
{
    header   @0 : Header;
    pressure @1 : Float64;   # absolute pressure, Pascals
    variance @2 : Float64;   # 0 means variance unknown
}
