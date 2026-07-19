@0xdca7a00b00000013;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("nioc::example");

# Config for the Mountains driver: the topic it publishes ore on and how long it spends producing
# one.
struct MountainsConfig
{
    resourceTopic @0 : Text = "ore";
    miningTimeMs @1 : UInt32 = 1000;
}
