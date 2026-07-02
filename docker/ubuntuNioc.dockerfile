# syntax=docker/dockerfile:1.7
ARG DEV_BASE_IMAGE=ghcr.io/ajakhotia/nioc/ubuntu-24-04/dev-base:latest

FROM ${DEV_BASE_IMAGE} AS dev-base


FROM dev-base AS build
ARG TOOLCHAIN=linux-gnu-15
ENV TOOLCHAIN=${TOOLCHAIN}

ARG BUILD_TYPE="Release"
ENV BUILD_TYPE=${BUILD_TYPE}

RUN cmake -E make_directory /opt/nioc

RUN --mount=type=bind,src=.,dst=/tmp/nioc-src,ro                                                              \
    cmake -G Ninja                                                                                            \
      -S /tmp/nioc-src                                                                                        \
      -B /tmp/nioc-build                                                                                      \
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/tmp/nioc-src/external/infraCommons/cmake/toolchains/${TOOLCHAIN}.cmake \
      -DCMAKE_BUILD_TYPE:STRING=${BUILD_TYPE}                                                                 \
      -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON                                                               \
      -DCMAKE_PREFIX_PATH:STRING=/opt/robotFarm                                                               \
      -DCMAKE_INSTALL_PREFIX:PATH=/opt/nioc &&                                                                \
    cmake --build /tmp/nioc-build &&                                                                          \
    cmake --install /tmp/nioc-build


FROM build AS test
RUN ctest --test-dir /tmp/nioc-build --output-on-failure


FROM dev-base AS deploy
COPY --from=build /opt/nioc /opt/nioc
