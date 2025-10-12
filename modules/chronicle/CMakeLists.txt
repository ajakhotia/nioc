if(NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(Boost CONFIG REQUIRED COMPONENTS headers iostreams)
find_package(spdlog REQUIRED)

capnproto_generate_library(
    TARGET
        chronicleIdl
    NAMESPACE
        nioc::
    EXPORT
        niocTargets
    SCHEMA_FILES
        include/nioc/chronicle/idl/frame.capnp
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
        chronicle
    NAMESPACE
        nioc::
    EXPORT
        niocTargets
    SOURCES
        src/chronicle.cpp
        src/memoryCrate.cpp
        src/mmapChannelReader.cpp
        src/mmapMemoryCrate.cpp
        src/mmapReader.cpp
        src/streamChannelWriter.cpp
        src/streamWriter.cpp
        src/utils.cpp
    HEADERS
        PRIVATE src/mmapChannelReader.hpp
        PRIVATE src/mmapMemoryCrate.hpp
        PRIVATE src/mmapReader.hpp
        PRIVATE src/streamChannelWriter.hpp
        PRIVATE src/streamWriter.hpp
        PRIVATE src/utils.hpp
        PUBLIC include/nioc/chronicle/chronicle.hpp
        PUBLIC include/nioc/chronicle/defines.hpp
        PUBLIC include/nioc/chronicle/memoryCrate.hpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBRARIES
        PRIVATE Boost::headers
        PRIVATE Boost::iostreams
        PRIVATE nioc::common
        PRIVATE nioc::chronicleIdl
        PRIVATE spdlog::spdlog
    COMPILE_FEATURES
        PUBLIC cxx_std_20
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
