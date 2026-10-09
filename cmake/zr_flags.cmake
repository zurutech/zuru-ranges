# ---------------------------------------------------------------------------------------
# Shared compiler flags for zr's test/fuzz targets (the library itself is header-only
# and carries none).
#
# Reads the cache options defined in the top-level CMakeLists: ZR_WARNINGS_AS_ERRORS,
# ZR_ENABLE_SANITIZERS.
# ---------------------------------------------------------------------------------------

add_library(zr_flags INTERFACE)

if(MSVC)
    # MSVC is true for cl.exe and clang-cl; both take /-style flags. /permissive- is
    # conformance mode.
    target_compile_options(zr_flags INTERFACE /W4 /permissive-)
    if(ZR_WARNINGS_AS_ERRORS)
        target_compile_options(zr_flags INTERFACE /WX)
    endif()
else()
    target_compile_options(zr_flags INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    )

    # GCC's -Wdangling-reference false-positives on range-for over a pipeline temporary
    # (the adaptor returns a prvalue; nothing dangles). GCC-only.
    target_compile_options(zr_flags INTERFACE $<$<CXX_COMPILER_ID:GNU>:-Wno-dangling-reference>)

    if(ZR_WARNINGS_AS_ERRORS)
        target_compile_options(zr_flags INTERFACE -Werror)
        # GCC -O2+ flags the payload of a disengaged std::optional<T*> copied with its
        # iterator (join_view's inner_; GCC bug 80635). Kept as a warning, not an error.
        target_compile_options(zr_flags INTERFACE $<$<CXX_COMPILER_ID:GNU>:-Wno-error=maybe-uninitialized>)
    endif()
endif()

if(ZR_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(zr_flags INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
    )
    target_link_options(zr_flags INTERFACE -fsanitize=address,undefined)
endif()
