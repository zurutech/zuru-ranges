# ctest driver (cmake -P): install zr from BUILD_DIR into a fresh prefix (or, with SOURCE_DIR,
# take the source tree through FetchContent), then configure, build and test this directory's
# consumer against it with the toolchain under test.
#
# -D: BUILD_DIR WORK_DIR GENERATOR ZR_VERSION ZR_MODULE WERROR CXX_COMPILER CXX_FLAGS
#     CXX_STANDARD BUILD_TYPE TOOLCHAIN_FILE MSVC_ROOT EMULATOR [SOURCE_DIR]

file(REMOVE_RECURSE "${WORK_DIR}")

set(args -S "${CMAKE_CURRENT_LIST_DIR}" -B "${WORK_DIR}/build" -G "${GENERATOR}"
    "-DZR_VERSION=${ZR_VERSION}"
    "-DZR_MODULE=${ZR_MODULE}"
    "-DZR_WARNINGS_AS_ERRORS=${WERROR}"
    "-DCMAKE_CXX_FLAGS=${CXX_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
# An empty CMAKE_CXX_STANDARD is an invalid CXX_STANDARD property: forward only a set one.
if(SOURCE_DIR)
    list(APPEND args "-DZR_SOURCE_DIR=${SOURCE_DIR}")
else()
    set(prefix "${WORK_DIR}/prefix")
    execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
        COMMAND_ERROR_IS_FATAL ANY)
    # zuru-ranges_DIR, not CMAKE_PREFIX_PATH: the wine toolchains confine package search to the
    # MSVC root (CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY). Globbed: the libdir is lib or lib64.
    file(GLOB_RECURSE config "${prefix}/*/zuru-rangesConfig.cmake")
    if(NOT config)
        message(FATAL_ERROR "no zuru-rangesConfig.cmake under ${prefix}")
    endif()
    cmake_path(GET config PARENT_PATH config_dir)
    list(APPEND args "-Dzuru-ranges_DIR=${config_dir}" "-DZR_INSTALL_PREFIX=${prefix}")
endif()
if(CXX_STANDARD)
    list(APPEND args "-DCMAKE_CXX_STANDARD=${CXX_STANDARD}")
endif()
if(TOOLCHAIN_FILE)
    list(APPEND args "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}" "-DZR_MSVC_ROOT=${MSVC_ROOT}")
else()
    list(APPEND args "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
endif()
if(EMULATOR)
    list(APPEND args "-DCMAKE_CROSSCOMPILING_EMULATOR=${EMULATOR}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" ${args} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${WORK_DIR}/build" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${WORK_DIR}/build" --output-on-failure
    COMMAND_ERROR_IS_FATAL ANY)
