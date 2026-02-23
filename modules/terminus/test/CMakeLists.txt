find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(terminusTest terminusTest.cpp)
target_link_libraries(terminusTest
    GTest::GTest
    GTest::Main
    nioc::chronicle
    nioc::terminus
    nioc::terminusIdl)

target_compile_options(terminusTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(terminusTest)
