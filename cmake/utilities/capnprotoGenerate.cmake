include(${CMAKE_CURRENT_LIST_DIR}/exportedTargets.cmake)

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
    COMPILE_OPTIONS
    COMPILE_DEFINITIONS
    LINK_LIBRARIES)

  cmake_parse_arguments("CGL_PARAM"
    "${OPTIONS_ARGUMENTS}"
    "${SINGLE_VALUE_ARGUMENTS}"
    "${MULTI_VALUE_ARGUMENTS}"
    ${ARGN})

  if(NOT CGL_PARAM_SCHEMA_FILES)
    message(FATAL_ERROR "No schema files were provided. Skipping generation.")
  endif()

  # Locate the Cap'n Proto package.
  find_package(CapnProto REQUIRED)

  # Acquire the paths to the capnp tool, run-time library, and interface directory.
  get_target_property(CGL_CAPNP_TOOL_PATH CapnProto::capnp_tool LOCATION)
  get_target_property(CGL_CAPNP_TOOL_RUNTIME_LIBRARY_PATH CapnProto::capnpc LOCATION)
  get_target_property(CGL_CAPNP_INTERFACE_DIRECTORY CapnProto::capnp INTERFACE_INCLUDE_DIRECTORIES)

  get_filename_component(CGL_CAPNP_TOOL_DIRECTORY ${CGL_CAPNP_TOOL_PATH} DIRECTORY)
  get_filename_component(CGL_CAPNP_RUNTIME_LIBRARY_DIRECTORY ${CGL_CAPNP_TOOL_RUNTIME_LIBRARY_PATH} DIRECTORY)

  # Collect --import-path flags from each linked library's INTERFACE_INCLUDE_DIRECTORIES.
  #
  # For build-tree targets, this property contains raw generator expressions such as
  # $<BUILD_INTERFACE:/path/to/include> and $<INSTALL_INTERFACE:include>. The latter must
  # be filtered out here: it evaluates to an empty string in the build context and would
  # produce a spurious empty argument in the capnp compile command. BUILD_INTERFACE genexes
  # are passed directly to add_custom_command and resolved to concrete paths at generation time.
  #
  # For installed targets (imported via find_package), CMake has already resolved all generator
  # expressions to concrete absolute paths when writing the export file. These paths pass
  # through the filter unchanged and point to where the .capnp schema files were installed.
  set(CGL_DEPENDENCY_IMPORT_PATH_FLAGS "")
  foreach(CGL_LINKED_LIBRARY IN LISTS CGL_PARAM_LINK_LIBRARIES)
    get_target_property(CGL_LINKED_LIBRARY_IMPORT_DIRECTORIES
      ${CGL_LINKED_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES)
    foreach(CGL_IMPORT_DIRECTORY IN LISTS CGL_LINKED_LIBRARY_IMPORT_DIRECTORIES)
      if(NOT CGL_IMPORT_DIRECTORY MATCHES "INSTALL_INTERFACE")
        list(APPEND CGL_DEPENDENCY_IMPORT_PATH_FLAGS
          "--import-path=${CGL_IMPORT_DIRECTORY}")
      endif()
    endforeach()
  endforeach()

  set(CGL_GENERATED_SOURCE_FILENAME_LIST "")
  set(CGL_GENERATED_HEADER_FILENAME_LIST "")

  foreach(CGL_SCHEMA_FILE_PATH IN LISTS CGL_PARAM_SCHEMA_FILES)

    # Build absolute and relative paths from the current source directory to the schema file.
    get_filename_component(CGL_SCHEMA_FILE_ABSOLUTE_PATH ${CGL_SCHEMA_FILE_PATH} ABSOLUTE)
    file(RELATIVE_PATH CGL_SCHEMA_FILE_RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include ${CGL_SCHEMA_FILE_ABSOLUTE_PATH})

    # Extract filename and directory prefix.
    get_filename_component(CGL_SCHEMA_FILENAME ${CGL_SCHEMA_FILE_RELATIVE_PATH} NAME)
    get_filename_component(CGL_SCHEMA_FILE_DIRECTORY ${CGL_SCHEMA_FILE_RELATIVE_PATH} DIRECTORY)

    # Create the directory that hosts the generated files.
    set(CGL_GENERATED_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/${CGL_SCHEMA_FILE_DIRECTORY})
    file(MAKE_DIRECTORY ${CGL_GENERATED_OUTPUT_DIRECTORY})

    # Paths for the generated source and header files.
    set(CGL_GENERATED_SOURCE_FILENAME ${CGL_GENERATED_OUTPUT_DIRECTORY}/${CGL_SCHEMA_FILENAME}.c++)
    set(CGL_GENERATED_HEADER_FILENAME ${CGL_GENERATED_OUTPUT_DIRECTORY}/${CGL_SCHEMA_FILENAME}.h)

    # Generate C++ sources from schema.
    add_custom_command(
      OUTPUT
        ${CGL_GENERATED_SOURCE_FILENAME} ${CGL_GENERATED_HEADER_FILENAME}
      COMMAND
        PATH=${CGL_CAPNP_TOOL_DIRECTORY}:$ENV{PATH}
        LD_LIBRARY_PATH=${CGL_CAPNP_RUNTIME_LIBRARY_DIRECTORY}:$ENV{LD_LIBRARY_PATH}
        ${CGL_CAPNP_TOOL_PATH}
        compile
        --no-standard-import
        --import-path=${CGL_CAPNP_INTERFACE_DIRECTORY}
        ${CGL_DEPENDENCY_IMPORT_PATH_FLAGS}
        --output=c++:${CMAKE_CURRENT_BINARY_DIR}/include
        --src-prefix=${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CGL_SCHEMA_FILE_ABSOLUTE_PATH}
      MAIN_DEPENDENCY
        ${CGL_SCHEMA_FILE_ABSOLUTE_PATH}
      DEPENDS
        CapnProto::capnp_tool
        CapnProto::capnpc
      COMMENT
        "Compiling capnp schema at ${CGL_SCHEMA_FILE_ABSOLUTE_PATH}"
      VERBATIM)

    file(RELATIVE_PATH CGL_GENERATED_HEADER_RELATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} ${CGL_GENERATED_HEADER_FILENAME})

    list(APPEND CGL_GENERATED_SOURCE_FILENAME_LIST ${CGL_GENERATED_SOURCE_FILENAME})
    list(APPEND CGL_GENERATED_HEADER_FILENAME_LIST ${CGL_GENERATED_HEADER_RELATIVE_PATH})

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
    SOURCES
      ${CGL_GENERATED_SOURCE_FILENAME_LIST}
    HEADERS
      PUBLIC ${CGL_GENERATED_HEADER_FILENAME_LIST}
    INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_BINARY_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBRARIES
      PUBLIC CapnProto::capnp
      PUBLIC ${CGL_PARAM_LINK_LIBRARIES}
    COMPILE_FEATURES
      ${CGL_PARAM_COMPILE_FEATURES}
    COMPILE_OPTIONS
      ${CGL_PARAM_COMPILE_OPTIONS}
    COMPILE_DEFINITIONS
      ${CGL_PARAM_COMPILE_DEFINITIONS})

endfunction()
