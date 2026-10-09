# zuru-ranges: header-only C++20 ranges library, no Unreal dependency.

cmake_minimum_required(VERSION 3.21...3.31)
project(zuru-ranges
    VERSION 0.1.0
    DESCRIPTION "C++20 ranges for Unreal and standard containers"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------------------
# Options (tests default ON only when zr is the top-level project).
# ---------------------------------------------------------------------------------------
option(ZR_BUILD_TESTS "Build zr tests" ${PROJECT_IS_TOP_LEVEL})
option(ZR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(ZR_ENABLE_SANITIZERS "Build tests with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(ZR_BUILD_FUZZERS "Build libFuzzer targets (Clang only)" OFF)
option(ZR_MODULE "Build the `import zr;` module tests" OFF)
# CMP0155 (CMake >= 3.28) scans every C++20 source for module imports, which needs
# clang-scan-deps or GCC 14+; only the module build has any.
if(NOT ZR_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

# ---------------------------------------------------------------------------------------
# The library: header-only. Generator expressions let it be both built-against in-tree
# and installed.
# ---------------------------------------------------------------------------------------
add_library(zuru-ranges INTERFACE)
add_library(zuru-ranges::zuru-ranges ALIAS zuru-ranges)

target_include_directories(zuru-ranges INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(zuru-ranges INTERFACE cxx_std_20)

# ---------------------------------------------------------------------------------------
# Optional C++20 named-module interface (`import zr;`).
#
# A C++ module BMI is tied to the exact compiler and language level it was built with, and
# CMake cannot attach a CXX_MODULES file set to an INTERFACE (header-only) target. So zr
# does not precompile the module; instead it exposes the interface units and a helper that
# adds them to the CONSUMER's own CXX_MODULES file set, where they are compiled with the
# consumer's flags (the portable approach: a C++20 zr works inside a C++23 consumer).
# The units include the plain headers in the global module fragment and re-export names, so
# #include and import can be mixed in one program (see modules/zr.cppm).
#
#   zr_add_modules(<target> [visibility])   # default visibility PRIVATE
#   zr_add_modules(app)   -> `import zr;` usable in app's own TUs
#
# Defined by cmake/zr_modules.cmake, here and in the installed package (find_package(zuru-ranges)).
# ZR_MODULE_SOURCES is also exported for consumers that prefer to wire the file set by hand.
# ---------------------------------------------------------------------------------------
set(_zr_module_dir ${CMAKE_CURRENT_SOURCE_DIR}/modules)
include(zr_modules)
unset(_zr_module_dir)
get_property(_zr_module_sources GLOBAL PROPERTY ZR_MODULE_SOURCES)
set(ZR_MODULE_SOURCES ${_zr_module_sources}
    CACHE INTERNAL "zr C++20 module interface units (add to a consumer's CXX_MODULES file set)"
)
unset(_zr_module_sources)

if(ZR_BUILD_TESTS)
    # Static CRT for the (top-level) test build so MSVC test executables are
    # self-contained, e.g. runnable under wine without the CRT
    # redistributable DLLs. Top-level builds only.
    if(MSVC AND PROJECT_IS_TOP_LEVEL)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    enable_testing()
    add_subdirectory(test)

    # Module smoke test: builds only when the named module is enabled. Uses a
    # symbol from each exported header through `import zr;` so a name missing from the
    # export list fails the build, and links against a header-mode TU. No GTest.
    if(ZR_MODULE)
        add_executable(zr_module_smoke test/module_smoke.cpp test/module_mix_header.cpp)
        zr_add_modules(zr_module_smoke)
        add_test(NAME zr_module_smoke COMMAND zr_module_smoke)
    endif()

    # Installed package: install this build into a fresh prefix and build test/install
    # against it through find_package(zuru-ranges), with this build's toolchain; with ZR_MODULE
    # also zr_add_modules() over the installed units.
    set(_zr_install_test_args
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DGENERATOR=${CMAKE_GENERATOR}
        -DZR_VERSION=${PROJECT_VERSION}
        -DWERROR=${ZR_WARNINGS_AS_ERRORS}
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DBUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DTOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DMSVC_ROOT=${ZR_MSVC_ROOT}
        -DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}
    )
    add_test(NAME zr_install_consumer COMMAND ${CMAKE_COMMAND} ${_zr_install_test_args}
        -DWORK_DIR=${CMAKE_BINARY_DIR}/install-test -DZR_MODULE=OFF
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test/install/run.cmake)
    if(ZR_MODULE)
        add_test(NAME zr_module_smoke_installed COMMAND ${CMAKE_COMMAND} ${_zr_install_test_args}
            -DWORK_DIR=${CMAKE_BINARY_DIR}/install-test-module -DZR_MODULE=ON
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test/install/run.cmake)
    endif()
    unset(_zr_install_test_args)
endif()

# ---------------------------------------------------------------------------------------
# Install: copy the headers and export a package so find_package(zuru-ranges) works.
# ---------------------------------------------------------------------------------------
include(GNUInstallDirs)

install(DIRECTORY include/zr DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS zuru-ranges EXPORT zuru-rangesTargets)
# The module interface units are sources the consumer compiles, so ship them alongside the
# headers; the installed zr_add_modules() points at them.
set(ZR_INSTALL_MODULEDIR ${CMAKE_INSTALL_INCLUDEDIR}/zr-modules)
set(ZR_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/zuru-ranges)
install(DIRECTORY modules/ DESTINATION ${ZR_INSTALL_MODULEDIR})
install(EXPORT zuru-rangesTargets
    NAMESPACE zuru-ranges::
    DESTINATION ${ZR_INSTALL_CMAKEDIR}
)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/zuru-rangesConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zuru-rangesConfig.cmake
    INSTALL_DESTINATION ${ZR_INSTALL_CMAKEDIR}
    PATH_VARS ZR_INSTALL_MODULEDIR
)
# find_package(zuru-ranges <version>) needs a version file. Before 1.0 a minor bump may break the
# API, so only the same minor version is compatible; header-only, so any architecture.
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/zuru-rangesConfigVersion.cmake
    COMPATIBILITY SameMinorVersion
    ARCH_INDEPENDENT
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zuru-rangesConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/zuru-rangesConfigVersion.cmake
    cmake/zr_modules.cmake
    DESTINATION ${ZR_INSTALL_CMAKEDIR}
)
