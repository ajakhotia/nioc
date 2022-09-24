find_package(Eigen3 REQUIRED)

add_exported_library(
        TARGET
            geometry
        NAMESPACE
            nioc::
        EXPORT
            niocTargets
        SOURCES
            src/frame.cpp
            src/frameReferences.cpp
        HEADERS
            PUBLIC include/nioc/geometry/constants.hpp
            PUBLIC include/nioc/geometry/frame.hpp
            PUBLIC include/nioc/geometry/frameConcepts.hpp
            PUBLIC include/nioc/geometry/frameReferences.hpp
            PUBLIC include/nioc/geometry/pose.hpp
            PUBLIC include/nioc/geometry/rigidTransform.hpp
            PUBLIC include/nioc/geometry/rotation.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            INTERFACE Eigen3::Eigen
            PUBLIC nioc::common
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
