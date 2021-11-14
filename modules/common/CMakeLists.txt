find_package(Boost REQUIRED COMPONENTS headers)

add_exported_library(
        TARGET
            common
        TYPE
            INTERFACE
        NAMESPACE
            Naksh::
        EXPORT
            nakshTargets
        SOURCES
            ""
        HEADERS
            INTERFACE
                include/naksh/common/locked.hpp
        include/naksh/common/typeTraits.hpp

        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            INTERFACE Boost::headers
        COMPILE_FEATURES
            INTERFACE
                cxx_std_17
        COMPILE_OPTIONS
            ""
        COMPILE_DEFINITIONS
            "")


if(BUILD_TESTING)
    add_subdirectory(test)
endif()
