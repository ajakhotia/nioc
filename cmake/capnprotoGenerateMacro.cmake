find_package(CapnProto REQUIRED)

function(capnproto_generate_library)
    cmake_parse_arguments("PARAM" "" "TARGET;NAMESPACE;EXPORT" "FILES" ${ARGN})

    if(NOT PARAM_FILES)
        message(FATAL_ERROR "No schema files were provided. Skipping generation.")
    endif()

    if(NOT PARAM_TARGET)
        message(FATAL_ERROR "No target name specified")
    endif()

    # Acquire the paths to capnp tool, run-time library and interface directory.
    get_target_property(CAPNP_TOOL_PATH CapnProto::capnp_tool LOCATION)
    get_target_property(CAPNP_TOOL_RUNTIME_LIBRARY_PATH CapnProto::capnpc LOCATION)
    get_target_property(CAPNP_INTERFACE_DIRECTORY CapnProto::capnp INTERFACE_INCLUDE_DIRECTORIES)

    get_filename_component(CAPNP_PATH ${CAPNP_TOOL_PATH} DIRECTORY)
    get_filename_component(CAPNP_LIBRARY_PATH ${CAPNP_TOOL_RUNTIME_LIBRARY_PATH} DIRECTORY)

    foreach(SCHEMA_FILE_PATH ${PARAM_FILES})

        # Build absolute and relative paths from the current source directory to the schema file.
        get_filename_component(SCHEMA_FILE_ABSOLUTE_PATH ${SCHEMA_FILE_PATH} ABSOLUTE)
        file(RELATIVE_PATH SCHEMA_FILE_RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include ${SCHEMA_FILE_ABSOLUTE_PATH})

        # Extract filename and prefix.
        get_filename_component(SCHEMA_FILENAME ${SCHEMA_FILE_RELATIVE_PATH} NAME)
        get_filename_component(SCHEMA_PREFIX ${SCHEMA_FILE_RELATIVE_PATH} DIRECTORY)

        # Create a directory that hosts the generated files
        set(SCHEMA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/${SCHEMA_PREFIX})
        file(MAKE_DIRECTORY ${SCHEMA_OUTPUT_DIR})

        # Name of the generated source and header file.
        set(SCHEMA_SOURCE_FILENAME ${SCHEMA_OUTPUT_DIR}/${SCHEMA_FILENAME}.c++)
        set(SCHEMA_HEADER_FILENAME ${SCHEMA_OUTPUT_DIR}/${SCHEMA_FILENAME}.h)

        # Generate c++ sources from schema.
        add_custom_command(
                OUTPUT
                    ${SCHEMA_SOURCE_FILENAME} ${SCHEMA_HEADER_FILENAME}
                COMMAND
                    PATH=${CAPNP_PATH}:$ENV{PATH}
                    LD_LIBRARY_PATH=${CAPNP_LIBRARY_PATH}:$ENV{LD_LIBRARY_PATH}
                    ${CAPNP_TOOL_PATH}
                    compile
                    --no-standard-import
                    --import-path=${CAPNP_INTERFACE_DIRECTORY}
                    --output=c++:${CMAKE_CURRENT_BINARY_DIR}/include
                    --src-prefix=${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${SCHEMA_FILE_ABSOLUTE_PATH}
                MAIN_DEPENDENCY
                    ${SCHEMA_FILE_ABSOLUTE_PATH}
                DEPENDS
                    CapnProto::capnp_tool
                    CapnProto::capnpc
                COMMENT
                    "Compiling capnp schema at ${SCHEMA_FILE_ABSOLUTE_PATH}"
                VERBATIM)

        list(APPEND SCHEMA_SOURCE_FILENAME_LIST ${SCHEMA_SOURCE_FILENAME})

    endforeach()

    # Build a shared library from the generated code.
    add_library(${PARAM_TARGET} SHARED ${SCHEMA_SOURCE_FILENAME_LIST})

    if(PARAM_NAMESPACE)
        add_library(${PARAM_NAMESPACE}${PARAM_TARGET} ALIAS ${PARAM_TARGET})
    endif()

    target_include_directories(${PARAM_TARGET} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>)

    target_link_libraries(${PARAM_TARGET} PUBLIC CapnProto::capnp)


    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
            DESTINATION include
            FILES_MATCHING PATTERN "*.capnp.h")

    install(TARGETS ${PARAM_TARGET} EXPORT ${PARAM_EXPORT})

endfunction()
