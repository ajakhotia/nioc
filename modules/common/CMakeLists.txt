find_package(CapnProto REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS headers)

capnproto_generate_library(
        TARGET
            commonIdl
        NAMESPACE
            nioc::
        EXPORT
            niocTargets
        SCHEMA_FILES
            include/nioc/common/idl/timestamp.capnp
            include/nioc/common/idl/header.capnp
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
            common
        TYPE
            INTERFACE
        NAMESPACE
            nioc::
        EXPORT
            niocTargets
        SOURCES
            ""
        HEADERS
            INTERFACE include/nioc/common/cacheManager.hpp
            INTERFACE include/nioc/common/locked.hpp
            INTERFACE include/nioc/common/typeTraits.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            INTERFACE Boost::headers
        COMPILE_FEATURES
            INTERFACE cxx_std_20
        COMPILE_OPTIONS
            ""
        COMPILE_DEFINITIONS
            "")

if(CLANG_TIDY)
    set_target_properties(common PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
