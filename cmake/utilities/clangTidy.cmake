function(add_clang_tidy)
    set(OPTIONS_ARGUMENTS "REQUIRED")
    set(SINGLE_VALUE_ARGUMENTS VERSION)
    set(MULTI_VALUE_ARGUMENTS "")

    cmake_parse_arguments("ACT_PARAM"
            "${OPTIONS_ARGUMENTS}"
            "${SINGLE_VALUE_ARGUMENTS}"
            "${MULTI_VALUE_ARGUMENTS}"
            ${ARGN})

    find_program(CLANG_TIDY clang-tidy-${ACT_PARAM_VERSION} NO_CACHE)
    
    if(CLANG_TIDY)
        message(STATUS "Found clang-tidy program with version ${ACT_PARAM_VERSION} at ${CLANG_TIDY}")
    else()
        message($<IF:${ACT_PARAM_REQUIRED},SEND_ERROR,STATUS>
                "Unable to find clang-tidy for version ${ACT_PARAM_VERSION}.")
    endif()

    set(CLANG_TIDY "${CLANG_TIDY}" PARENT_SCOPE)
endfunction()
