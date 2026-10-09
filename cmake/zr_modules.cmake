# ---------------------------------------------------------------------------------------
# zr_add_modules(<target> [visibility]): add zr's module interface units to a consumer's
# CXX_MODULES file set (see the module notes in the top-level CMakeLists).
#
# Shared by the source tree (add_subdirectory / FetchContent) and the installed package
# (zuru-rangesConfig.cmake). The includer sets _zr_module_dir to the directory holding the .cppm
# units. The first copy included wins: a second zr in one build reuses its function and
# units, so one program never compiles two `zr` modules.
# ---------------------------------------------------------------------------------------

if(COMMAND zr_add_modules)
    return()
endif()

# Global properties, not variables: the function runs in the consumer's scope, which need
# not see the includer's variables (a find_package in a sibling directory).
set_property(GLOBAL PROPERTY ZR_MODULE_BASE_DIR "${_zr_module_dir}")
set_property(GLOBAL PROPERTY ZR_MODULE_SOURCES
    "${_zr_module_dir}/zr.cppm"
)

# cmake-lint: disable=C0111
function(zr_add_modules target)
    set(visibility "PRIVATE")
    if(ARGC GREATER 1)
        set(visibility "${ARGV1}")
    endif()
    get_property(base_dir GLOBAL PROPERTY ZR_MODULE_BASE_DIR)
    get_property(sources GLOBAL PROPERTY ZR_MODULE_SOURCES)
    target_sources(${target}
        ${visibility} FILE_SET zr_modules TYPE CXX_MODULES
        BASE_DIRS ${base_dir}
        FILES ${sources}
    )
    # The module units #include the zr headers to compile; import consumers do not
    # need the headers, so the include dependency is PRIVATE to the compiling target.
    target_link_libraries(${target} PRIVATE zuru-ranges::zuru-ranges)
    # Scan the target's own sources for `import zr;`. CMake enables this by default
    # only under policy CMP0155 (NEW when the project requires CMake >= 3.28); force it
    # so a consumer whose project sets an older minimum still resolves the import.
    set_target_properties(${target} PROPERTIES CXX_SCAN_FOR_MODULES ON)
    # zr is a SYSTEM-style dependency: compile its interface units warning-free so a
    # strict consumer (-Werror, -Wconversion, ...) is not broken by zr's own code.
    # zr holds these sources to its own warning bar in its own CI.
    if(MSVC)
        set(nowarn "/w")
    else()
        set(nowarn "-w")
    endif()
    set_source_files_properties(${sources} TARGET_DIRECTORY ${target}
        PROPERTIES COMPILE_OPTIONS "${nowarn}")
endfunction()
