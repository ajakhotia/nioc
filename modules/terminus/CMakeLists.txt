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
            include/nioc/terminus/idl/sample1.capnp
            include/nioc/terminus/idl/sample2.capnp
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
            src/msgBase.cpp
            src/port.cpp
            src/programOption.cpp
        HEADERS
            PUBLIC include/nioc/terminus/msg.hpp
            PUBLIC include/nioc/terminus/msgBase.hpp
            PUBLIC include/nioc/terminus/port.hpp
            PUBLIC include/nioc/terminus/programOption.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PRIVATE CapnProto::capnp
            PRIVATE CapnProto::kj
            PRIVATE Boost::headers
            PRIVATE nioc::common
            PRIVATE nioc::logger
            PRIVATE spdlog::spdlog
            PUBLIC Boost::program_options
            PUBLIC nioc::chronicle
            PUBLIC nioc::chronicleIdl
            PUBLIC nlohmann_json::nlohmann_json
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
