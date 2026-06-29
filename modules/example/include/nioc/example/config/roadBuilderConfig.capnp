@0xdca7a00b0000000b;

using Cxx = import "/capnp/c++.capnp";
using ComponentConfig = import "/nioc/terminus/config/componentConfig.capnp".ComponentConfig;

$Cxx.namespace("nioc::example");

# Topics in/out and the recipe sizes for the RoadBuilder.
struct RoadBuilderConfig
{
    component @0 : ComponentConfig = (name = "roadBuilder");
    brickTopic @1 : Text = "brick";
    lumberTopic @2 : Text = "lumber";
    roadTopic @3 : Text = "road";
    brickPerRoad @4 : UInt32 = 1;
    lumberPerRoad @5 : UInt32 = 1;
}
