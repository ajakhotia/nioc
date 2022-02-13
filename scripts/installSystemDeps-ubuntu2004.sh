#!/bin/bash
####################################################################################################
# Copyright (c) 2022.                                                                              #
# Project  : Naksh                                                                                 #
# Author   : Anurag Jakhotia                                                                       #
####################################################################################################
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - || exit 1
echo "deb https://apt.llvm.org/focal/ llvm-toolchain-focal-14 main" >  /etc/apt/sources.list.d/clang-14.list || exit 1
add-apt-repository -y ppa:ubuntu-toolchain-r/test || exit 1



apt-get update && apt-get install -y    \
    clang-12                            \
    clang-14                            \
    clang-format-14                     \
    clang-tidy-14                       \
    g++-10                              \
    g++-11                              \
    gcc-10                              \
    gcc-11                              \
    || exit 1
