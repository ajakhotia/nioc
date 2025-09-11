find_package(GTest REQUIRED)
include(GoogleTest)

find_package(Boost CONFIG REQUIRED COMPONENTS iostreams)

add_executable(loggerTest
    boostMMapExampleTest.cpp
    channelTest.cpp
    channelReaderTest.cpp
    loggerTest.cpp
    logReaderTest.cpp
    utilsTest.cpp)

target_include_directories(loggerTest PRIVATE ../src)

target_link_libraries(loggerTest
    Boost::iostreams
    GTest::GTest
    GTest::Main
    nioc::logger)

target_compile_options(loggerTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(loggerTest)
