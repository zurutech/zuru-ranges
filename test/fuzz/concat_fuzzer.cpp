// libFuzzer harness for concat_view. Carves the input into four ranges (lengths drawn
// from the input, so empty ranges and runs of empties occur naturally), concatenates
// them (mixing Unreal and std containers, hence mixed iterator types), and checks the
// flattened result equals a straightforward concatenation. ASan/UBSan catch memory errors and
// UB; the equality check catches advance/sentinel logic errors.
//
// Built only with Clang (-fsanitize=fuzzer), in the debug iterator configuration.
#include "CoreMinimal.h"

#include "zr/ranges/views/concat_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // Four sub-ranges; each length is a control byte mod 5, so 0 (empty) is common.
    std::vector<std::vector<int>> reference(4);
    std::size_t pos = 0;
    for (auto& sub : reference) {
        const std::size_t len = (pos < size) ? (data[pos++] % 5u) : 0u;
        for (std::size_t k = 0; k < len && pos < size; ++k, ++pos) {
            sub.push_back(static_cast<int>(data[pos]));
        }
    }

    // Build matching containers, alternating Unreal and std (different iterator types).
    TArray<int> a;
    std::vector<int> b;
    TArray<int> c;
    std::vector<int> d;
    for (int x : reference[0]) {
        a.Add(x);
    }
    for (int x : reference[1]) {
        b.push_back(x);
    }
    for (int x : reference[2]) {
        c.Add(x);
    }
    for (int x : reference[3]) {
        d.push_back(x);
    }

    std::vector<int> got;
    for (int x : zr::ranges::views::concat(a, b, c, d)) {
        got.push_back(x);
    }

    std::vector<int> want;
    for (const auto& sub : reference) {
        for (int x : sub) {
            want.push_back(x);
        }
    }

    if (got != want) {
        std::abort(); // libFuzzer/sanitizer reports the crashing input
    }
    return 0;
}
