@0xe92c8f17e25c43ef;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");

# The storage discipline of a Component's inbox. Mirrors nioc::concurrent::BufferMode.
enum BufferMode @0xb39984dfa6e66fc2
{
    overwriting @0;
    unbounded @1;
}

# Settings of the terminus::Component base.
struct ComponentConfig @0xda482b1add5914a9
{
    name @0 : Text;
    inboxCapacity @1 : UInt32 = 16;
    bufferMode @2 : BufferMode = unbounded;
}
