find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(loggerTest
    loggerTest.cpp)

target_link_libraries(loggerTest nioc::logger GTest::GTest GTest::Main)

target_compile_options(loggerTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

if(CLANG_TIDY)
  set_target_properties(loggerTest PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()

gtest_discover_tests(loggerTest)
