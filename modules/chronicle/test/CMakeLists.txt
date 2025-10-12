find_package(GTest REQUIRED)
include(GoogleTest)

find_package(Boost CONFIG REQUIRED COMPONENTS iostreams)

add_executable(chronicleTest
    boostMMapExampleTest.cpp
    streamChannelWriterTest.cpp
    mmapChannelReaderTest.cpp
    loggerTest.cpp
    logReaderTest.cpp
    utilsTest.cpp)

target_include_directories(chronicleTest PRIVATE ../src)

target_link_libraries(chronicleTest
    Boost::iostreams
    GTest::GTest
    GTest::Main
    nioc::chronicle)

target_compile_options(chronicleTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(chronicleTest)
