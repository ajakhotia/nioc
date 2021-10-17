add_library(common SHARED src/common.cpp)
add_library(Naksh::common ALIAS common)

target_include_directories(common
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(common PUBLIC cxx_std_17)

target_compile_options(common PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)


if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

install(TARGETS common EXPORT nakshTargets)
