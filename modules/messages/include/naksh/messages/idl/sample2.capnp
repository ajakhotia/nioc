@0xc48e8c1ef6bb045c;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("naksh::messages");

struct Sample2 @0xb5d6c21f4954bf78
{
    idNumber @0 : UInt32;
    rank @1 : UInt32;
    name @2 : Text;
}
