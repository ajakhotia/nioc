@0xdca7a00b00000010;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("nioc::example");

# Config for the Forest driver: the topic it publishes lumber on and how long it spends producing
# one.
struct ForestConfig
{
    resourceTopic @0 : Text = "lumber";
    miningTimeMs @1 : UInt32 = 2000;
}
