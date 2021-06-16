@0xedf94979a37df74f;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("naksh::messages");

struct Sample @0xc319225d305dbe45
{
    name @0 : Text;
    value @1 : Int64;
}
