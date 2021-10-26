cmake_minimum_required(VERSION 3.16)

project(naksh VERSION 0.0.0.0 LANGUAGES C CXX)


#[[ Setup unit testing framework. ]]
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
else()
    find_package(GTest)
endif()


if((TARGET GTest::GTest) AND (TARGET GTest::Main))
    include(CTest)
endif()


#[[ Only add external dependencies to the source tree from here if this project
    is the top-level project. If this project has been included with-in another
    project as a submodule, then the onus of setting up the required external
    dependencies fall on that top-level project. This is done in order to
    maintain a single flat level of external sub-modules/sub-repos and avoid a
    tree of sub-modules with potential for conflict and multiple points of
    truth. ]]
if(${CMAKE_SOURCE_DIR} STREQUAL ${naksh_SOURCE_DIR})
    add_subdirectory(external)
endif()


add_subdirectory(modules)
add_subdirectory(docker)


#[[ Include cmake-default tools to create export files. ]]
include(CMakePackageConfigHelpers)

#[[ Export the target metadata to the install tree. ]]
install(EXPORT nakshTargets NAMESPACE Naksh::
        FILE nakshTargets.cmake
        DESTINATION lib/cmake/naksh)

#[[ Generate config file and install it. ]]
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/nakshConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/nakshConfigVersion.cmake
        DESTINATION lib/cmake/naksh)

#[[ Configure and install nakshConfig.cmake.in to enable other projects find
    naksh from the install tree. Even though the function
    "configure_package_config_file" takes the INSTALL_DESTINATION, it doesn't
    actually create a installation step for "make install". So, to install the
    generated config file, we are required to manually create a install step. ]]
configure_package_config_file(
        cmake/nakshConfig.cmake.in cmake/nakshConfig.cmake
        INSTALL_DESTINATION lib/cmake/naksh)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/nakshConfig.cmake
        DESTINATION lib/cmake/naksh)
