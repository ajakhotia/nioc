ARG OS_BASE=ubuntu:22.04
FROM ${OS_BASE} AS base

# Set dpkg to run in non-interactive mode.
ENV DEBIAN_FRONTEND=noninteractive

# Set shell to return failure code if any command in the pipe fails.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN echo $'Acquire::http::Pipeline-Depth 0;\n\
    Acquire::http::No-Cache true;\n\
    Acquire::BrokenProxy    true;\n'\
    >> /etc/apt/apt.conf.d/90fix-hashsum-mismatch

RUN --mount=type=bind,src=tools,dst=/tools,ro                                                       \
    apt-get update &&                                                                               \
    apt-get full-upgrade -y --no-install-recommends &&                                              \
    apt-get autoclean -y &&                                                                         \
    apt-get autoremove -y &&                                                                        \
    apt-get install -y --no-install-recommends jq &&                                                \
    apt-get install -y --no-install-recommends $(bash /tools/apt/extractDependencies.sh Basics) &&  \
    bash /tools/installCMake.sh &&                                                                  \
    bash /tools/apt/addGNUSources.sh -y &&                                                          \
    bash /tools/apt/addLLVMSources.sh -y &&                                                         \
    bash /tools/apt/addNvidiaSources.sh -y &&                                                       \
    apt-get install -y --no-install-recommends $(bash /tools/apt/extractDependencies.sh Compilers)


FROM base AS dev-base
ARG TOOLCHAIN=linux-gnu-default
ARG ROBOT_FARM_URL="https://github.com/ajakhotia/robotFarm/archive/refs/tags/v1.0.0.tar.gz"
ARG ROBOT_FARM_BUILD_LIST="BoostExternalProject;Eigen3ExternalProject;NlohmannJsonExternalProject;GoogleTestExternalProject;SpdLogExternalProject;CapnprotoExternalProject"

RUN --mount=type=bind,src=cmake/toolchains,dst=/toolchains,ro                                       \
    curl -L -o /tmp/robotFarm.tar.gz ${ROBOT_FARM_URL} &&                                           \
    cmake -E make_directory /tmp/robotFarm-src &&                                                   \
    cmake -E make_directory /tmp/robotFarm-build  &&                                                \
    cmake -E make_directory /opt/robotFarm &&                                                       \
    tar -xzf /tmp/robotFarm.tar.gz -C /tmp/robotFarm-src --strip-components=1 &&                    \
    cmake -G Ninja                                                                                  \
      -S /tmp/robotFarm-src                                                                         \
      -B /tmp/robotFarm-build                                                                       \
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/toolchains/${TOOLCHAIN}.cmake                                \
      -DCMAKE_INSTALL_PREFIX:PATH=/opt/robotFarm                                                    \
      -DROBOT_FARM_REQUESTED_BUILD_LIST:STRING=${ROBOT_FARM_BUILD_LIST} &&                          \
    apt-get update &&                                                                               \
    apt-get install -y --no-install-recommends $(cat /tmp/robotFarm-build/systemDependencies.txt) &&\
    cp /tmp/robotFarm-build/systemDependencies.txt /opt/robotFarm &&                                \
    cmake --build /tmp/robotFarm-build &&                                                           \
    rm -rf /tmp/robotFarm.tar.gz /tmp/robotFarm-src /tmp/robotFarm-build


FROM dev-base AS build
ARG BUILD_TYPE="Release"

RUN --mount=type=bind,src=.,dst=/tmp/nioc-src,ro                                                    \
    cmake -E make_directory /tmp/nioc-build &&                                                      \
    cmake -E make_directory /opt/nioc &&                                                            \
    cmake -G Ninja                                                                                  \
      -S /tmp/nioc-src                                                                              \
      -B /tmp/nioc-build                                                                            \
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/tmp/nioc-src/cmake/toolchains/${TOOLCHAIN}.cmake             \
      -DCMAKE_BUILD_TYPE:STRING=${BUILD_TYPE}                                                       \
      -DCMAKE_PREFIX_PATH:STRING=/opt/robotFarm                                                     \
      -DCMAKE_INSTALL_PREFIX:PATH=/opt/nioc &&                                                      \
    cmake --build /tmp/nioc-build &&                                                                \
    cmake --install /tmp/nioc-build


FROM build AS test
RUN ctest --test-dir /tmp/nioc-build --output-on-failure


FROM dev-base AS deploy
COPY --from=build /opt/nioc /opt/nioc
