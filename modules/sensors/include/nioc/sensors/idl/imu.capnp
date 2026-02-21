@0xc9995cf6cd2f5e81;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::sensors");

using Header     = import "/nioc/primitives/idl/header.capnp".Header;
using Vector3    = import "/nioc/primitives/idl/geometry.capnp".Vector3;
using Quaternion = import "/nioc/primitives/idl/geometry.capnp".Quaternion;


struct Imu @0xa32f91dd180cb931
{
    header                       @0 : Header;
    orientation                  @1 : Quaternion;
    orientationCovariance        @2 : List(Float64);   # 9 elements, row-major
    angularVelocity              @3 : Vector3;
    angularVelocityCovariance    @4 : List(Float64);   # 9 elements, row-major
    linearAcceleration           @5 : Vector3;
    linearAccelerationCovariance @6 : List(Float64);   # 9 elements, row-major
}
