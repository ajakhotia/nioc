@0xdca7a00b0000000a;

using Cxx = import "/capnp/c++.capnp";
using DriverConfig = import "/nioc/terminus/config/driverConfig.capnp".DriverConfig;

$Cxx.namespace("nioc::example");

# Config shared by every resource miner: its name, the topic it publishes on, and how long it
# spends producing one resource.
struct MinerConfig
{
    driver @0 : DriverConfig = (name = "miner");
    resourceTopic @1 : Text = "resource";
    miningTimeMs @2 : UInt32 = 2000;
}
