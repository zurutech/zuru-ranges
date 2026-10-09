# find_package(zuru-ranges) consumer of an installed zr; run.cmake drives it (ctest
# zr_install_consumer, zr_module_smoke_installed).

cmake_minimum_required(VERSION 3.21...3.31)
project(zr_install_consumer LANGUAGES CXX)

option(ZR_MODULE "Also consume the named module through zr_add_modules()" OFF)
option(ZR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(NOT ZR_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()
# Self-contained executables under wine, as zr's own test build.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(zuru-ranges ${ZR_VERSION} EXACT REQUIRED CONFIG)

# Everything must come from the prefix: a path back into the source tree would hide a file
# missing from the install.
get_target_property(include_dirs zuru-ranges::zuru-ranges INTERFACE_INCLUDE_DIRECTORIES)
foreach(path ${include_dirs} ${ZR_MODULE_SOURCES})
    cmake_path(IS_PREFIX ZR_INSTALL_PREFIX "${path}" NORMALIZE in_prefix)
    if(NOT in_prefix)
        message(FATAL_ERROR "zr package path outside ${ZR_INSTALL_PREFIX}: ${path}")
    endif()
endforeach()
if(NOT ZR_MODULE_SOURCES OR NOT COMMAND zr_add_modules)
    message(FATAL_ERROR "zuru-rangesConfig.cmake did not provide zr_add_modules / ZR_MODULE_SOURCES")
endif()

if(MSVC)
    set(warnings /W4 $<$<BOOL:${ZR_WARNINGS_AS_ERRORS}>:/WX>)
else()
    set(warnings -Wall -Wextra -Wpedantic $<$<BOOL:${ZR_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

enable_testing()

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE zuru-ranges::zuru-ranges)
target_compile_options(consumer PRIVATE ${warnings})
add_test(NAME consumer COMMAND consumer)

if(ZR_MODULE)
    add_executable(module_consumer ../module_smoke.cpp ../module_mix_header.cpp)
    # No ${warnings}, as the in-tree zr_module_smoke: Clang 18 drops the system-header status
    # of the global module fragment, so an importer is warned about libstdc++ internals
    # (std::reverse_iterator's deprecated std::iterator base).
    zr_add_modules(module_consumer)
    add_test(NAME module_consumer COMMAND module_consumer)
endif()
