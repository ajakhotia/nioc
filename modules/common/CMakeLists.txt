add_exported_library(
        TARGET
            common
        TYPE
            SHARED
        NAMESPACE
            Naksh::
        EXPORT
            nakshTargets
        SRC_FILES
            src/common.cpp
        PUBLIC_INCLUDE_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE_INCLUDE_DIR
            ""
        PUBLIC_LINK_LIBRARIES
            ""
        PRIVATE_LINK_LIBRARIES
            ""
        PUBLIC_HEADERS
            include/naksh/common/locked.hpp
        PRIVATE_HEADERS
            ""
        COMPILE_FEATURES
            cxx_std_17
        COMPILE_OPTIONS
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)


if(BUILD_TESTING)
    add_subdirectory(test)
endif()
