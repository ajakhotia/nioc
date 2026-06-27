find_package(CapnProto REQUIRED)

capnproto_generate_library(
  TARGET
    exampleIdl
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/example/idl/sample1.capnp
    include/nioc/example/idl/sample2.capnp
    include/nioc/example/idl/sample3.capnp
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")


capnproto_generate_library(
  TARGET
    exampleConfig
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/example/config/exampleComponent1Config.capnp
    include/nioc/example/config/exampleComponent2Config.capnp
    include/nioc/example/config/exampleDriverConfig.capnp
    include/nioc/example/config/exampleMainConfig.capnp
  LINK_LIBRARIES
    nioc::terminusConfig
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
    example
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/exampleComponent1.cpp
    src/exampleComponent2.cpp
    src/exampleDriver.cpp
  HEADERS
    PUBLIC include/nioc/example/exampleComponent1.hpp
    PUBLIC include/nioc/example/exampleComponent2.hpp
    PUBLIC include/nioc/example/exampleDriver.hpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBRARIES
    PUBLIC nioc::terminus
    PUBLIC nioc::exampleConfig
    PUBLIC nioc::exampleIdl
    PRIVATE nioc::logger
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")


add_exported_executable(
  TARGET
    exampleMain
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/exampleMain.cpp
  LINK_LIBRARIES
    PRIVATE nioc::concurrent
    PRIVATE nioc::example
    PRIVATE nioc::terminus
  COMPILE_FEATURES
    PRIVATE cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")


if(CLANG_TIDY)
  set_target_properties(example PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
  set_target_properties(exampleMain PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()


install(DIRECTORY config DESTINATION .)
