#!/bin/bash
set -o errexit -o nounset -o pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update -y
apt-get install binutils build-essential -y
apt-get install software-properties-common -y
apt-get install git curl wget -y

# Install CMake 3.28 (Dawn requires >= 3.22, Ubuntu 20.04 only has 3.16)
CMAKE_VERSION=3.28.6
wget -q "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" -O /tmp/cmake.tar.gz
tar xf /tmp/cmake.tar.gz --strip-components=1 -C /usr/local
rm /tmp/cmake.tar.gz

apt-get install python3.9 -y
update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 100

# Install wasi-sdk
WASI_SDK_VERSION=33
WASI_SDK_DIR=/opt/wasi-sdk
wget -q "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}.0-x86_64-linux.tar.gz" -O /tmp/wasi-sdk.tar.gz
mkdir -p "${WASI_SDK_DIR}"
tar xf /tmp/wasi-sdk.tar.gz --strip-components=1 -C "${WASI_SDK_DIR}"
rm /tmp/wasi-sdk.tar.gz
