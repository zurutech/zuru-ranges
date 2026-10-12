#!/bin/sh
# Install pinned CMake and Ninja release binaries into /usr/local when the image lacks them
# or has a CMake older than this project needs. Used by the compiler-matrix jobs, whose
# images (gcc:N, distro clang) are chosen for the compiler, not the build tools.
# ZR_CMAKE_VERSION installs that CMake regardless (the 3.21 floor job).
set -eu

CMAKE_VERSION=${ZR_CMAKE_VERSION:-3.31.6}
NINJA_VERSION=1.12.1
need_cmake=1
if [ -z "${ZR_CMAKE_VERSION:-}" ] && command -v cmake >/dev/null 2>&1; then
    have=$(cmake --version | sed -n 's/^cmake version \([0-9]*\.[0-9]*\).*/\1/p')
    [ "$(printf '%s\n3.25\n' "$have" | sort -V | head -1)" = 3.25 ] && need_cmake=0
fi

fetch() { if command -v curl >/dev/null 2>&1; then curl -fsSL "$1"; else wget -qO- "$1"; fi; }

if [ "$need_cmake" = 1 ]; then
    fetch "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
        | tar -xz --strip-components=1 -C /usr/local
fi
if ! command -v ninja >/dev/null 2>&1; then
    if ! command -v python3 >/dev/null 2>&1 && ! command -v unzip >/dev/null 2>&1; then
        apt-get update -qq && apt-get install -y -qq --no-install-recommends unzip >/dev/null
    fi
    fetch "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip" > /tmp/ninja.zip
    python3 -c "import zipfile; zipfile.ZipFile('/tmp/ninja.zip').extract('ninja', '/usr/local/bin')" 2>/dev/null \
        || (cd /usr/local/bin && unzip -qo /tmp/ninja.zip)
    chmod +x /usr/local/bin/ninja
    rm -f /tmp/ninja.zip
fi
cmake --version | head -1
ninja --version
