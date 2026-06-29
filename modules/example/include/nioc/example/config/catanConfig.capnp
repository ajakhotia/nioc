@0xdca7a00b00000010;

using Cxx = import "/capnp/c++.capnp";
using MinerConfig = import "minerConfig.capnp".MinerConfig;
using RoadBuilderConfig = import "roadBuilderConfig.capnp".RoadBuilderConfig;
using SettlementBuilderConfig = import "settlementBuilderConfig.capnp".SettlementBuilderConfig;
using CityBuilderConfig = import "cityBuilderConfig.capnp".CityBuilderConfig;
using DevelopmentCardBuilderConfig = import "developmentCardBuilderConfig.capnp".DevelopmentCardBuilderConfig;

$Cxx.namespace("nioc::example");

# Root config: one block per routine. Each block's defaults are filled in here; every routine reads
# only its own block. Mining times are in milliseconds.
struct CatanConfig
{
    hills @0 : MinerConfig = (driver = (name = "hills"), resourceTopic = "brick", miningTimeMs = 2000);
    forest @1 : MinerConfig = (driver = (name = "forest"), resourceTopic = "lumber", miningTimeMs = 2000);
    pasture @2 : MinerConfig = (driver = (name = "pasture"), resourceTopic = "wool", miningTimeMs = 2000);
    fields @3 : MinerConfig = (driver = (name = "fields"), resourceTopic = "grain", miningTimeMs = 1000);
    mountains @4 : MinerConfig = (driver = (name = "mountains"), resourceTopic = "ore", miningTimeMs = 1000);

    roadBuilder @5 : RoadBuilderConfig;
    settlementBuilder @6 : SettlementBuilderConfig;
    cityBuilder @7 : CityBuilderConfig;
    developmentCardBuilder @8 : DevelopmentCardBuilderConfig;
}
