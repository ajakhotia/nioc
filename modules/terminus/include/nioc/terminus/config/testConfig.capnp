@0x8be12bea377feced;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");

# Nested structure exercised by configStoreTest.
struct TestLeafConfig @0xfdd0045afa082652
{
    value @0 : Int64 = 3;
    tag @1 : Text = "leaf";
}

# Root schema exercised by configStoreTest: one field per json value category. The leaf carries a
# struct-literal default so the tests cover literals surviving partial overrides.
struct TestConfig @0xa0f210406fb0badf
{
    name @0 : Text;
    count @1 : UInt32 = 7;
    enabled @2 : Bool = true;
    gains @3 : List(Float64);
    leaf @4 : TestLeafConfig = (value = 11, tag = "lit");
}
