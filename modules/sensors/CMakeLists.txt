find_package(CapnProto REQUIRED)

capnproto_generate_library(
  TARGET
    sensorsIdl
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/sensors/idl/imu.capnp
    include/nioc/sensors/idl/pointCloud.capnp
  LINK_LIBRARIES
    nioc::primitivesIdl
  COMPILE_FEATURES
    PUBLIC cxx_std_20
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")
