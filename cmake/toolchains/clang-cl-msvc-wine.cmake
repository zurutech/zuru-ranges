# ---------------------------------------------------------------------------------------
# clang-cl cross toolchain: Linux-native clang-cl and lld-link against an msvc-wine install
# (MSVC STL + Windows SDK, default /opt/msvc). Compiling needs no wine; ctest runs the .exe
# tests through CMAKE_CROSSCOMPILING_EMULATOR (pass -DCMAKE_CROSSCOMPILING_EMULATOR=wine).
#
#   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-cl-msvc-wine.cmake
#
# MSVC and SDK versions: ZR_MSVC_VERSION / ZR_WINSDK_VERSION if set, else those of msvc-wine's
# cl.exe (bin/x64/msvcenv.sh), else the newest installed.
# ---------------------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT ZR_MSVC_ROOT)
    set(ZR_MSVC_ROOT "$ENV{ZR_MSVC_ROOT}")
endif()
if(NOT ZR_MSVC_ROOT)
    set(ZR_MSVC_ROOT /opt/msvc)
endif()
# Try-compile projects re-read this file; forward the inputs to them.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ZR_MSVC_ROOT ZR_MSVC_VERSION ZR_WINSDK_VERSION)

# Set `out` to the newest (natural sort) subdirectory name of `dir`.
macro(zr_newest_subdir out dir)
    file(GLOB _zr_candidates LIST_DIRECTORIES true RELATIVE "${dir}" "${dir}/*")
    list(SORT _zr_candidates COMPARE NATURAL ORDER DESCENDING)
    list(GET _zr_candidates 0 ${out})
endmacro()

# Default to the versions msvc-wine's own cl.exe wrapper uses, so both compilers see one STL.
set(_zr_msvcenv "${ZR_MSVC_ROOT}/bin/x64/msvcenv.sh")
if(EXISTS "${_zr_msvcenv}")
    file(STRINGS "${_zr_msvcenv}" _zr_msvcver REGEX "^MSVCVER=")
    file(STRINGS "${_zr_msvcenv}" _zr_sdkver REGEX "^SDKVER=")
    if(NOT ZR_MSVC_VERSION AND _zr_msvcver)
        string(REGEX REPLACE "^MSVCVER=" "" ZR_MSVC_VERSION "${_zr_msvcver}")
    endif()
    if(NOT ZR_WINSDK_VERSION AND _zr_sdkver)
        string(REGEX REPLACE "^SDKVER=" "" ZR_WINSDK_VERSION "${_zr_sdkver}")
    endif()
endif()
if(NOT ZR_MSVC_VERSION)
    zr_newest_subdir(ZR_MSVC_VERSION "${ZR_MSVC_ROOT}/vc/tools/msvc")
endif()
if(NOT ZR_WINSDK_VERSION)
    zr_newest_subdir(ZR_WINSDK_VERSION "${ZR_MSVC_ROOT}/kits/10/include")
endif()

set(_zr_msvc "${ZR_MSVC_ROOT}/vc/tools/msvc/${ZR_MSVC_VERSION}")
set(_zr_sdk_inc "${ZR_MSVC_ROOT}/kits/10/include/${ZR_WINSDK_VERSION}")
set(_zr_sdk_lib "${ZR_MSVC_ROOT}/kits/10/lib/${ZR_WINSDK_VERSION}")

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)

set(_zr_target --target=x86_64-pc-windows-msvc)
foreach(_zr_dir "${_zr_msvc}/include" "${_zr_sdk_inc}/ucrt" "${_zr_sdk_inc}/shared"
                 "${_zr_sdk_inc}/um" "${_zr_sdk_inc}/winrt")
    string(APPEND _zr_imsvc " /imsvc \"${_zr_dir}\"")
endforeach()
set(CMAKE_C_FLAGS_INIT "${_zr_target}${_zr_imsvc}")
set(CMAKE_CXX_FLAGS_INIT "${_zr_target}${_zr_imsvc}")

foreach(_zr_dir "${_zr_msvc}/lib/x64" "${_zr_sdk_lib}/ucrt/x64" "${_zr_sdk_lib}/um/x64")
    string(APPEND _zr_libpath " /libpath:\"${_zr_dir}\"")
endforeach()
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_zr_libpath}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_zr_libpath}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_zr_libpath}")

# Keep find_package away from the host (a Linux GTest would otherwise be picked).
set(CMAKE_FIND_ROOT_PATH "${ZR_MSVC_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
