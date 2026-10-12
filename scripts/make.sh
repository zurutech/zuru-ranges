#!/usr/bin/env bash
# Configure, build and test zr. Run from anywhere; resolves the project root itself.
# The GCC, Clang, Apple Clang and module-gcc workflows run this script.
#
# Usage: scripts/make.sh [options]
#   -c, --compiler <id>  g++ (default) | clang++ | <any C++ compiler> | msvc | clang-cl
#   -W, --werror         Treat compiler warnings as errors
#   -s, --sanitizers     AddressSanitizer + UndefinedBehaviorSanitizer (clang/gcc)
#   -f, --fuzzers        Build libFuzzer targets and run only their smoke test (forces clang++)
#   -m, --module         Build and run only the `import zr;` smoke tests (ZR_MODULE=ON)
#   -l, --libcxx         Clang with libc++ instead of the platform standard library
#       --libcxx-root <d>
#                        libc++ of another LLVM install (<d>/include/c++/v1, <d>/lib): UE 5.3
#                        to 5.5 build with clang 16/18 against libc++ 15. Implies --libcxx
#       --std <n>        C++ standard (default 20)
#       --config <type>  CMAKE_BUILD_TYPE: Debug (default) | Release | RelWithDebInfo
#   -H, --hardened       Standard library assertions: _GLIBCXX_ASSERTIONS (libstdc++),
#                        _LIBCPP_HARDENING_MODE_DEBUG (libc++ 18+)
#   -t, --no-test        Configure and build only; do not run ctest
#   -B, --build-dir <d>  Build directory (default: build)
#       --junit <file>   Also write a JUnit report of the ctest run
#       --clean          Remove the build directory first
#   -h, --help           Show this help
#
# Environment: ZR_BUILD_JOBS / ZR_CTEST_JOBS override the parallelism (default: CPUs, and
# under wine also available memory / 768 MB).
#
# msvc (cl.exe) and clang-cl cross-compile against the msvc-wine install in /opt/msvc
# (ZR_MSVC_ROOT overrides it) and run the tests under wine. CMake cannot switch toolchains
# in an existing build directory: change compiler with --clean or another --build-dir.

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Absolute path without requiring it to exist (BSD realpath has no -m).
abspath() { [[ "$1" = /* ]] && echo "$1" || echo "${PWD}/$1"; }

compiler="g++"
werror="OFF"
sanitizers="OFF"
fuzzers="OFF"
module="OFF"
libcxx=0
libcxx_root=""
std=20
config=Debug
hardened=0
run_tests=1
clean=0
build="${root}/build"
junit=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--compiler) compiler="$2"; shift 2 ;;
        -W|--werror) werror="ON"; shift ;;
        -s|--sanitizers) sanitizers="ON"; shift ;;
        -f|--fuzzers) fuzzers="ON"; compiler="clang++"; shift ;;
        -m|--module) module="ON"; shift ;;
        -l|--libcxx) libcxx=1; shift ;;
        --libcxx-root) libcxx=1; libcxx_root="$(abspath "$2")"; shift 2 ;;
        --std) std="$2"; shift 2 ;;
        --config) config="$2"; shift 2 ;;
        -H|--hardened) hardened=1; shift ;;
        -t|--no-test) run_tests=0; shift ;;
        -B|--build-dir) build="$(abspath "$2")"; shift 2 ;;
        --junit) junit="$(abspath "$2")"; shift 2 ;;
        --clean) clean=1; shift ;;
        -h|--help) sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

[[ $clean -eq 1 ]] && rm -rf "${build}"

msvc_root="${ZR_MSVC_ROOT:-/opt/msvc}"
declare -a args=(-S "${root}" -B "${build}" -G Ninja
    "-DZR_WARNINGS_AS_ERRORS=${werror}"
    "-DZR_ENABLE_SANITIZERS=${sanitizers}"
    "-DZR_BUILD_FUZZERS=${fuzzers}"
    "-DZR_MODULE=${module}"
    "-DCMAKE_CXX_STANDARD=${std}"
    "-DCMAKE_BUILD_TYPE=${config}")
# CMAKE_CXX_FLAGS also reach the link line of C++ executables. A system GoogleTest is built
# against the platform standard library in its default mode: fetch one for libc++ or hardening.
cxxflags=""
if [[ -n "${libcxx_root}" ]]; then
    # -nostdinc++/-nostdlib++ in place of -stdlib=libc++, which would go unused (-Werror) at
    # compile time; the C compiler (GoogleTest's project, possibly gcc) sees none of it.
    cxxflags+=" -nostdinc++ -isystem ${libcxx_root}/include/c++/v1"
    args+=("-DCMAKE_EXE_LINKER_FLAGS=-L${libcxx_root}/lib -Wl,-rpath,${libcxx_root}/lib"
           "-DCMAKE_CXX_STANDARD_LIBRARIES=-nostdlib++ -lc++ -lc++abi"
           -DCMAKE_DISABLE_FIND_PACKAGE_GTest=ON)
elif [[ $libcxx -eq 1 ]]; then
    cxxflags+=" -stdlib=libc++"
    args+=(-DCMAKE_DISABLE_FIND_PACKAGE_GTest=ON)
fi
if [[ $hardened -eq 1 ]]; then
    cxxflags+=" -D_GLIBCXX_ASSERTIONS -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG"
    args+=(-DCMAKE_DISABLE_FIND_PACKAGE_GTest=ON)
fi
[[ -n "${cxxflags}" ]] && args+=("-DCMAKE_CXX_FLAGS=${cxxflags# }")

wine=0
case "${compiler}" in
    msvc)
        args+=("-DCMAKE_TOOLCHAIN_FILE=${root}/cmake/toolchains/msvc-wine.cmake" "-DZR_MSVC_ROOT=${msvc_root}"
               -DCMAKE_CROSSCOMPILING_EMULATOR=wine)
        wine=1 ;;
    clang-cl)
        args+=("-DCMAKE_TOOLCHAIN_FILE=${root}/cmake/toolchains/clang-cl-msvc-wine.cmake" "-DZR_MSVC_ROOT=${msvc_root}"
               -DCMAKE_CROSSCOMPILING_EMULATOR=wine)
        wine=1 ;;
    *)
        args+=("-DCMAKE_CXX_COMPILER=${compiler}") ;;
esac

if [[ $wine -eq 1 ]]; then
    export WINEDEBUG="${WINEDEBUG:--all}"
    # One persistent wineserver for the whole run: a fresh one per test costs seconds each.
    # Boot the prefix here, detached: its services otherwise start inside the first test and
    # inherit that test's stdout, so ctest waits on it forever. Nothing wine starts may hold
    # this script's stdout, or whoever reads it waits on the open pipe after the script exits.
    # The wineserver holds every wine process's handles as file descriptors: at a container's
    # default soft limit (1024) parallel cl.exe/link.exe fail to open files (Windows error 4).
    ulimit -n "$(ulimit -Hn)" 2>/dev/null || true
    wineserver -p </dev/null >/dev/null 2>&1
    trap 'wineserver -k </dev/null >/dev/null 2>&1 || true' EXIT
    # No crash debugger: winedbg would attach to a crashed process and keep it alive.
    wine reg add 'HKLM\Software\Microsoft\Windows NT\CurrentVersion\AeDebug' /v Debugger /t REG_SZ /d '' /f \
        </dev/null >/dev/null 2>&1 || true
fi

cmake "${args[@]}"

# Parallelism: the CPUs, and under wine also the memory limit (~768 MB per cl.exe, link.exe
# or test process). A container sees every host CPU but only its cgroup's memory, so CPUs
# alone overcommit it and wine reports the failed mappings as LNK1104/LNK1181/LNK1102.
jobs="$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
if [[ $wine -eq 1 ]]; then
    mem=$(( $(awk '/^MemAvailable:/ {print $2}' /proc/meminfo) * 1024 ))
    limit="$(cat /sys/fs/cgroup/memory.max 2>/dev/null || cat /sys/fs/cgroup/memory/memory.limit_in_bytes 2>/dev/null || echo max)"
    [[ "${limit}" =~ ^[0-9]+$ ]] && (( limit < mem )) && mem=${limit}
    mem_jobs=$(( mem / (768 * 1024 * 1024) ))
    (( mem_jobs < 1 )) && mem_jobs=1
    (( mem_jobs < jobs )) && jobs=${mem_jobs}
fi
jobs="${ZR_BUILD_JOBS:-${jobs}}"
echo "make.sh: ${jobs} parallel jobs"

declare -a ctest_args=(--test-dir "${build}" --output-on-failure --timeout 300 -j "${ZR_CTEST_JOBS:-${jobs}}")
[[ -n "${junit}" ]] && ctest_args+=(--output-junit "${junit}")
# A wine process occasionally dies at startup; the tests are deterministic, so one retry.
[[ $wine -eq 1 ]] && ctest_args+=(--repeat until-pass:2)

if [[ "${module}" == "ON" ]]; then
    cmake --build "${build}" -j "${jobs}" --target zr_module_smoke
    ctest_args+=(-R '^zr_module_smoke(_installed|_subdirectory)?$')
elif [[ "${fuzzers}" == "ON" ]]; then
    cmake --build "${build}" -j "${jobs}"
    ctest_args+=(-R concat_fuzzer_smoke)
else
    cmake --build "${build}" -j "${jobs}"
fi

if [[ $run_tests -eq 1 ]]; then
    ctest "${ctest_args[@]}"
fi
