function(add_clang_format)
  set(OPTIONS_ARGUMENTS REQUIRED)
  set(SINGLE_VALUE_ARGUMENTS TARGET VERSION)
  set(MULTI_VALUE_ARGUMENTS "")

  cmake_parse_arguments("ACF_PARAM"
    "${OPTIONS_ARGUMENTS}"
    "${SINGLE_VALUE_ARGUMENTS}"
    "${MULTI_VALUE_ARGUMENTS}"
    ${ARGN})

  require_arguments(PREFIX ACF_PARAM ARGUMENTS TARGET VERSION)

  find_program(ACF_CLANG_FORMAT clang-format-${ACF_PARAM_VERSION})
  find_program(ACF_XARGS xargs)

  if(ACF_CLANG_FORMAT AND ACF_XARGS)
    message(STATUS "Found clang-format version ${ACF_PARAM_VERSION} at ${ACF_CLANG_FORMAT}")
    message(STATUS "Found xargs at ${ACF_XARGS}")
    message(STATUS "Setting up custom target '${ACF_PARAM_TARGET}' to run clang-format.")
    add_custom_target(${ACF_PARAM_TARGET}
      COMMAND
        ${ACF_CLANG_FORMAT} --version
      COMMAND
        find
          -not -path \"*build*\" -and
          -not -path \"${CMAKE_BINARY_DIR}\" -and
          \\\(
          -iname *.cpp -o -iname *.hpp -o
          -iname *.c -o -iname *.h -o
          -iname *.cc -o -iname *.hh
          \\\) | ${XARGS} ${CLANG_FORMAT} -style=file -i
      WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}
      COMMENT
        "Formatting files in ${PROJECT_SOURCE_DIR} using clang-format at: ${ACF_CLANG_FORMAT}")
  else()
    if(ACF_PARAM_REQUIRED)
      message(FATAL_ERROR "Unable to find clang-format and/or xargs for version ${ACF_PARAM_VERSION}.")
    else()
      message(STATUS "Unable to find clang-format and/or xargs for version ${ACF_PARAM_VERSION}. Skipping ${ACF_PARAM_TARGET} setup.")
    endif()
  endif()
endfunction()
