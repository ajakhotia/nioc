@0xdca7a00b00000011;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("nioc::example");

# Config for the Pasture driver: the topic it publishes wool on and how long it spends producing
# one.
struct PastureConfig
{
    resourceTopic @0 : Text = "wool";
    miningTimeMs @1 : UInt32 = 2000;
}
