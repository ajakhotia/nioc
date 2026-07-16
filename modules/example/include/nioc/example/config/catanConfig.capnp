@0xdca7a00b00000010;

using Cxx = import "/capnp/c++.capnp";
using MinerConfig = import "minerConfig.capnp".MinerConfig;
using RoadBuilderConfig = import "roadBuilderConfig.capnp".RoadBuilderConfig;
using SettlementBuilderConfig = import "settlementBuilderConfig.capnp".SettlementBuilderConfig;
using CityBuilderConfig = import "cityBuilderConfig.capnp".CityBuilderConfig;
using DevelopmentCardBuilderConfig = import "developmentCardBuilderConfig.capnp".DevelopmentCardBuilderConfig;

$Cxx.namespace("nioc::example");

struct CatanConfig
{
    hills @0 : MinerConfig = (resourceTopic = "brick", miningTimeMs = 2000);
    forest @1 : MinerConfig = (resourceTopic = "lumber", miningTimeMs = 2000);
    pasture @2 : MinerConfig = (resourceTopic = "wool", miningTimeMs = 2000);
    fields @3 : MinerConfig = (resourceTopic = "grain", miningTimeMs = 1000);
    mountains @4 : MinerConfig = (resourceTopic = "ore", miningTimeMs = 1000);

    roadBuilder @5 : RoadBuilderConfig;
    settlementBuilder @6 : SettlementBuilderConfig;
    cityBuilder @7 : CityBuilderConfig;
    developmentCardBuilder @8 : DevelopmentCardBuilderConfig;
}
