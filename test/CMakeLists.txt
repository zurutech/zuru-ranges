include(zr_flags)

# ---------------------------------------------------------------------------------------
# GoogleTest
# Prefer an installed copy; otherwise fetch a pinned release.
# ---------------------------------------------------------------------------------------
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )

    # Match the test build's CRT: dynamic by default, static under MSVC test builds (see the
    # CMAKE_MSVC_RUNTIME_LIBRARY note in the top-level CMakeLists) so gtest and the test
    # exes agree and the latter run self-contained under wine.
    if(MSVC)
        set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    else()
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    # googletest uses Win32 threads there; its pthread probes are failing links, and a failing
    # link.exe under wine 6 can hang the configure step.
    if(MSVC)
        set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(googletest)

    # googletest builds itself with -WX under every MSVC-style compiler, and its headers trip
    # newer clang-cl warnings (-Wcharacter-conversion, clang 21+): neither is ours to fix.
    foreach(target gtest gtest_main gmock gmock_main)
        if(TARGET ${target})
            if(MSVC)
                target_compile_options(${target} PRIVATE /WX-)
            endif()
            get_target_property(dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
            set_target_properties(${target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${dirs}")
        endif()
    endforeach()
endif()

# Behavioural UE fakes, no Epic source.
add_library(fake_unreal INTERFACE)
target_include_directories(fake_unreal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fake-unreal)

include(GoogleTest)

# Each test target is built three times: shipping (raw-pointer UE iterators), debug
# (checked-class iterators) and debug against the UE 5.3 to 5.6 mocks (fake-unreal/README.md).
set(ZR_CONFIGS shipping debug debug_ue503)

# Compile definitions of a test target for one entry of ZR_CONFIGS.
function(zr_config_definitions target config)
    if(config MATCHES "^debug")
        target_compile_definitions(${target} PRIVATE ZR_FAKE_UE_RANGED_FOR_CHECKS=1)
    endif()
    if(config STREQUAL debug_ue503)
        target_compile_definitions(${target} PRIVATE ZR_FAKE_UE_VERSION=503)
    endif()
endfunction()

# ---------------------------------------------------------------------------------------
# Compile-only checks
#
# Pure static_assert translation units (concept/trait classification). Built as OBJECT
# libraries and never run.
# ---------------------------------------------------------------------------------------

file(GLOB ZR_COMPILE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/compile/*.cpp)

foreach(config ${ZR_CONFIGS})
    set(target "zr_compile_checks_${config}")
    add_library(${target} OBJECT ${ZR_COMPILE_SOURCES})
    target_link_libraries(${target} PRIVATE zuru-ranges fake_unreal zr_flags)
    target_compile_definitions(${target} PRIVATE
        ZR_TEST_PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ZR_TEST_PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ZR_TEST_PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH})
    zr_config_definitions(${target} ${config})
endforeach()

# Every header compiles on its own: one generated translation unit per header.
file(GLOB_RECURSE ZR_HEADERS RELATIVE ${PROJECT_SOURCE_DIR}/include CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/include/zr/*.h)
set(ZR_HEADER_SOURCES)
foreach(header ${ZR_HEADERS})
    string(MAKE_C_IDENTIFIER ${header} id)
    set(source ${CMAKE_CURRENT_BINARY_DIR}/self_contained/${id}.cpp)
    if(header MATCHES "^zr/unreal/")
        set(content "#include \"CoreMinimal.h\"\n#include \"${header}\"\n")
    else()
        set(content "#include \"${header}\"\n")
    endif()
    # file(CONFIGURE) (CMake 3.18) rewrites only on change; cmakelang does not know the form.
    # cmake-lint: disable=E1126
    file(CONFIGURE OUTPUT ${source} CONTENT "${content}")
    list(APPEND ZR_HEADER_SOURCES ${source})
endforeach()
add_library(zr_self_contained_headers OBJECT ${ZR_HEADER_SOURCES})
target_link_libraries(zr_self_contained_headers PRIVATE zuru-ranges fake_unreal zr_flags)

# ---------------------------------------------------------------------------------------
# Runtime tests (GoogleTest)
# ---------------------------------------------------------------------------------------
file(GLOB ZR_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ranges/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithm/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/iterator/*.cpp)
# Clang < 16 substitutes constraints eagerly and cannot instantiate libstdc++'s range adaptors,
# and libc++ < 16 ships them as experimental (UE 5.3 to 5.5: clang 16/18 over libc++ 15):
# there is no std side to interoperate with. Same condition as ZR_TEST_STD_VIEWS in
# conformance.cpp.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles([[
#include <version>
#if defined(_LIBCPP_VERSION)
#if _LIBCPP_VERSION < 160000
#error libc++ < 16: experimental range adaptors
#endif
#elif defined(__clang__) && __clang_major__ < 16
#error clang < 16: cannot instantiate libstdc++'s range adaptors
#endif
int main() {}
]] ZR_HAS_STD_RANGE_ADAPTORS)
if(NOT ZR_HAS_STD_RANGE_ADAPTORS)
    list(FILTER ZR_TEST_SOURCES EXCLUDE REGEX "/std_interop\\.cpp$")
endif()
foreach(source ${ZR_TEST_SOURCES})
    get_filename_component(name ${source} NAME_WE)

    foreach(config ${ZR_CONFIGS})
        set(target "${name}_${config}")
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE zuru-ranges fake_unreal zr_flags GTest::gtest_main)

        zr_config_definitions(${target} ${config})

        # PRE_TEST discovery defers enumeration to ctest time, so it works under a
        # cross-compiling emulator (e.g. wine for the msvc-wine toolchain).
        gtest_discover_tests(${target} TEST_PREFIX "${config}." DISCOVERY_MODE PRE_TEST)
    endforeach()
endforeach()

if(ZR_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()
