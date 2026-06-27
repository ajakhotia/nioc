find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(commonTest
    exceptionTest.cpp
    filesystemTest.cpp
    lockTest.cpp
    makeBimapTest.cpp
    raiiTokenTest.cpp
    signalCatcherTest.cpp
    timeTest.cpp
    typeTraitsTest.cpp
    utilsTest.cpp)

target_link_libraries(commonTest nioc::common GTest::GTest GTest::Main)

target_compile_options(commonTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

if(CLANG_TIDY)
  set_target_properties(commonTest PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()

gtest_discover_tests(commonTest)
