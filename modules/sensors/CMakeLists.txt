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


add_exported_library(
  TARGET
    sensors
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/time.cpp
  HEADERS
    PUBLIC include/nioc/sensors/time.hpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBRARIES
    PUBLIC nioc::sensorsIdl
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")


if(CLANG_TIDY)
  set_target_properties(sensors PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()
