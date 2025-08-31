if(NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(Boost REQUIRED COMPONENTS headers iostreams)
find_package(spdlog REQUIRED)

capnproto_generate_library(
    TARGET
        loggerIdl
    NAMESPACE
        nioc::
    EXPORT
        niocTargets
    SCHEMA_FILES
        include/nioc/logger/idl/frame.capnp
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
        logger
    NAMESPACE
        nioc::
    EXPORT
        niocTargets
    SOURCES
        src/channel.cpp
        src/channelReader.cpp
        src/channelReader.cpp
        src/logReader.cpp
        src/logReaderImpl.cpp
        src/logger.cpp
        src/loggerImpl.cpp
        src/memoryCrate.cpp
        src/memoryCrateImpl.cpp
        src/utils.cpp
    HEADERS
        PRIVATE src/channel.hpp
        PRIVATE src/channelReader.hpp
        PRIVATE src/channelReader.hpp
        PRIVATE src/logReaderImpl.hpp
        PRIVATE src/loggerImpl.hpp
        PRIVATE src/memoryCrateImpl.hpp
        PRIVATE src/utils.hpp
        PUBLIC include/nioc/logger/defines.hpp
        PUBLIC include/nioc/logger/logReader.hpp
        PUBLIC include/nioc/logger/logger.hpp
        PUBLIC include/nioc/logger/memoryCrate.hpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBRARIES
        PRIVATE Boost::headers
        PRIVATE Boost::iostreams
        PRIVATE nioc::common
        PRIVATE nioc::loggerIdl
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
    set_target_properties(logger PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
