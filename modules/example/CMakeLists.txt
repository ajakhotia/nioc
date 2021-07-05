#[[ Add library and its alias. Any reference to this library should be made
    using the namespaced alias. This way, we avoid linkage conflicts after the
    targets are exported. ]]
add_library(example SHARED src/example.cpp)
add_library(Naksh::example ALIAS example)

#[[ Let the compiler know that the example library and its dependees (hence the use
    of keyword PUBLIC) need access to headers under the include directories. ]]
target_include_directories(example
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

#[[ Let the compiler know that example library and its dependees (hence the use of
    keyword PUBLIC) needs to compiled using the specified C++ standard. ]]
target_compile_features(example PUBLIC cxx_std_17)

#[[ Set the compile options to an appropriate level. We set them as PRIVATE so that any of
    dependee libraries do not inherit the compile options. This way a work-in-progress
    client library won't fail compilation because this library was compiled stricter warning
    and error levels. ]]
target_compile_options(example PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)


#[[ Add and executable and its alias. Same reasons as above.]]
add_executable(exampleMain src/exampleMain.cpp)
add_executable(Naksh::exampleMain ALIAS exampleMain)

#[[ Include directories for executables are PRIVATE because other targets(library or other executables)
    not link against an executable. Hence, there is no need to propagate this information. ]]
target_include_directories(exampleMain
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(exampleMain Naksh::example)

#[[ Compile feature are private for the same reasons as include directories. ]]
target_compile_features(exampleMain PRIVATE cxx_std_17)

#[[ Compile option are private for the same reasons as include directories. ]]
target_compile_options(exampleMain PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)


if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

# Install the created targets as part the following target set.
install(TARGETS example exampleMain EXPORT nakshTargets)
