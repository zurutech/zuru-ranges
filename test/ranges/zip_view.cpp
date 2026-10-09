// views::zip over heterogeneous containers and value types, both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

using zip_arr_vec = decltype(views::zip(std::declval<TArray<int>&>(), std::declval<std::vector<char>&>()));
using zip_arr_set = decltype(views::zip(std::declval<TArray<int>&>(), std::declval<TSet<int>&>()));
// All-random-access -> random_access (capped), sized, not contiguous.
static_assert(random_access_range<zip_arr_vec> && !contiguous_range<zip_arr_vec> && sized_range<zip_arr_vec>);
// A forward base drags zip down to forward.
static_assert(forward_range<zip_arr_set> && !random_access_range<zip_arr_set>);

} // namespace

TEST(ZipView, LockstepPairs) {
    TArray<int> a{1, 2, 3};
    std::vector<char> b{'a', 'b', 'c'};
    int sum = 0;
    for (auto&& [i, c] : views::zip(a, b)) {
        sum += i + static_cast<int>(c - 'a');
    }
    EXPECT_EQ(sum, 9); // (1+0)+(2+1)+(3+2)
}

TEST(ZipView, StopsAtShortest) {
    TArray<int> a{1, 2, 3};
    std::vector<int> longer{10, 20, 30, 40, 50};
    int sum = 0;
    int count = 0;
    for (auto&& [x, y] : views::zip(a, longer)) {
        sum += x + y;
        ++count;
    }
    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum, 66); // (1+10)+(2+20)+(3+30)
}

TEST(ZipView, SizeIsMinAndIndexable) {
    TArray<int> a{1, 2, 3};
    std::vector<char> b{'a', 'b', 'c'};
    auto z = views::zip(a, b);
    EXPECT_EQ(static_cast<int>(z.size()), 3);
    auto [zi, zc] = z[2];
    EXPECT_EQ(zi, 3);
    EXPECT_EQ(static_cast<int>(zc - 'a'), 2);
}

TEST(ZipView, MutateThroughZippedReferences) {
    TArray<int> a{1, 2, 3};
    std::vector<char> b{'a', 'b', 'c'};
    for (auto&& [i, c] : views::zip(a, b)) {
        i += 1;
    }
    EXPECT_EQ(a[0] + a[1] + a[2], 9); // 2+3+4
}
