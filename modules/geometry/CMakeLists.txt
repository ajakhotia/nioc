find_package(Boost REQUIRED)

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
            src/frameId.cpp
        HEADERS
            PUBLIC
                include/naksh/geometry/frameId.hpp
                include/naksh/geometry/frameReferences.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PUBLIC
                Naksh::common
                Boost::headers
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
