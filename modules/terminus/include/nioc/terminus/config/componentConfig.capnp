@0xe92c8f17e25c43ef;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");

# The storage discipline of a Component's inbox. Mirrors nioc::concurrent::BufferMode.
enum BufferMode @0xb39984dfa6e66fc2
{
    overwriting @0;
    unbounded @1;
}

# Settings of the terminus::Component base. A component's config block carries these under its
# "component" subsection. The name is instance-specific and must come from the config data: a
# schema default cannot tell two instances of one class apart.
struct ComponentConfig @0xda482b1add5914a9
{
    name @0 : Text;

    # UInt32 deliberately: JsonCodec renders 64-bit integers as quoted strings to dodge json's
    # double-precision limit, which would make the recorded config.json read awkwardly.
    inboxCapacity @1 : UInt32 = 16;

    bufferMode @2 : BufferMode = unbounded;
}
