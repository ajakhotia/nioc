@0xdca7a00b00000012;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("nioc::example");

# Config for the Fields driver: the topic it publishes grain on and how long it spends producing
# one.
struct FieldsConfig
{
    resourceTopic @0 : Text = "grain";
    miningTimeMs @1 : UInt32 = 1000;
}
