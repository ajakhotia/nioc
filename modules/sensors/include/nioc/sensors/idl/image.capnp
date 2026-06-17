@0xf44e409f492e6f29;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::sensors");

using Header = import "header.capnp".Header;

struct Image @0xc2fb22a0a71cf775
{
    header @0 : Header;
    format @1 : Text;   # codec and pixel format, e.g. "rgb8; jpeg compressed bgr8"
    data   @2 : Data;   # encoded image bytes, stored verbatim (compressed unless format says raw)
}
