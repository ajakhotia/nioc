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
    include/nioc/terminus/idl/frame.capnp
    include/nioc/terminus/idl/testSchema.capnp
  LINK_LIBRARIES
    nioc::commonIdl
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    ""
)


add_exported_library(
  TARGET
    terminus
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/component.cpp
    src/msgBase.cpp
    src/port.cpp
    src/programOption.cpp
    src/runner.cpp
    src/threadedRunner.cpp
  HEADERS
    PUBLIC include/nioc/terminus/asyncProcessor.hpp
    PUBLIC include/nioc/terminus/component.hpp
    PUBLIC include/nioc/terminus/driver.hpp
    PUBLIC include/nioc/terminus/msg.hpp
    PUBLIC include/nioc/terminus/msgBase.hpp
    PUBLIC include/nioc/terminus/notifyingInbox.hpp
    PUBLIC include/nioc/terminus/port.hpp
    PUBLIC include/nioc/terminus/programOption.hpp
    PUBLIC include/nioc/terminus/routine.hpp
    PUBLIC include/nioc/terminus/runner.hpp
    PUBLIC include/nioc/terminus/threadedRunner.hpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBRARIES
    PRIVATE Boost::headers
    PRIVATE CapnProto::capnp
    PRIVATE CapnProto::kj
    PRIVATE nioc::common
    PRIVATE nioc::logger
    PUBLIC Boost::program_options
    PUBLIC nioc::chronicle
    PUBLIC nlohmann_json::nlohmann_json
    PUBLIC spdlog::spdlog
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    ""
)

#if(CLANG_TIDY)
#  set_target_properties(terminus PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
#endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
