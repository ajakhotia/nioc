@0xdca7a00b0000000c;

using Cxx = import "/capnp/c++.capnp";
using ComponentConfig = import "/nioc/terminus/config/componentConfig.capnp".ComponentConfig;

$Cxx.namespace("nioc::example");

# Topics in/out and the recipe sizes for the SettlementBuilder.
struct SettlementBuilderConfig
{
    component @0 : ComponentConfig = (name = "settlementBuilder");
    roadTopic @1 : Text = "road";
    brickTopic @2 : Text = "brick";
    lumberTopic @3 : Text = "lumber";
    woolTopic @4 : Text = "wool";
    grainTopic @5 : Text = "grain";
    settlementTopic @6 : Text = "settlement";
    roadPerSettlement @7 : UInt32 = 2;
    brickPerSettlement @8 : UInt32 = 1;
    lumberPerSettlement @9 : UInt32 = 1;
    woolPerSettlement @10 : UInt32 = 1;
    grainPerSettlement @11 : UInt32 = 1;
}
