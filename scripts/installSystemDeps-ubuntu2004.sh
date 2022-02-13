#!/bin/bash
####################################################################################################
# Copyright (c) 2022.                                                                              #
# Project  : Naksh                                                                                 #
# Author   : Anurag Jakhotia                                                                       #
####################################################################################################
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
echo "deb https://apt.llvm.org/focal/ llvm-toolchain-focal-14 main" >  /etc/apt/sources.list.d/clang-14.list

apt-get update
apt-get install -y      \
    clang-14            \
    clang-format-14
