#!/bin/sh
# Install clang-N (and libc++-N from 15 on) on Ubuntu, from the distro archive or else
# apt.llvm.org, plus g++ for libstdc++ and the build tools. Used by the Linux clang matrix:
#   ci/install-clang.sh 18        ->  scripts/make.sh -c clang++-18 [--libcxx]
# Ubuntu 22.04 carries clang 13-15, Ubuntu 24.04 clang 16 onwards.
set -eu

N=$1
export DEBIAN_FRONTEND=noninteractive
# shellcheck disable=SC1091
codename=$(. /etc/os-release && echo "$VERSION_CODENAME")

pkgs="clang-$N"
# libc++ 14 has no std::ranges.
[ "$N" -ge 15 ] && pkgs="$pkgs libc++-$N-dev libc++abi-$N-dev"

apt-get update -qq
apt-get install -y -qq --no-install-recommends ca-certificates curl gnupg git g++ python3 unzip >/dev/null
# shellcheck disable=SC2086 # $pkgs is a word list
if ! apt-get install -y -qq --no-install-recommends $pkgs >/dev/null 2>&1; then
    curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm.gpg
    echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/$codename/ llvm-toolchain-$codename-$N main" \
        > /etc/apt/sources.list.d/llvm.list
    apt-get update -qq
    # shellcheck disable=SC2086
    apt-get install -y -qq --no-install-recommends $pkgs >/dev/null
fi

"$(dirname "$0")/install-cmake-ninja.sh"
"clang++-$N" --version | head -1
