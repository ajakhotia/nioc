find_package(CapnProto REQUIRED)

capnproto_generate_library(
  TARGET
    sensorsIdl
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/sensors/idl/gnss.capnp
    include/nioc/sensors/idl/header.capnp
    include/nioc/sensors/idl/image.capnp
    include/nioc/sensors/idl/imu.capnp
    include/nioc/sensors/idl/pointCloud.capnp
    include/nioc/sensors/idl/pressure.capnp
    include/nioc/sensors/idl/temperature.capnp
    include/nioc/sensors/idl/timestamp.capnp
  LINK_LIBRARIES
    nioc::geometryIdl
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")
