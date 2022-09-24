#!/bin/bash
####################################################################################################
# Copyright (c) 2022.                                                                              #
# Project  : nioc                                                                                 #
# Author   : Anurag Jakhotia                                                                       #
####################################################################################################

# Add repository for latest Clang toolchains.
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - || exit 1

echo "deb https://apt.llvm.org/focal/ llvm-toolchain-focal-14 main" >   \
    /etc/apt/sources.list.d/clang-14.list || exit 1


# Add repository for latest GNU toolchains.
add-apt-repository -y ppa:ubuntu-toolchain-r/test || exit 1


# Add repository for latest CMake.
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null |             \
    gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null || exit 1

echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] '     \
    'https://apt.kitware.com/ubuntu/ focal main' |                          \
    sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null || exit 1


# Install required apt packages.
apt-get update && apt-get install -y    \
    clang-12                            \
    clang-14                            \
    clang-format-14                     \
    clang-tidy-14                       \
    cmake                               \
    g++-10                              \
    g++-11                              \
    gcc-10                              \
    gcc-11                              \
    ninja-build                         \
    || exit 1
