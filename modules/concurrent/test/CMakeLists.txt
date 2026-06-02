find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(concurrentTest
  anyMpscTest.cpp
  asyncProcessorTest.cpp
  notifyingInboxTest.cpp
  overwritingMpscTest.cpp
  runnerTest.cpp
  unboundedMpscTest.cpp)

target_link_libraries(concurrentTest
  GTest::GTest
  GTest::Main
  nioc::concurrent)

target_compile_options(concurrentTest PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(concurrentTest)
