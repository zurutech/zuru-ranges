# ---------------------------------------------------------------------------------------
# MSVC cl.exe cross toolchain through an msvc-wine install (default /opt/msvc): its bin/x64
# wrappers run cl/link/lib/rc under wine with the install's INCLUDE/LIB. Pass
# -DCMAKE_CROSSCOMPILING_EMULATOR=wine for ctest to run the .exe tests.
#
#   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/msvc-wine.cmake
# ---------------------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT ZR_MSVC_ROOT)
    set(ZR_MSVC_ROOT "$ENV{ZR_MSVC_ROOT}")
endif()
if(NOT ZR_MSVC_ROOT)
    set(ZR_MSVC_ROOT /opt/msvc)
endif()
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ZR_MSVC_ROOT)

set(CMAKE_C_COMPILER "${ZR_MSVC_ROOT}/bin/x64/cl")
set(CMAKE_CXX_COMPILER "${ZR_MSVC_ROOT}/bin/x64/cl")
set(CMAKE_LINKER "${ZR_MSVC_ROOT}/bin/x64/link")
set(CMAKE_AR "${ZR_MSVC_ROOT}/bin/x64/lib")
set(CMAKE_RC_COMPILER "${ZR_MSVC_ROOT}/bin/x64/rc")
set(CMAKE_MT "${ZR_MSVC_ROOT}/bin/x64/mt")

# Embedded debug info (/Z7), not /Zi: /Zi needs mspdbsrv.exe, which Ubuntu's wine 9 cannot run
# (C1902 "Program database manager mismatch"). Honoured under CMP0141 (CMake >= 3.25).
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

# No incremental linking: each /INCREMENTAL link.exe under wine maps an .ilk of tens of MB and
# parallel links exhaust a CI container's memory (LNK1104/LNK1102). Cache entries set here
# take precedence over the platform defaults ("/debug /INCREMENTAL").
foreach(_zr_kind EXE SHARED MODULE)
    set(CMAKE_${_zr_kind}_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO" CACHE STRING "")
    set(CMAKE_${_zr_kind}_LINKER_FLAGS_RELWITHDEBINFO "/debug /INCREMENTAL:NO" CACHE STRING "")
endforeach()

# Keep find_package away from the host (a Linux GTest would otherwise be picked).
set(CMAKE_FIND_ROOT_PATH "${ZR_MSVC_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
