# syntax=docker/dockerfile:1.7
ARG OS_BASE=ubuntu:22.04

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

RUN printf '%s\n'                                                                                  \
    'path-exclude /usr/share/doc/*'                                                                \
    'path-exclude /usr/share/man/*'                                                                \
    'path-include /usr/share/locale/locale.alias'                                                  \
    'path-include /usr/share/locale/en*/*'                                                         \
    'path-exclude /usr/share/locale/*'                                                             \
    'path-exclude /usr/share/info/*'                                                               \
    > /etc/dpkg/dpkg.cfg.d/01_nodoc

RUN printf '%s\n'                                                                                  \
    'Acquire::http::Pipeline-Depth 0;'                                                             \
    'Acquire::https::Pipeline-Depth 0;'                                                            \
    'Acquire::http::No-Cache true;'                                                                \
    'Acquire::https::No-Cache true;'                                                               \
    'Acquire::BrokenProxy    true;'                                                                \
    >> /etc/apt/apt.conf.d/90fix-hashsum-mismatch

# Make apt resilient to flaky upstreams (e.g., Launchpad PPA hosting under stress):
# 5 retries with short per-request timeouts so each failed attempt fails fast and
# we get more shots at reaching a healthy backend behind the load balancer.
RUN printf '%s\n'                                                                                  \
    'Acquire::Retries "5";'                                                                        \
    'Acquire::http::Timeout "30";'                                                                 \
    'Acquire::https::Timeout "30";'                                                                \
    > /etc/apt/apt.conf.d/91retry-and-timeouts

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get update &&                                                                              \
    apt-get full-upgrade -y --no-install-recommends &&                                             \
    apt-get autoremove -y --no-install-recommends &&                                               \
    apt-get autoclean -y --no-install-recommends

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get update &&                                                                              \
    apt-get install -y --no-install-recommends                                                     \
      ca-certificates curl gnupg jq software-properties-common

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    --mount=type=bind,src=external/infraCommons/tools,dst=/tmp/tools                               \
    bash /tmp/tools/apt/addGNUSources.sh    -y &&                                                  \
    bash /tmp/tools/apt/addLLVMSources.sh   -y &&                                                  \
    bash /tmp/tools/apt/addNvidiaSources.sh -y &&                                                  \
    bash /tmp/tools/installCMake.sh

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    --mount=type=bind,src=external/infraCommons/tools,dst=/tmp/tools                               \
    --mount=type=bind,src=systemDependencies.json,dst=/tmp/systemDependencies.json                 \
    apt-get update &&                                                                              \
    apt-get install -y --no-install-recommends                                                     \
      $(sh /tmp/tools/extractDependencies.sh "Basics Compilers" /tmp/systemDependencies.json)

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


# Nothing FROMs this stage as an image base — dev-base only pulls /opt/robotFarm and
# systemDependencies.txt forward via COPY/bind, so the source and build trees never get committed.
FROM base AS throw-away-dev-base
ARG ROBOTFARM_VERSION=v2.2.0
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

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get update &&                                                                              \
    apt-get install -y --no-install-recommends $(cat /tmp/robotFarm-build/systemDependencies.txt)

RUN cmake --build /tmp/robotFarm-build


FROM base AS dev-base
COPY --from=throw-away-dev-base /opt/robotFarm /opt/robotFarm

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get update &&                                                                              \
    apt-get install -y --no-install-recommends $(cat /opt/robotFarm/systemDependencies.txt)
