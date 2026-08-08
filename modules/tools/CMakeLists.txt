find_package(Boost CONFIG REQUIRED COMPONENTS headers program_options)

add_exported_executable(
  TARGET
    logInspector
  NAMESPACE
    nioc::
  EXPORT
    niocTargets
  SOURCES
    src/logInspector.cpp
    src/cadenceMonitor.cpp
  LINK_LIBRARIES
    PRIVATE Boost::headers
    PRIVATE Boost::program_options
    PRIVATE nioc::chronicle
    PRIVATE nioc::common
    PRIVATE nioc::concurrent
    PRIVATE nioc::logger
    PRIVATE nioc::terminus
  COMPILE_FEATURES
    PRIVATE cxx_std_23
  COMPILE_OPTIONS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  COMPILE_DEFINITIONS
    "")

if(CLANG_TIDY)
  set_target_properties(logInspector PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()
