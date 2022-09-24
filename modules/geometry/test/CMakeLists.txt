find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(geometryTest
        frameTest.cpp
        frameConceptsTest.cpp
        frameReferencesTest.cpp
        rotationTest.cpp
        poseTest.cpp
        rigidTransformTest.cpp)

target_link_libraries(geometryTest nioc::geometry nioc::common GTest::GTest GTest::Main)

target_compile_options(geometryTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

target_compile_features(geometryTest PRIVATE cxx_std_20)

gtest_discover_tests(geometryTest)
