find_package(Boost CONFIG REQUIRED COMPONENTS headers)
find_package(Threads REQUIRED)

add_exported_library(
  TARGET
    common
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/signalCatcher.cpp
    src/utils.cpp
  HEADERS
    PUBLIC include/nioc/common/annotators.hpp
    PUBLIC include/nioc/common/exception.hpp
    PUBLIC include/nioc/common/filesystem.hpp
    PUBLIC include/nioc/common/locked.hpp
    PUBLIC include/nioc/common/makeBimap.hpp
    PUBLIC include/nioc/common/raiiToken.hpp
    PUBLIC include/nioc/common/signalCatcher.hpp
    PUBLIC include/nioc/common/time.hpp
    PUBLIC include/nioc/common/typeTraits.hpp
    PUBLIC include/nioc/common/utils.hpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  LINK_LIBRARIES
    PUBLIC Boost::headers
    PRIVATE Threads::Threads
  COMPILE_FEATURES
    PUBLIC cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")

if(CLANG_TIDY)
  set_target_properties(common PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
