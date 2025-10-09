find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(messagesTest messagesTest.cpp)
target_link_libraries(messagesTest
    GTest::GTest
    GTest::Main
    nioc::chronicle
    nioc::messages
    nioc::messagesIdl)

target_compile_options(messagesTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(messagesTest)
