@0xdca7a00b0000000a;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("nioc::example");

# Config shared by every resource miner: the topic it publishes on and how long it spends
# producing one resource.
struct MinerConfig
{
    resourceTopic @0 : Text = "resource";
    miningTimeMs @1 : UInt32 = 2000;
}
