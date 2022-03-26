if(NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(Boost REQUIRED COMPONENTS headers iostreams)
find_package(spdlog REQUIRED)

capnproto_generate_library(
    TARGET
        loggerIdl
    NAMESPACE
        Naksh::
    EXPORT
        nakshTargets
    SCHEMA_FILES
        include/naksh/logger/idl/frame.capnp
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
        logger
    NAMESPACE
        Naksh::
    EXPORT
        nakshTargets
    SOURCES
        src/channel.cpp
        src/logger.cpp
        src/utils.cpp
    HEADERS
        PUBLIC include/naksh/logger/channel.hpp
        PUBLIC include/naksh/logger/logger.hpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBRARIES
        PRIVATE Boost::headers
        PRIVATE Boost::iostreams
        PRIVATE Naksh::loggerIdl
        PRIVATE spdlog::spdlog
        PUBLIC Naksh::common
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
