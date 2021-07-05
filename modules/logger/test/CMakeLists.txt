find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(loggerTest boostMMapWriteTest.cpp)
target_link_libraries(loggerTest Naksh::logger GTest::GTest GTest::Main)

target_compile_options(loggerTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)

gtest_discover_tests(loggerTest)
