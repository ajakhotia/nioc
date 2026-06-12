@0xd9009165a14bfd98;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");

# Settings of the terminus::Driver base. A driver's config block carries these under its "driver"
# subsection. The name is instance-specific and must come from the config data: a schema default
# cannot tell two instances of one class apart.
struct DriverConfig @0xedd5b219bcaa0886
{
    name @0 : Text;
}
