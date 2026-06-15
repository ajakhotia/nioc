@0xffa3af6657f6f9d8;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::sensors");

using Header = import "/nioc/common/idl/header.capnp".Header;


struct Gnss @0xbd02583ac98eb501
{
    header             @0 : Header;
    status             @1 : Int8;            # -2 unknown, -1 no fix, 0 fix, 1 SBAS, 2 GBAS
    service            @2 : UInt16;          # bitfield: GPS=1, GLONASS=2, COMPASS=4, GALILEO=8
    latitude           @3 : Float64;         # degrees, positive north
    longitude          @4 : Float64;         # degrees, positive east
    altitude           @5 : Float64;         # metres above the WGS-84 ellipsoid
    positionCovariance @6 : List(Float64);   # 9 elements, ENU, row-major (m^2)
    covarianceType     @7 : UInt8;           # 0 unknown, 1 approximated, 2 diagonal known, 3 known
}
