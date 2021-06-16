add_library(example SHARED src/example.cpp)
add_library(Naksh::example ALIAS example)

target_include_directories(example
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(example INTERFACE cxx_std_17)


add_executable(exampleMain src/exampleMain.cpp)
add_executable(Naksh::exampleMain ALIAS exampleMain)

target_include_directories(exampleMain
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(exampleMain INTERFACE cxx_std_17)

target_link_libraries(exampleMain Naksh::example)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

install(TARGETS example exampleMain EXPORT nakshTargets)
