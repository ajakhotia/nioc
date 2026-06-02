find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(terminusTest
  testComponent.cpp
  componentTest.cpp
  portTest.cpp)

target_link_libraries(terminusTest
  GTest::GTest
  GTest::Main
  nioc::chronicle
  nioc::logger
  nioc::terminus
  nioc::terminusIdl)

# Path to the committed test fixtures (config and resource files) read by portTest.
target_compile_definitions(terminusTest PRIVATE
  NIOC_TERMINUS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

target_compile_options(terminusTest PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(terminusTest)
