add_library(logger SHARED src/logger.cpp)
add_library(Naksh::logger ALIAS logger)

target_include_directories(logger
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(logger INTERFACE cxx_std_17)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

install(TARGETS logger EXPORT nakshTargets)
