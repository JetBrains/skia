#!/bin/bash
set -o errexit -o nounset -o pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update -y
apt-get install binutils build-essential software-properties-common -y
add-apt-repository ppa:git-core/ppa -y
add-apt-repository ppa:ubuntu-toolchain-r/test -y
apt-get update -y
apt-get install git unzip fontconfig libfontconfig1-dev libglu1-mesa-dev curl wget pkg-config -y
apt-get install -y \
  libx11-xcb-dev \
  libxcb-dri2-0-dev \
  libxcb-dri3-dev \
  libxcb-present-dev \
  libxcb-sync-dev \
  libxcb-xfixes0-dev \
  libxrandr-dev \
  libxext-dev \
  libxi-dev

apt-get install gcc-12 g++-12 -y
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 60 --slave /usr/bin/g++ g++ /usr/bin/g++-12
update-alternatives --set gcc /usr/bin/gcc-12

apt-get install python3.9 -y
update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 100
