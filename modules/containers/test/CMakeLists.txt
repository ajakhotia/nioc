find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(containersTest
    mmapRegionTest.cpp
    mmapArrayTest.cpp
    mmapConstArrayTest.cpp)

target_link_libraries(containersTest
    GTest::GTest
    GTest::Main
    nioc::containers)

target_compile_options(containersTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(containersTest)
