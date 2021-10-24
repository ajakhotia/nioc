find_package(CapnProto REQUIRED)

capnproto_generate_library(
        TARGET messagesIdl
        NAMESPACE Naksh::
        EXPORT nakshTargets
        SCHEMA_FILES
            include/naksh/messages/idl/sample1.capnp
            include/naksh/messages/idl/sample2.capnp)


add_exported_library(
        TARGET
            messages
        TYPE
            SHARED
        NAMESPACE
            Naksh::
        EXPORT
            nakshTargets
        SRC_FILES
            src/msgBase.cpp
        PUBLIC_INCLUDE_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE_INCLUDE_DIR
            ""
        PUBLIC_LINK_LIBRARIES
            ""
        PRIVATE_LINK_LIBRARIES
            CapnProto::capnp
            CapnProto::kj
        PUBLIC_HEADERS
            include/naksh/messages/msg.hpp
            include/naksh/messages/msgBase.hpp
        PRIVATE_HEADERS
            ""
        COMPILE_FEATURES
            cxx_std_17
        COMPILE_OPTIONS
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)


if(BUILD_TESTING)
    add_subdirectory(test)
endif()
