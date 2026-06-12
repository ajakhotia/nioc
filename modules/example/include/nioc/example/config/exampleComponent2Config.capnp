@0xbea06b02eafdebb0;

using Cxx = import "/capnp/c++.capnp";
using ComponentConfig = import "/nioc/terminus/config/componentConfig.capnp".ComponentConfig;

$Cxx.namespace("nioc::example");

struct ExampleComponent2Config @0xd03eac95d42cf8c4
{
    component @0 : ComponentConfig = (name = "exampleComponent2");
    sample1Topic @1 : Text = "sample1";
    sample2Topic @2 : Text = "sample2";
    sample3Topic @3 : Text = "sample3";
}
