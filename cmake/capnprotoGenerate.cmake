find_package(CapnProto REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/addTargets.cmake)

function(capnproto_generate_library)

    set(OPTIONS_ARGUMENTS "")

    set(SINGLE_VALUE_ARGUMENTS
            TARGET
            TYPE
            NAMESPACE
            EXPORT)

    set(MULTI_VALUE_ARGUMENTS
            SCHEMA_FILES
            COMPILE_FEATURES
            COMPILE_OPTIONS)

    cmake_parse_arguments("CGL_PARAM"
            "${OPTIONS_ARGUMENTS}"
            "${SINGLE_VALUE_ARGUMENTS}"
            "${MULTI_VALUE_ARGUMENTS}"
            ${ARGN})

    if(NOT CGL_PARAM_SCHEMA_FILES)
        message(FATAL_ERROR "No schema files were provided. Skipping generation.")
    endif()

    # Acquire the paths to capnp tool, run-time library and interface directory.
    get_target_property(CAPNP_TOOL_PATH CapnProto::capnp_tool LOCATION)
    get_target_property(CAPNP_TOOL_RUNTIME_LIBRARY_PATH CapnProto::capnpc LOCATION)
    get_target_property(CAPNP_INTERFACE_DIRECTORY CapnProto::capnp INTERFACE_INCLUDE_DIRECTORIES)

    get_filename_component(CAPNP_PATH ${CAPNP_TOOL_PATH} DIRECTORY)
    get_filename_component(CAPNP_LIBRARY_PATH ${CAPNP_TOOL_RUNTIME_LIBRARY_PATH} DIRECTORY)

    foreach(SCHEMA_FILE_PATH ${CGL_PARAM_SCHEMA_FILES})

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
        list(APPEND SCHEMA_HEADER_FILENAME_LIST ${SCHEMA_HEADER_FILENAME})

    endforeach()

    add_exported_library(
            TARGET
                ${CGL_PARAM_TARGET}
            TYPE
                ${CGL_PARAM_TYPE}
            NAMESPACE
                ${CGL_PARAM_NAMESPACE}
            EXPORT
                ${CGL_PARAM_EXPORT}
            SRC_FILES
                ${SCHEMA_SOURCE_FILENAME_LIST}
            PUBLIC_INCLUDE_DIR
                ${CMAKE_CURRENT_BINARY_DIR}/include
            PRIVATE_INCLUDE_DIR
                ""
            PUBLIC_LINK_LIBRARIES
                CapnProto::capnp
            PRIVATE_LINK_LIBRARIES
                ""
            PUBLIC_HEADERS
                ${SCHEMA_HEADER_FILENAME_LIST}
            PRIVATE_HEADERS
                ""
            COMPILE_FEATURES
                ${CGL_PARAM_COMPILE_FEATURES}
            COMPILE_OPTIONS
                ${CGL_PARAM_COMPILE_OPTIONS})

endfunction()
