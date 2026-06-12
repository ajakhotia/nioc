@0xd22b8031813ad275;

using Cxx = import "/capnp/c++.capnp";
using ComponentConfig = import "/nioc/terminus/config/componentConfig.capnp".ComponentConfig;

$Cxx.namespace("nioc::example");

struct ExampleComponent1Config @0xe814f654bb745f93
{
    component @0 : ComponentConfig = (name = "exampleComponent1");
    sample3Topic @1 : Text = "sample3";
    sample2Topic @2 : Text = "sample2";
}
