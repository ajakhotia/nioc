add_exported_library(
    TARGET
        containers
    NAMESPACE
        nioc::
    EXPORT
        niocTargets
    SOURCES
        src/mmapRegion.cpp
    HEADERS
        PUBLIC include/nioc/containers/mmapRegion.hpp
        PUBLIC include/nioc/containers/mmapArray.hpp
        PUBLIC include/nioc/containers/mmapConstArray.hpp
        PUBLIC include/nioc/containers/tape.hpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBRARIES
        PUBLIC nioc::logger
        PUBLIC nioc::common
    COMPILE_FEATURES
        PUBLIC cxx_std_23
    COMPILE_OPTIONS
        PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    COMPILE_DEFINITIONS
        "")

if(CLANG_TIDY)
    set_target_properties(containers PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
