find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(chronicleTest
    crateTest.cpp
    channelTest.cpp
    readerTest.cpp
    utilsTest.cpp
    writerTest.cpp)

target_include_directories(chronicleTest PRIVATE ../src)

target_link_libraries(chronicleTest
    GTest::GTest
    GTest::Main
    nioc::chronicle)

target_compile_options(chronicleTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(chronicleTest)
