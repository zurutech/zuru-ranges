# libFuzzer targets. Requires Clang (-fsanitize=fuzzer). fake_unreal and zr are
# defined by the parent (test/) and top-level CMakeLists.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "ZR_BUILD_FUZZERS=ON requires Clang; skipping fuzz targets.")
    return()
endif()

add_executable(concat_fuzzer concat_fuzzer.cpp)
target_link_libraries(concat_fuzzer PRIVATE zuru-ranges fake_unreal)
target_compile_definitions(concat_fuzzer PRIVATE ZR_FAKE_UE_RANGED_FOR_CHECKS=1)
target_compile_options(concat_fuzzer PRIVATE -g -fsanitize=fuzzer,address,undefined)
target_link_options(concat_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)

# A short, bounded run so CI exercises the fuzzer without an open-ended campaign.
add_test(NAME concat_fuzzer_smoke COMMAND concat_fuzzer -runs=50000 -max_len=128)
