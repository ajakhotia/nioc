@0xd2d85103c61626da;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::sensors");

using Header = import "/nioc/common/idl/header.capnp".Header;


enum FieldDatatype @0xedcb6938e7342764
{
    int8    @0;
    uint8   @1;
    int16   @2;
    uint16  @3;
    int32   @4;
    uint32  @5;
    float32 @6;
    float64 @7;
}

struct PointField @0xb6402a82e8147690
{
    name     @0 : Text;
    offset   @1 : UInt32;
    datatype @2 : FieldDatatype;
    count    @3 : UInt32;
}

struct PointCloud @0xe3a7aa44209753fa
{
    header      @0 : Header;
    height      @1 : UInt32;           # 1 for unorganized clouds
    width       @2 : UInt32;           # points per row
    fields      @3 : List(PointField);
    isBigEndian @4 : Bool;
    pointStep   @5 : UInt32;           # bytes per point
    rowStep     @6 : UInt32;           # bytes per row
    data        @7 : Data;             # raw binary point data
    isDense     @8 : Bool;
}
