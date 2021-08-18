find_package(Boost REQUIRED COMPONENTS iostreams)
find_package(spdlog REQUIRED)

naksh_capnproto_generate_cpp(Naksh:: loggerIdl
        include/naksh/logger/idl/frame.capnp)

add_library(logger SHARED src/logger.cpp)
add_library(Naksh::logger ALIAS logger)

target_include_directories(logger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(logger PRIVATE Naksh::loggerIdl spdlog::spdlog Boost::iostreams)

target_compile_features(logger PUBLIC cxx_std_17)

target_compile_options(logger PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

install(TARGETS logger EXPORT nakshTargets)
