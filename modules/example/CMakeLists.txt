find_package(CapnProto REQUIRED)

capnproto_generate_library(
  TARGET
    catanMessages
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/example/idl/brick.capnp
    include/nioc/example/idl/lumber.capnp
    include/nioc/example/idl/wool.capnp
    include/nioc/example/idl/grain.capnp
    include/nioc/example/idl/ore.capnp
    include/nioc/example/idl/road.capnp
    include/nioc/example/idl/settlement.capnp
    include/nioc/example/idl/city.capnp
    include/nioc/example/idl/developmentCard.capnp
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
    catanConfig
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/example/config/minerConfig.capnp
    include/nioc/example/config/roadBuilderConfig.capnp
    include/nioc/example/config/settlementBuilderConfig.capnp
    include/nioc/example/config/cityBuilderConfig.capnp
    include/nioc/example/config/developmentCardBuilderConfig.capnp
    include/nioc/example/config/catanConfig.capnp
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
    catan
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/roadBuilder.cpp
    src/settlementBuilder.cpp
    src/cityBuilder.cpp
    src/developmentCardBuilder.cpp
  HEADERS
    PUBLIC include/nioc/example/hills.hpp
    PUBLIC include/nioc/example/forest.hpp
    PUBLIC include/nioc/example/pasture.hpp
    PUBLIC include/nioc/example/fields.hpp
    PUBLIC include/nioc/example/mountains.hpp
    PUBLIC include/nioc/example/roadBuilder.hpp
    PUBLIC include/nioc/example/settlementBuilder.hpp
    PUBLIC include/nioc/example/cityBuilder.hpp
    PUBLIC include/nioc/example/developmentCardBuilder.hpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBRARIES
    PUBLIC nioc::terminus
    PUBLIC nioc::catanConfig
    PUBLIC nioc::catanMessages
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
    catanMain
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/catanMain.cpp
  LINK_LIBRARIES
    PRIVATE nioc::concurrent
    PRIVATE nioc::catan
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
  set_target_properties(catan PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
  set_target_properties(catanMain PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()


install(DIRECTORY config DESTINATION .)
