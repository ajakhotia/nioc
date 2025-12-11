cmake_minimum_required(VERSION 3.27)

project(nioc VERSION 0.0.0.0 LANGUAGES C CXX)


#[[ Setup unit testing framework. ]]
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
else()
    find_package(GTest)
endif()

if((TARGET GTest::GTest) AND (TARGET GTest::Main))
    include(CTest)
endif()


#[[ Setup project level tools and external dependencies if and only if this project
    is the top-level project. If this project has been included with-in another
    project as a submodule, then the onus of setting up these requirement falls on that
    top-level project. This prevent this project from overreaching when it is used as a submodule
    in another project. ]]
if(PROJECT_IS_TOP_LEVEL)
  include(cmake/utilities/clangFormat.cmake)
  include(cmake/utilities/clangTidy.cmake)
  include(cmake/utilities/exportedTargets.cmake)

  add_clang_format(TARGET niocClangFormat VERSION 19)
  add_clang_tidy(VERSION 19)

  add_subdirectory(external)
  add_subdirectory(docker)
endif()


include(cmake/utilities/capnprotoGenerate.cmake)
add_subdirectory(modules)


#[[ Include cmake-default tools to create export files. ]]
include(CMakePackageConfigHelpers)

#[[ Export the target metadata to the install tree. ]]
install(EXPORT niocTargets NAMESPACE nioc::
        FILE niocTargets.cmake
        DESTINATION lib/cmake/nioc)

#[[ Generate config file and install it. ]]
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/niocConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/niocConfigVersion.cmake
        DESTINATION lib/cmake/nioc)

#[[ Configure and install niocConfig.cmake.in to enable other projects find
    nioc from the install tree. Even though the function
    "configure_package_config_file" takes the INSTALL_DESTINATION, it doesn't
    actually create a installation step for "make install". So, to install the
    generated config file, we are required to manually create a install step. ]]
configure_package_config_file(
        cmake/niocConfig.cmake.in cmake/niocConfig.cmake
        INSTALL_DESTINATION lib/cmake/nioc)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/niocConfig.cmake
        DESTINATION lib/cmake/nioc)
