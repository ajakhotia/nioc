find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(commonTest lockTest.cpp)
target_link_libraries(commonTest Naksh::common GTest::GTest GTest::Main)

target_compile_options(commonTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)

gtest_discover_tests(commonTest)
