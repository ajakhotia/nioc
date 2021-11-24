add_exported_library(
        TARGET
            geometry
        TYPE
            SHARED
        NAMESPACE
            Naksh::
        EXPORT
            nakshTargets
        SOURCES
            src/frame.cpp
        HEADERS
            PUBLIC
                include/naksh/geometry/frame.hpp
                include/naksh/geometry/frameConcepts.hpp
                include/naksh/geometry/frameReferences.hpp
                include/naksh/geometry/compose.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PUBLIC Naksh::common
        COMPILE_FEATURES
            PUBLIC cxx_std_17
        COMPILE_OPTIONS
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
                $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        COMPILE_DEFINITIONS
            "")

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
