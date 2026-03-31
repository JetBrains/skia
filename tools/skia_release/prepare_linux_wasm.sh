#!/bin/bash
set -o errexit -o nounset -o pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update -y
apt-get install binutils build-essential -y
apt-get install software-properties-common -y
apt-get install git unzip curl wget pkg-config -y

apt-get install python3.9 -y
update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 100

apt-get install -y python3-pip
pip3 install "cmake==3.30.*" "ninja>=1.11.1"
