# syntax=docker/dockerfile:1.7
ARG OS_BASE=ubuntu:22.04

FROM ${OS_BASE} AS base

ARG OS_BASE
ENV OS_BASE=${OS_BASE}
ENV APT_VAR_CACHE_ID=nioc-apt-var-cache-${OS_BASE}
ENV APT_LIST_CACHE_ID=nioc-apt-list-cache-${OS_BASE}
ENV DEBIAN_FRONTEND=noninteractive

# Set shell to return failure code if any command in the pipe fails.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN printf '%s\n'                                                                                   \
  'path-exclude /usr/share/doc/*'                                                                   \
  'path-exclude /usr/share/man/*'                                                                   \
  'path-include /usr/share/locale/locale.alias'                                                     \
  'path-include /usr/share/locale/en*/*'                                                            \
  'path-exclude /usr/share/locale/*'                                                                \
  'path-exclude /usr/share/info/*'                                                                  \
  > /etc/dpkg/dpkg.cfg.d/01_nodoc

RUN printf '%s\n'                                                                                   \
  'Acquire::http::Pipeline-Depth 0;'                                                                \
  'Acquire::http::No-Cache true;'                                                                   \
  'Acquire::BrokenProxy    true;'                                                                   \
  >> /etc/apt/apt.conf.d/90fix-hashsum-mismatch

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                  \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked             \
    apt-get update &&                                                                               \
    apt-get full-upgrade -y --no-install-recommends &&                                              \
    apt-get autoremove &&                                                                           \
    apt-get autoclean

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                  \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked             \
    apt-get update &&                                                                               \
    apt-get install -y --no-install-recommends jq

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                  \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked             \
    --mount=type=bind,src=tools/extractDependencies.sh,dst=/tmp/tools/extractDependencies.sh,ro     \
    --mount=type=bind,src=systemDependencies.json,dst=/tmp/systemDependencies.json,ro               \
    apt-get update &&                                                                               \
    apt-get install -y --no-install-recommends                                                      \
      $(sh /tmp/tools/extractDependencies.sh Basics /tmp/systemDependencies.json)


FROM base AS dev-base

ARG TOOLCHAIN=linux-gnu-default
ENV TOOLCHAIN=${TOOLCHAIN}
ARG ROBOTFARM_BUILD_LIST="BoostExternalProject;Eigen3ExternalProject;NlohmannJsonExternalProject;GoogleTestExternalProject;SpdLogExternalProject;CapnprotoExternalProject"

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                  \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked             \
    curl -fsSL                                                                                      \
      https://raw.githubusercontent.com/ajakhotia/robotFarm/refs/heads/main/tools/quickBuild.sh |   \
      bash -s --                                                                                    \
        --version v1.1.0                                                                            \
        --toolchain ${TOOLCHAIN}                                                                    \
        --prefix /opt/robotFarm                                                                     \
        --build-list ${ROBOTFARM_BUILD_LIST}

RUN --mount=type=cache,target=/var/cache/apt,id=${APT_VAR_CACHE_ID},sharing=locked                  \
    --mount=type=cache,target=/var/lib/apt/lists,id=${APT_LIST_CACHE_ID},sharing=locked             \
    --mount=type=bind,src=tools/extractDependencies.sh,dst=/tmp/tools/extractDependencies.sh,ro     \
    --mount=type=bind,src=systemDependencies.json,dst=/tmp/systemDependencies.json,ro               \
    apt-get update &&                                                                               \
    apt-get install -y --no-install-recommends                                                      \
      $(bash /tmp/tools/extractDependencies.sh Compilers /tmp/systemDependencies.json)


FROM dev-base AS build
ARG BUILD_TYPE="Release"
ENV BUILD_TYPE=${BUILD_TYPE}

RUN cmake -E make_directory /opt/nioc

RUN --mount=type=bind,src=.,dst=/tmp/nioc-src,ro                                                    \
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
