find_package(Boost CONFIG REQUIRED COMPONENTS headers)

add_exported_library(
        TARGET
            concurrent
        NAMESPACE
            nioc::
        EXPORT
            niocTargets
        SOURCES
            src/routine.cpp
            src/runner.cpp
            src/threadedRunner.cpp
        HEADERS
            PUBLIC include/nioc/concurrent/anyMpsc.hpp
            PUBLIC include/nioc/concurrent/asyncProcessor.hpp
            PUBLIC include/nioc/concurrent/droppingMpsc.hpp
            PUBLIC include/nioc/concurrent/mpscQueue.hpp
            PUBLIC include/nioc/concurrent/notifyingInbox.hpp
            PUBLIC include/nioc/concurrent/overwritingMpsc.hpp
            PUBLIC include/nioc/concurrent/routine.hpp
            PUBLIC include/nioc/concurrent/runner.hpp
            PUBLIC include/nioc/concurrent/threadedRunner.hpp
            PUBLIC include/nioc/concurrent/unboundedMpsc.hpp
        INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES
            PUBLIC Boost::headers
            PUBLIC nioc::common
            PUBLIC nioc::logger
        COMPILE_FEATURES
            PUBLIC cxx_std_23
        COMPILE_OPTIONS
            PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
            PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        COMPILE_DEFINITIONS
            "")

if(CLANG_TIDY)
    set_target_properties(concurrent PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
