@0xdca7a00b00000006;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::example");

# A built piece: monotonic id plus the name of the builder that made it.
struct Road
{
    id @0 : UInt64;
    builder @1 : Text;
}
