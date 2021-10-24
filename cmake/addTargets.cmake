function(add_exported_library)

    set(OPTIONS_ARGUMENTS "")

    set(SINGLE_VALUE_ARGUMENTS
            TARGET
            TYPE
            NAMESPACE
            EXPORT)

    set(MULTI_VALUE_ARGUMENTS
            SRC_FILES
            PUBLIC_INCLUDE_DIR
            PRIVATE_INCLUDE_DIR
            PUBLIC_LINK_LIBRARIES
            PRIVATE_LINK_LIBRARIES
            PUBLIC_HEADERS
            PRIVATE_HEADERS
            COMPILE_FEATURES
            COMPILE_OPTIONS)

    cmake_parse_arguments("AEL_PARAM"
            "${OPTIONS_ARGUMENTS}"
            "${SINGLE_VALUE_ARGUMENTS}"
            "${MULTI_VALUE_ARGUMENTS}"
            ${ARGN})


    add_library(${AEL_PARAM_TARGET} ${AEL_PARAM_TYPE} ${AEL_PARAM_SRC_FILES})

    if(AEL_PARAM_NAMESPACE)
        add_library(${AEL_PARAM_NAMESPACE}${AEL_PARAM_TARGET} ALIAS ${AEL_PARAM_TARGET})
    endif()


    target_include_directories(${AEL_PARAM_TARGET}
            PUBLIC
                $<BUILD_INTERFACE:${AEL_PARAM_PUBLIC_INCLUDE_DIR}>
                $<INSTALL_INTERFACE:include>
            PRIVATE
                ${AEL_PARAM_PRIVATE_INCLUDE_DIR})


    target_link_libraries(${AEL_PARAM_TARGET}
            PUBLIC
                ${AEL_PARAM_PUBLIC_LINK_LIBRARIES}
            PRIVATE
                ${AEL_PARAM_PRIVATE_LINK_LIBRARIES})


    target_compile_features(${AEL_PARAM_TARGET} PUBLIC ${AEL_PARAM_COMPILE_FEATURES})
    target_compile_options(${AEL_PARAM_TARGET} PRIVATE ${AEL_PARAM_COMPILE_OPTIONS})

    install(DIRECTORY ${AEL_PARAM_PUBLIC_INCLUDE_DIR}/
            DESTINATION include
            FILES_MATCHING PATTERN "*.h*")

    install(TARGETS ${AEL_PARAM_TARGET} EXPORT ${AEL_PARAM_EXPORT})

endfunction()


function(add_exported_executable)

    set(OPTIONS_ARGUMENTS "")

    set(SINGLE_VALUE_ARGUMENTS
            TARGET
            TYPE
            NAMESPACE
            EXPORT)

    set(MULTI_VALUE_ARGUMENTS
            SRC_FILES
            PRIVATE_INCLUDE_DIR
            PRIVATE_LINK_LIBRARIES
            PRIVATE_HEADERS
            COMPILE_FEATURES
            COMPILE_OPTIONS)

    cmake_parse_arguments("AEL_PARAM"
            "${OPTIONS_ARGUMENTS}"
            "${SINGLE_VALUE_ARGUMENTS}"
            "${MULTI_VALUE_ARGUMENTS}"
            ${ARGN})


    add_executable(${AEL_PARAM_TARGET} ${AEL_PARAM_SRC_FILES})

    if(AEL_PARAM_NAMESPACE)
        add_executable(${AEL_PARAM_NAMESPACE}${AEL_PARAM_TARGET} ALIAS ${AEL_PARAM_TARGET})
    endif()

    target_include_directories(${AEL_PARAM_TARGET} PRIVATE ${AEL_PARAM_PRIVATE_INCLUDE_DIR})
    target_link_libraries(${AEL_PARAM_TARGET} PRIVATE ${AEL_PARAM_PRIVATE_LINK_LIBRARIES})

    target_compile_features(${AEL_PARAM_TARGET} PRIVATE ${AEL_PARAM_COMPILE_FEATURES})
    target_compile_options(${AEL_PARAM_TARGET} PRIVATE ${AEL_PARAM_COMPILE_OPTIONS})

    install(TARGETS ${AEL_PARAM_TARGET} EXPORT ${AEL_PARAM_EXPORT})

endfunction()
