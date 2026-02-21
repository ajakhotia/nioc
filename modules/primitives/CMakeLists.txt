find_package(CapnProto REQUIRED)

capnproto_generate_library(
  TARGET
    primitivesIdl
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/primitives/idl/timestamp.capnp
    include/nioc/primitives/idl/geometry.capnp
    include/nioc/primitives/idl/header.capnp
  COMPILE_FEATURES
    PUBLIC cxx_std_20
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")
