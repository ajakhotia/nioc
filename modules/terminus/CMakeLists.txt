find_package(CapnProto REQUIRED)

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
            PUBLIC cxx_std_20
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
        HEADERS
            PUBLIC include/nioc/terminus/msg.hpp
            PUBLIC include/nioc/terminus/msgBase.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PRIVATE CapnProto::capnp
            PRIVATE CapnProto::kj
            PUBLIC nioc::chronicle
            PUBLIC nioc::chronicleIdl
        COMPILE_FEATURES
            PUBLIC cxx_std_20
        COMPILE_OPTIONS
            PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        COMPILE_DEFINITIONS
            "")


if(BUILD_TESTING)
    add_subdirectory(test)
endif()
