find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(terminusTest
  testComponent.cpp
  componentTest.cpp
  configStoreTest.cpp
  manifestTest.cpp
  portTest.cpp)

target_link_libraries(terminusTest
  GTest::GTest
  GTest::Main
  nioc::chronicle
  nioc::logger
  nioc::terminus
  nioc::terminusIdl
  nlohmann_json::nlohmann_json)

# Stage the committed test fixtures (config and resource files) next to the binary so portTest
# finds them via its ctest working directory, independent of the source tree.
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_compile_options(terminusTest PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(terminusTest)
