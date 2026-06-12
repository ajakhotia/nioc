@0x9d8a681128cc0f60;

using Cxx = import "/capnp/c++.capnp";
using DriverConfig = import "/nioc/terminus/config/driverConfig.capnp".DriverConfig;

$Cxx.namespace("nioc::example");

struct ExampleDriverConfig @0xd8b4f927d9ea4140
{
    driver @0 : DriverConfig = (name = "exampleDriver");
    sample1Topic @1 : Text = "sample1";
    sample3Topic @2 : Text = "sample3";
}
