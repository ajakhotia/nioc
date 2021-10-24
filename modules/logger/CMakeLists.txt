find_package(Boost REQUIRED COMPONENTS iostreams)
find_package(spdlog REQUIRED)

capnproto_generate_library(
        TARGET
            loggerIdl
        TYPE
            SHARED
        NAMESPACE
            Naksh::
        EXPORT
            nakshTargets
        SCHEMA_FILES
            include/naksh/logger/idl/frame.capnp
        COMPILE_FEATURES
            cxx_std_17
        COMPILE_OPTIONS
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)


add_exported_library(
        TARGET
            logger
        TYPE
            SHARED
        NAMESPACE
            Naksh::
        SRC_FILES
            src/logger.cpp
        PUBLIC_INCLUDE_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE_INCLUDE_DIR
            ""
        PUBLIC_LINK_LIBRARIES
            ""
        PRIVATE_LINK_LIBRARIES
            Naksh::loggerIdl
            spdlog::spdlog
            Boost::iostreams
        PUBLIC_HEADERS
            include/naksh/logger/logger.hpp
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
