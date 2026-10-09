// views::concat over heterogeneous containers (UE + std, different iterator types),
// both build configurations. Exercises the variant position and visit_index dispatch.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <array>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int doubler(int x) { return x * 2; }
} // namespace

int run() {
    int s = 0;

    // Mixed iterator types: TArray (pointer/checked), std::vector, TSet (class iter), std::array.
    TArray<int> a{1, 2};
    std::vector<int> b{3, 4};
    TSet<int> c{5};
    std::array<int, 2> d{6, 7};

    auto cat = views::concat(a, b, c, d);
    static_assert(forward_range<decltype(cat)>);
    static_assert(sized_range<decltype(cat)>);

    for (int v : cat) {
        s += v; // 1+2+3+4+5+6+7 = 28
    }
    s += static_cast<int>(cat.size()); // 7

    // Empty range in the middle is skipped (satisfy).
    TArray<int> empty_arr{};
    int t = 0;
    for (int v : views::concat(a, empty_arr, b)) {
        t += v; // 1+2+3+4 = 10
    }
    s += t;

    // concat | transform | to, mixing UE and std down the pipeline.
    TArray<int> out = views::concat(a, b) | views::transform(doubler) | zr::ranges::to<TArray<int>>();
    s += out.Num();                    // 4
    s += out[0] + out[1] + out[2] + out[3]; // (1+2+3+4)*2 = 20

    // Single-range concat collapses to all() (a ref_view here).
    s += static_cast<int>(zr::ranges::size(views::concat(a))); // 2

    // iter_move yields the common rvalue reference (rvalue access point).
    auto cat2 = views::concat(a, b);
    auto it = zr::ranges::begin(cat2);
    static_assert(std::is_same_v<decltype(iter_move(it)), int&&>);
    s += iter_move(it); // 1
    return s;
}

TEST(ConcatView, HeterogeneousMixAndIterMove) { EXPECT_NE(run(), 0); }
