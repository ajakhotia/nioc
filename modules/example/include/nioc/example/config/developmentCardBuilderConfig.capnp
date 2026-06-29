@0xdca7a00b0000000e;

using Cxx = import "/capnp/c++.capnp";
using ComponentConfig = import "/nioc/terminus/config/componentConfig.capnp".ComponentConfig;

$Cxx.namespace("nioc::example");

# Topics in/out and the recipe sizes for the DevelopmentCardBuilder.
struct DevelopmentCardBuilderConfig
{
    component @0 : ComponentConfig = (name = "developmentCardBuilder");
    oreTopic @1 : Text = "ore";
    woolTopic @2 : Text = "wool";
    grainTopic @3 : Text = "grain";
    developmentCardTopic @4 : Text = "developmentCard";
    orePerCard @5 : UInt32 = 1;
    woolPerCard @6 : UInt32 = 1;
    grainPerCard @7 : UInt32 = 1;
}
