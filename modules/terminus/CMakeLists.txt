find_package(Boost CONFIG REQUIRED COMPONENTS headers program_options)
find_package(CapnProto REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

capnproto_generate_library(
  TARGET
    terminusIdl
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/terminus/idl/envelope.capnp
    include/nioc/terminus/idl/testSchema.capnp
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
    terminusConfig
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SCHEMA_FILES
    include/nioc/terminus/config/componentConfig.capnp
    include/nioc/terminus/config/driverConfig.capnp
    include/nioc/terminus/config/testConfig.capnp
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
    terminus
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/asyncChronicleWriter.cpp
    src/component.cpp
    src/configStore.cpp
    src/defaultSignalCatcher.cpp
    src/driver.cpp
    src/manifest.cpp
    src/msgBase.cpp
    src/port.cpp
    src/programOption.cpp
    src/runContext.cpp
  HEADERS
    PUBLIC include/nioc/terminus/asyncChronicleWriter.hpp
    PUBLIC include/nioc/terminus/component.hpp
    PUBLIC include/nioc/terminus/configStore.hpp
    PUBLIC include/nioc/terminus/consignment.hpp
    PUBLIC include/nioc/terminus/defaultSignalCatcher.hpp
    PUBLIC include/nioc/terminus/driver.hpp
    PUBLIC include/nioc/terminus/manifest.hpp
    PUBLIC include/nioc/terminus/msg.hpp
    PUBLIC include/nioc/terminus/msgBase.hpp
    PUBLIC include/nioc/terminus/port.hpp
    PUBLIC include/nioc/terminus/programOption.hpp
    PUBLIC include/nioc/terminus/runContext.hpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBRARIES
    PRIVATE Boost::headers
    PRIVATE CapnProto::capnp-json
    PRIVATE CapnProto::kj
    PRIVATE nlohmann_json::nlohmann_json
    PUBLIC Boost::program_options
    PUBLIC CapnProto::capnp
    PUBLIC nioc::chronicle
    PUBLIC nioc::common
    PUBLIC nioc::concurrent
    PUBLIC nioc::logger
    PUBLIC nioc::terminusConfig
    PUBLIC nioc::terminusIdl
    PUBLIC spdlog::spdlog
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")

if(CLANG_TIDY)
  set_target_properties(terminus PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
