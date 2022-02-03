find_package(Boost REQUIRED COMPONENTS iostreams)
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
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
                $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
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
            src/logger.cpp
        HEADERS
            PUBLIC
                include/naksh/logger/logger.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PRIVATE
                Naksh::loggerIdl
                spdlog::spdlog
                Boost::iostreams
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
