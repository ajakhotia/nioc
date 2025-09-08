add_custom_target(niocDockerBuildUbuntu2204LinuxGnuDefault
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-22-04/linux-gnu-default:latest                        \
          --build-arg OS_BASE=ubuntu:22.04                                                          \
          --build-arg TOOLCHAIN=linux-gnu-default                                                   \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntu.dockerfile                                        \
          ${nioc_SOURCE_DIR}"
    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc docker image for ubuntu 22.04 with linux-gnu-default"
    VERBATIM
    USES_TERMINAL)

add_custom_target(niocDockerBuildUbuntu2404LinuxGnuDefault
    COMMAND
      bash -c "docker buildx build --progress=plain                                                 \
          --force-rm                                                                                \
          --tag ghcr.io/ajakhotia/nioc/ubuntu-24-04/linux-gnu-default:latest                        \
          --build-arg OS_BASE=ubuntu:24.04                                                          \
          --build-arg TOOLCHAIN=linux-gnu-default                                                   \
          --file ${CMAKE_CURRENT_LIST_DIR}/ubuntu.dockerfile                                        \
          ${nioc_SOURCE_DIR}"

    WORKING_DIRECTORY
      ${nioc_SOURCE_DIR}
    COMMENT
      "Building nioc docker image for ubuntu 24.04 with linux-gnu-default"
    VERBATIM
    USES_TERMINAL)
