#[[ Add the library and its alias. The alias name is determined by the namespace
    into which the targets are exported during installation(see root
    CMakeLists.txt where it says install(EXPORT ... )). Any reference to this
    library outside this file should be made using the namespaced alias. This
    way, we avoid link-conflicts after the targets are exported. ]]
add_library(example SHARED src/example.cpp)
add_library(nioc::example ALIAS example)


#[[ Set the include directories. These directories are passed to the compiler
    during the build using the -I option. We use the keyword PUBLIC to tell the
    build system that any other library that links against this library will
    also need access to these headers.

    Because a client(another project) may refer to this target either from the
    current build-tree (using add_subdirectory) or from an install-tree(using
    find_package), we need to appropriately adjust the path for pre-install
    (BUILD_INTERFACE) and post-install(INSTALL_INTERFACE) reference to this
     library. This is done below using generator expressions. ]]
target_include_directories(example
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)


#[[ Let the build system know that the "example" library and its dependees need
    to compiled with the specified C++ standard. We use the keyword PUBLIC to
    tell the build system that any other library that links against this library
    will also need to be compiled with at least the specified C++ standard. ]]
target_compile_features(example PUBLIC cxx_std_17)


#[[ Set the compile options to an appropriate level. We set them as PRIVATE so
    that any of dependee libraries do not inherit the compile options. This way
    a work-in-progress client libraries won't fail compilation because this
    library was compiled with stricter warning and error levels.

    NOTE:
        -Wall enables unknown-pragmas which is a bit problematic as GCC will
        fail compilation if it sees any pragmas for Clang or MSVC(and
        vice-versa). We explicitly disable this with -Wno-unknown-pragmas. ]]
target_compile_options(example PRIVATE
        PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>)


#[[ Add and executable and its alias. Same reasons as above.]]
add_executable(exampleMain src/exampleMain.cpp)
add_executable(nioc::exampleMain ALIAS exampleMain)


#[[ Set the include directories. These directories are passed to the compiler
    during the build using the -I option. We use the keyword PRIVATE because
    "exampleMain" is an executable and we don't expect other targets to link
    against it. Hence, the information need not be transitive. Additionally, we
    don't have to use any generator expressions as we did for "example" because
    we don't expect other target to link to it. ]]
target_include_directories(exampleMain PRIVATE include)


#[[ Let the build system know that exampleMain needs to be linked against
    nioc::example library for build and run-time(if nioc::example is a shared
    library). Note that we use namespace aliased name(nioc::example) of the
    library here instead of its direct name. ]]
target_link_libraries(exampleMain nioc::example)


#[[ Let the build system know that the "example" library and its dependees need
    to compiled with the specified C++ standard. We use the keyword PRIVATE
    because "exampleMain" is an executable and we don't expect other targets to
    link against it. Hence, the information need not be transitive. ]]
target_compile_features(exampleMain PRIVATE cxx_std_17)


#[[ Set the compile options to an appropriate level. We use the keyword PRIVATE
    because "exampleMain" is an executable and we don't expect other targets to
    link against it. Hence, the information need not be transitive.

    NOTE:
        -Wall enables unknown-pragmas which is a bit problematic as GCC will
        fail compilation if it sees any pragmas for Clang or MSVC(and
        vice-versa). We explicitly disable this with -Wno-unknown-pragmas. ]]
target_compile_options(exampleMain PRIVATE
        PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas>
        PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>)


#[[ Add unit-tests conditionally. A user may-not always want to build unit-tests
    as they typically require external libraries and elaborate setups and there
    will be circumstance wherein, the user will simply want to build the code
    with minimal setup. ]]
if(BUILD_TESTING)
    add_subdirectory(test)
endif()


#[[ Setup install steps for the headers. ]]
install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

#[[ Setup install steps for the created targets. ]]
install(TARGETS example exampleMain EXPORT niocTargets)
