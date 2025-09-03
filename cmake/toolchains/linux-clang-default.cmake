set(CMAKE_HOST_SYSTEM_NAME "Linux")
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_Fortran_COMPILER /usr/bin/flang-new)

execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE CLANG_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(CLANG_VERSION VERSION_LESS "19")
  message(STATUS "Detected default clang as ${CLANG_VERSION} < 19 — forcing clang-19, clang++-19, and flang-new-19")
  set(CMAKE_C_COMPILER /usr/bin/clang-19)
  set(CMAKE_CXX_COMPILER /usr/bin/clang++-19)
  set(CMAKE_Fortran_COMPILER /usr/bin/flang-new-19)
endif()

set(CMAKE_CUDA_COMPILER /usr/local/cuda-13/bin/nvcc)
set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
set(CMAKE_CUDA_ARCHITECTURES 75;80)
