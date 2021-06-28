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

include(cmake/capnprotoGenerateMacro.cmake)

add_subdirectory(external)
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

#[[ Configure and install nakshConfig.cmake.in to enable other projects find naksh from
    the install tree. Even though the function configure_package_config_file takes the INSTALL_DESTINATION, it
    doesn't actually create a installation step for "make install". So, to install the generated config file, we
    are required to manually create a install step. ]]
configure_package_config_file(
        cmake/nakshConfig.cmake.in cmake/nakshConfig.cmake
        INSTALL_DESTINATION lib/cmake/naksh)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/nakshConfig.cmake
        DESTINATION lib/cmake/naksh)
