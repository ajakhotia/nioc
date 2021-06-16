find_package(CapnProto REQUIRED)

capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS
        include/naksh/messages/idl/sample.capnp)


add_library(messages SHARED
        src/message.cpp
        ${CAPNP_SRCS})

add_library(Naksh::messages ALIAS messages)


target_include_directories(messages
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(messages INTERFACE cxx_std_17)
target_link_libraries(messages PUBLIC CapnProto::capnp)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.capnp.h")

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

install(TARGETS messages EXPORT nakshTargets)
