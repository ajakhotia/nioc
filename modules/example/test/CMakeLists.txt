find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(exampleTest exampleTest.cpp)
target_link_libraries(exampleTest Naksh::example GTest::GTest GTest::Main)

target_compile_options(exampleTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)

gtest_discover_tests(exampleTest)
