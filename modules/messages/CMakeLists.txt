find_package(CapnProto REQUIRED)

capnproto_generate_library(
        TARGET
            messagesIdl
        NAMESPACE
            nioc::
        EXPORT
            niocTargets
        SCHEMA_FILES
            include/naksh/messages/idl/sample1.capnp
            include/naksh/messages/idl/sample2.capnp
        COMPILE_FEATURES
            PUBLIC cxx_std_17
        COMPILE_OPTIONS
            PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        COMPILE_DEFINITIONS
            "")


add_exported_library(
        TARGET
            messages
        NAMESPACE
            nioc::
        EXPORT
            niocTargets
        SOURCES
            src/msgBase.cpp
        HEADERS
            PUBLIC include/naksh/messages/msg.hpp
            PUBLIC include/naksh/messages/msgBase.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PRIVATE CapnProto::capnp
            PRIVATE CapnProto::kj
            PUBLIC nioc::logger
            PUBLIC nioc::loggerIdl
        COMPILE_FEATURES
            PUBLIC cxx_std_17
        COMPILE_OPTIONS
            PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        COMPILE_DEFINITIONS
            "")


if(BUILD_TESTING)
    add_subdirectory(test)
endif()
