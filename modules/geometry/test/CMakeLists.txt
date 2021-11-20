find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(geometryTest frameTest.cpp frameConceptsTest.cpp transformTest.cpp)
target_link_libraries(geometryTest Naksh::geometry GTest::GTest GTest::Main)

target_compile_options(geometryTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(geometryTest)
