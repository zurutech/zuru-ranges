// iter_move moves and iter_swap swaps for zip, enumerate and join over std and Unreal
// containers, in both iterator configurations (debug exercises the relaxed Unreal swap).
#include "CoreMinimal.h"

#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/join_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
// The iterators are not assignable in the debug configuration: copy and ++ only.
template <class It> It advanced(It it, int n) {
    while (n-- > 0) {
        ++it;
    }
    return it;
}
} // namespace

TEST(IterSwap, ZipSwapsEveryColumnInLockstep) {
    TArray<int> keys{1, 2, 3}; // Unreal leg
    std::vector<std::string> vals{"a", "b", "c"}; // std leg
    auto z = views::zip(keys, vals);

    std::ranges::iter_swap(z.begin(), advanced(z.begin(), 2));

    EXPECT_EQ(keys[0], 3);
    EXPECT_EQ(keys[2], 1);
    EXPECT_EQ(vals[0], "c");
    EXPECT_EQ(vals[2], "a");
}

TEST(IterMove, ZipMovesElementOut) {
    std::vector<std::string> names{"hello", "world"};
    TArray<int> ids{7, 8};
    auto z = views::zip(names, ids);

    auto moved = std::ranges::iter_move(z.begin()); // tuple<std::string&&, int&&>
    static_assert(std::is_same_v<decltype(moved), std::tuple<std::string&&, int&&>>);

    std::string taken = std::get<0>(std::move(moved));
    EXPECT_EQ(taken, "hello"); // the element was moved out of names[0], not copied
}

TEST(IterMove, EnumerateKeepsIndexButMovesElement) {
    TArray<std::string> words{"alpha", "beta"};
    auto en = views::enumerate(words);

    auto t = std::ranges::iter_move(advanced(en.begin(), 1)); // tuple<difference_type, std::string&&>
    static_assert(std::is_same_v<decltype(t), std::tuple<std::ptrdiff_t, std::string&&>>);

    EXPECT_EQ(std::get<0>(t), 1); // index copied
    std::string taken = std::get<1>(std::move(t));
    EXPECT_EQ(taken, "beta"); // element moved
}

TEST(IterSwap, JoinSwapsAcrossInnerRanges) {
    TArray<TArray<int>> nested;
    nested.Add(TArray<int>{1, 2});
    nested.Add(TArray<int>{3, 4});
    auto j = views::join(nested); // flattens to 1, 2, 3, 4

    // element 0 is nested[0][0], element 2 is nested[1][0].
    std::ranges::iter_swap(j.begin(), advanced(j.begin(), 2));

    EXPECT_EQ(nested[0][0], 3);
    EXPECT_EQ(nested[1][0], 1);
}

TEST(IterMove, JoinMovesInnerElement) {
    std::vector<std::vector<std::string>> nested{{"x", "y"}, {"z"}};
    auto j = views::join(nested);

    std::string taken = std::ranges::iter_move(j.begin()); // std::string&&
    EXPECT_EQ(taken, "x");
}
