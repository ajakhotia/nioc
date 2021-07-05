find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(messagesTest messagesTest.cpp)
target_link_libraries(messagesTest Naksh::messages Naksh::messagesIdl GTest::GTest GTest::Main)

target_compile_options(messagesTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>)

gtest_discover_tests(messagesTest)
