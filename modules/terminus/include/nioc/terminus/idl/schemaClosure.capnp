@0xae570290a25bea10;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");

using CapnpSchema = import "/capnp/schema.capnp";


struct SchemaClosure @0xa5eea0095ec2fcd3
{
    nodes @0 : List(CapnpSchema.Node);
    # All the schemas a recording uses: the schema of each published message plus every schema
    # they refer to, each stored once. A recording saves this list as `schemas.bin`, and a
    # replay loads it back to decode the recorded messages without having their schemas built
    # in.
}
