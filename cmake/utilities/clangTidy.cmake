function(add_clang_tidy)
  set(OPTIONS_ARGUMENTS REQUIRED)
  set(SINGLE_VALUE_ARGUMENTS TARGET VERSION)
  set(MULTI_VALUE_ARGUMENTS "")

  cmake_parse_arguments("ACT_PARAM"
    "${OPTIONS_ARGUMENTS}"
    "${SINGLE_VALUE_ARGUMENTS}"
    "${MULTI_VALUE_ARGUMENTS}"
    ${ARGN})

  require_arguments(PREFIX ACT_PARAM ARGUMENTS TARGET VERSION)

  find_program(ACT_CLANG_TIDY clang-tidy-${ACT_PARAM_VERSION})
  find_program(ACT_RUN_CLANG_TIDY run-clang-tidy-${ACT_PARAM_VERSION})

  if(ACT_CLANG_TIDY AND ACT_RUN_CLANG_TIDY)
    message(STATUS "Found clang-tidy version ${ACT_PARAM_VERSION} at ${ACT_CLANG_TIDY}")
    message(STATUS "Found run-clang-tidy version ${ACT_PARAM_VERSION} at ${ACT_RUN_CLANG_TIDY}")
    message(STATUS "Setting up custom target '${ACT_PARAM_TARGET}' to run clang-tidy.")
    add_custom_target(${ACT_PARAM_TARGET}
      COMMAND
        ${ACT_RUN_CLANG_TIDY}
          -p ${CMAKE_BINARY_DIR}
          -clang-tidy-binary ${ACT_CLANG_TIDY}
          -fix
      WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}
      COMMENT
        "Running clang-tidy with fixes in ${PROJECT_SOURCE_DIR} using clang-tidy at: ${ACT_CLANG_TIDY}")

    set(CLANG_TIDY "${ACT_CLANG_TIDY}" PARENT_SCOPE)
  else()
    if(ACT_PARAM_REQUIRED)
      message(FATAL_ERROR "Unable to find clang-tidy for version ${ACT_PARAM_VERSION}.")
    else()
      message(STATUS "Unable to find clang-tidy for version ${ACT_PARAM_VERSION}. Skipping ${ACT_PARAM_TARGET} setup.")
    endif()
  endif()
endfunction()
