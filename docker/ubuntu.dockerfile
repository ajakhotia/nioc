# syntax=docker/dockerfile:1.7
ARG OS_BASE=ubuntu:22.04

FROM ${OS_BASE} AS base

ARG OS_BASE
ENV OS_BASE=${OS_BASE}
ENV APT_VAR_CACHE_ID=nioc-apt-var-cache-${OS_BASE}
ENV APT_LIST_CACHE_ID=nioc-apt-list-cache-${OS_BASE}
ENV DEBIAN_FRONTEND=noninteractive

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
    'Acquire::http::No-Cache true;'                                                                \
    'Acquire::BrokenProxy    true;'                                                                \
    >> /etc/apt/apt.conf.d/90fix-hashsum-mismatch

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get update &&                                                                              \
    apt-get full-upgrade -y --no-install-recommends &&                                             \
    apt-get autoremove -y --no-install-recommends &&                                               \
    apt-get autoclean -y --no-install-recommends

# Bootstrap: hardcoded so that systemDependencies.json edits don't invalidate the slow apt-source
# registration + Kitware CMake fetch below. jq is needed for extractDependencies.sh; the rest are
# the minimum set the apt-source scripts and installCMake.sh themselves require.
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
    apt-get install -y --no-install-recommends                                                     \
      $(sh /tmp/tools/extractDependencies.sh "Basics Compilers" /tmp/systemDependencies.json)


# Nothing FROMs this stage as an image base — dev-base only pulls /opt/robotFarm and
# systemDependencies.txt forward via COPY/bind, so the source and build trees never get committed.
FROM base AS throw-away-dev-base
ARG ROBOTFARM_VERSION=v2.2.0
ARG TOOLCHAIN=linux-gnu-15
ARG ROBOTFARM_BUILD_LIST="BoostExternalProject;Eigen3ExternalProject;NlohmannJsonExternalProject;GoogleTestExternalProject;SpdLogExternalProject;CapnprotoExternalProject"

# COPY (not bind-mount) so the toolchain path stays valid across RUNs. robotFarm's super-build
# re-runs cmake during ExternalProject sub-builds, and the path baked into CMakeCache.txt must
# resolve in the build RUN as well, not just configure.
COPY external/infraCommons/cmake/toolchains/${TOOLCHAIN}.cmake /tmp/${TOOLCHAIN}.cmake

RUN git clone --depth 1 --branch ${ROBOTFARM_VERSION}                                              \
      https://github.com/ajakhotia/robotFarm.git /tmp/robotFarm-src &&                             \
    git -C /tmp/robotFarm-src submodule update --init --depth 1

RUN cmake -G Ninja                                                                                 \
      -S /tmp/robotFarm-src                                                                        \
      -B /tmp/robotFarm-build                                                                      \
      -DCMAKE_BUILD_TYPE:STRING=Release                                                            \
      -DBUILD_SHARED_LIBS:BOOL=ON                                                                  \
      -DCMAKE_INSTALL_PREFIX:PATH=/opt/robotFarm                                                   \
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/tmp/${TOOLCHAIN}.cmake                                      \
      -DROBOT_FARM_REQUESTED_BUILD_LIST:STRING=${ROBOTFARM_BUILD_LIST}

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get install -y --no-install-recommends                                                     \
      $(cat /tmp/robotFarm-build/systemDependencies.txt)

RUN cmake --build /tmp/robotFarm-build


FROM base AS dev-base
ARG TOOLCHAIN=linux-gnu-15
ENV TOOLCHAIN=${TOOLCHAIN}

COPY --from=throw-away-dev-base /opt/robotFarm /opt/robotFarm

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                 \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked            \
    apt-get install -y --no-install-recommends $(cat /opt/robotFarm/systemDependencies.txt)


FROM dev-base AS build
ARG BUILD_TYPE="Release"
ENV BUILD_TYPE=${BUILD_TYPE}

RUN cmake -E make_directory /opt/nioc

RUN --mount=type=bind,src=.,dst=/tmp/nioc-src,ro                                                   \
    cmake -G Ninja                                                                                 \
      -S /tmp/nioc-src                                                                             \
      -B /tmp/nioc-build                                                                           \
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/tmp/nioc-src/external/infraCommons/cmake/toolchains/${TOOLCHAIN}.cmake \
      -DCMAKE_BUILD_TYPE:STRING=${BUILD_TYPE}                                                      \
      -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON                                                    \
      -DCMAKE_PREFIX_PATH:STRING=/opt/robotFarm                                                    \
      -DCMAKE_INSTALL_PREFIX:PATH=/opt/nioc &&                                                     \
    cmake --build /tmp/nioc-build &&                                                               \
    cmake --install /tmp/nioc-build


FROM build AS test
RUN ctest --test-dir /tmp/nioc-build --output-on-failure


FROM dev-base AS deploy
COPY --from=build /opt/nioc /opt/nioc
