add_custom_target(niocDockerDevBaseUbuntu2204
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-22-04/dev-base:latest                                 \
          --build-arg OS_BASE=ubuntu:22.04                                                          \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntuDevBase.dockerfile                                 \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc dev-base docker image for ubuntu 22.04"
    VERBATIM
    USES_TERMINAL)

add_custom_target(niocDockerDevBaseUbuntu2404
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-24-04/dev-base:latest                                 \
          --build-arg OS_BASE=ubuntu:24.04                                                          \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntuDevBase.dockerfile                                 \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc dev-base docker image for ubuntu 24.04"
    VERBATIM
    USES_TERMINAL)

add_custom_target(niocDockerBuildUbuntu2204LinuxGnu15
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-22-04/linux-gnu-15:latest                             \
          --build-arg DEV_BASE_IMAGE=ghcr.io/ajakhotia/nioc/ubuntu-22-04/dev-base:latest            \
          --build-arg TOOLCHAIN=linux-gnu-15                                                        \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntuNioc.dockerfile                                    \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc docker image for ubuntu 22.04 with linux-gnu-15"
    VERBATIM
    USES_TERMINAL)

add_custom_target(niocDockerBuildUbuntu2404LinuxGnu15
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-24-04/linux-gnu-15:latest                             \
          --build-arg DEV_BASE_IMAGE=ghcr.io/ajakhotia/nioc/ubuntu-24-04/dev-base:latest            \
          --build-arg TOOLCHAIN=linux-gnu-15                                                        \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntuNioc.dockerfile                                    \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc docker image for ubuntu 24.04 with linux-gnu-15"
    VERBATIM
    USES_TERMINAL)

add_custom_target(niocDockerBuildUbuntu2204LinuxClang22
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-22-04/linux-clang-22:latest                           \
          --build-arg DEV_BASE_IMAGE=ghcr.io/ajakhotia/nioc/ubuntu-22-04/dev-base:latest            \
          --build-arg TOOLCHAIN=linux-clang-22                                                      \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntuNioc.dockerfile                                    \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc docker image for ubuntu 22.04 with linux-clang-22"
    VERBATIM
    USES_TERMINAL)

add_custom_target(niocDockerBuildUbuntu2404LinuxClang22
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-24-04/linux-clang-22:latest                           \
          --build-arg DEV_BASE_IMAGE=ghcr.io/ajakhotia/nioc/ubuntu-24-04/dev-base:latest            \
          --build-arg TOOLCHAIN=linux-clang-22                                                      \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntuNioc.dockerfile                                    \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc docker image for ubuntu 24.04 with linux-clang-22"
    VERBATIM
    USES_TERMINAL)
