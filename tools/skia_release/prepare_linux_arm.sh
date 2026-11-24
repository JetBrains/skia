#!/bin/bash
set -o errexit -o nounset -o pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update -y
apt-get install binutils build-essential software-properties-common -y

# add-apt-repository fetches the PPA signing key from a keyserver, which
# intermittently times out on CI runners (rate-limited runner IPs).
add_ppa() {
  local ppa="$1" attempt
  for attempt in 1 2 3 4 5; do
    if add-apt-repository "$ppa" -y; then
      return 0
    fi
    echo "add-apt-repository $ppa failed (attempt $attempt/5), retrying in 15s" >&2
    sleep 15
  done
  echo "add-apt-repository $ppa failed after 5 attempts" >&2
  return 1
}
add_ppa ppa:git-core/ppa
add_ppa ppa:ubuntu-toolchain-r/test
apt-get update -y
apt-get install git fontconfig libfontconfig1-dev libglu1-mesa-dev curl wget -y

apt-get install gcc-11 g++-11 -y
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60 --slave /usr/bin/g++ g++ /usr/bin/g++-11
update-alternatives --config gcc

apt-get install python3.9 -y
update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 100
