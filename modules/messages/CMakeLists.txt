find_package(CapnProto REQUIRED)

capnproto_generate_library(Naksh:: messagesIdl nakshTargets
        include/naksh/messages/idl/sample1.capnp
        include/naksh/messages/idl/sample2.capnp)

add_library(messages SHARED src/messageBase.cpp)
add_library(Naksh::messages ALIAS messages)

target_include_directories(messages PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(messages PUBLIC CapnProto::capnp CapnProto::kj)

target_compile_features(messages PUBLIC cxx_std_17)

target_compile_options(messages PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>)


if(BUILD_TESTING)
    add_subdirectory(test)
endif()


install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")

install(TARGETS messages EXPORT nakshTargets)
