@0x8941ccd694862716;

using Cxx = import "/capnp/c++.capnp";
using ExampleDriverConfig = import "exampleDriverConfig.capnp".ExampleDriverConfig;
using ExampleComponent1Config = import "exampleComponent1Config.capnp".ExampleComponent1Config;
using ExampleComponent2Config = import "exampleComponent2Config.capnp".ExampleComponent2Config;

$Cxx.namespace("nioc::example");

# Root of exampleMain's config tree: one block per routine instance.
struct ExampleMainConfig @0x9e8b068b16f4a520
{
    exampleDriver @0 : ExampleDriverConfig;
    exampleComponent1 @1 : ExampleComponent1Config;
    exampleComponent2 @2 : ExampleComponent2Config;
}
