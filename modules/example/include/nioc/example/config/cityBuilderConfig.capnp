@0xdca7a00b0000000d;

using Cxx = import "/capnp/c++.capnp";
using ComponentConfig = import "/nioc/terminus/config/componentConfig.capnp".ComponentConfig;

$Cxx.namespace("nioc::example");

# Topics in/out and the recipe sizes for the CityBuilder.
struct CityBuilderConfig
{
    component @0 : ComponentConfig = (name = "cityBuilder");
    settlementTopic @1 : Text = "settlement";
    oreTopic @2 : Text = "ore";
    grainTopic @3 : Text = "grain";
    cityTopic @4 : Text = "city";
    settlementPerCity @5 : UInt32 = 1;
    orePerCity @6 : UInt32 = 3;
    grainPerCity @7 : UInt32 = 2;
}
