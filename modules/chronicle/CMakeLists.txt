if(NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(Boost CONFIG REQUIRED COMPONENTS headers)

add_exported_library(
    TARGET
        chronicle
    NAMESPACE
        nioc::
    EXPORT
        niocTargets
    SOURCES
        src/channel.cpp
        src/crate.cpp
        src/defines.cpp
        src/reader.cpp
        src/utils.cpp
        src/writer.cpp
    HEADERS
        PRIVATE src/timeline.hpp
        PRIVATE src/utils.hpp
        PUBLIC include/nioc/chronicle/channel.hpp
        PUBLIC include/nioc/chronicle/crate.hpp
        PUBLIC include/nioc/chronicle/defines.hpp
        PUBLIC include/nioc/chronicle/reader.hpp
        PUBLIC include/nioc/chronicle/reservation.hpp
        PUBLIC include/nioc/chronicle/writer.hpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBRARIES
        PRIVATE Boost::headers
        PRIVATE nioc::logger
        PUBLIC nioc::common
        PUBLIC nioc::containers
    COMPILE_FEATURES
        PUBLIC cxx_std_23
    COMPILE_OPTIONS
        PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    COMPILE_DEFINITIONS
        "")

if(CLANG_TIDY)
    set_target_properties(chronicle PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
