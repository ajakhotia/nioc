# syntax=docker/dockerfile:1.7
ARG OS_BASE=ghcr.io/ajakhotia/infracommons/weekly-base/ubuntu-24-04:latest

FROM ${OS_BASE} AS base

ARG OS_BASE
ENV OS_BASE=${OS_BASE}
ENV APT_VAR_CACHE_ID=nioc-apt-var-cache-${OS_BASE}
ENV APT_LIST_CACHE_ID=nioc-apt-list-cache-${OS_BASE}
ENV DEBIAN_FRONTEND=noninteractive

# The CUDA toolkit installs outside the default search paths. Put it on PATH so that builds
# which don't pass a toolchain file (e.g. the robotFarm build below) resolve nvcc by default,
# mirroring how update-alternatives resolves the host compilers further down.
ENV PATH=/usr/local/cuda/bin:${PATH}

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# The base image already ships the vendor apt sources and the bootstrap package set. jq is the
# one extra tool needed here because extractDependencies.sh uses it to read
# systemDependencies.json.
RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get update &&                                                                              \
    apt-get install -y --no-install-recommends                                                     \
      jq

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    --mount=type=bind,src=external/infraCommons/tools/extractDependencies.sh,dst=/tmp/extractDependencies.sh \
    --mount=type=bind,src=systemDependencies.json,dst=/tmp/systemDependencies.json                 \
    apt-get update &&                                                                              \
    apt-get install -y --no-install-recommends                                                     \
      $(sh /tmp/extractDependencies.sh                                                             \
          "Basics Compilers RobotFarmDependencies"                                                 \
          /tmp/systemDependencies.json)

RUN gnu=$(ls /usr/bin | grep -E '^gcc-[0-9]+$' | sort -V | tail -n 1 | cut -d- -f2) &&             \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${gnu} 100                         \
      --slave /usr/bin/g++ g++ /usr/bin/g++-${gnu}                                                 \
      --slave /usr/bin/gfortran gfortran /usr/bin/gfortran-${gnu} &&                               \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100 &&                               \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 100

# Register the newest installed LLVM compilers as the defaults for the unversioned clang names.
RUN llvm=$(ls /usr/bin | grep -E '^clang-[0-9]+$' | sort -V | tail -n 1 | cut -d- -f2) &&          \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${llvm} 100                  \
      --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${llvm}                                    \
      --slave /usr/bin/flang flang /usr/bin/flang-${llvm}


# Nothing FROMs this stage as an image base; dev-base only pulls /opt/robotFarm forward via
# COPY, so the source and build trees never get committed.
FROM base AS throw-away-dev-base
ARG ROBOTFARM_VERSION=v2.3.0
ARG ROBOTFARM_BUILD_LIST="BoostExternalProject;Eigen3ExternalProject;NlohmannJsonExternalProject;GoogleTestExternalProject;SpdLogExternalProject;CapnprotoExternalProject"

RUN git clone --depth 1 --branch ${ROBOTFARM_VERSION}                                              \
      https://github.com/ajakhotia/robotFarm.git /tmp/robotFarm-src &&                             \
    git -C /tmp/robotFarm-src submodule update --init --depth 1

# CMAKE_CUDA_ARCHITECTURES is set explicitly because no toolchain file provides it here;
# robotFarm forwards it to CUDA-using sub-builds (e.g. SuiteSparse).
RUN cmake -G Ninja                                                                                 \
      -S /tmp/robotFarm-src                                                                        \
      -B /tmp/robotFarm-build                                                                      \
      -DCMAKE_BUILD_TYPE:STRING=Release                                                            \
      -DBUILD_SHARED_LIBS:BOOL=ON                                                                  \
      -DCMAKE_INSTALL_PREFIX:PATH=/opt/robotFarm                                                   \
      -DCMAKE_CUDA_ARCHITECTURES:STRING="75;80"                                                    \
      -DROBOT_FARM_REQUESTED_BUILD_LIST:STRING=${ROBOTFARM_BUILD_LIST}

RUN cmake --build /tmp/robotFarm-build


FROM base AS dev-base
COPY --from=throw-away-dev-base /opt/robotFarm /opt/robotFarm
