find_package(GTest REQUIRED)
include(GoogleTest)

find_package(Boost REQUIRED COMPONENTS iostreams)

add_executable(loggerTest boostMMapExampleTest.cpp loggerTest.cpp channelTest.cpp)

target_link_libraries(loggerTest Naksh::logger Boost::iostreams GTest::GTest GTest::Main)

target_compile_options(loggerTest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>)

gtest_discover_tests(loggerTest)
