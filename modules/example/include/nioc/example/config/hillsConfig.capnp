@0xdca7a00b0000000f;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("nioc::example");

# Config for the Hills driver: the topic it publishes brick on and how long it spends producing one.
struct HillsConfig
{
    resourceTopic @0 : Text = "brick";
    miningTimeMs @1 : UInt32 = 2000;
}
