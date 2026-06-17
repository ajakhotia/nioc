@0xa9dedc35677617d1;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::sensors");

using Header = import "header.capnp".Header;


struct Temperature @0x9094a85f40cf8f38
{
    header      @0 : Header;
    temperature @1 : Float64;   # degrees Celsius
    variance    @2 : Float64;   # 0 means variance unknown
}
