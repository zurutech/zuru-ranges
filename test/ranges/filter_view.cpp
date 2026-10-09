// filter_view (stateless) over contiguous / forward / std bases, both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr bool is_even(int x) { return x % 2 == 0; }
constexpr int doubler(int x) { return x * 2; }

using arr_fv = decltype(views::filter(std::declval<TArray<int>&>(), is_even));
// filter caps at forward; never sized / random-access / contiguous.
static_assert(forward_range<arr_fv> && !random_access_range<arr_fv> && !sized_range<arr_fv> && !contiguous_range<arr_fv>);
} // namespace

TEST(FilterView, FiltersUnrealContainer) {
    TArray<int> a{1, 2, 3, 4, 5, 6};
    int sum = 0;
    for (int x : views::filter(a, is_even)) {
        sum += x;
    }
    EXPECT_EQ(sum, 12);
}

TEST(FilterView, StatelessBeginRecomputes) {
    TArray<int> a{1, 2, 3, 4, 5, 6};
    auto fv = views::filter(a, is_even);
    EXPECT_EQ(fv.front(), 2);
    EXPECT_EQ(fv.front(), 2); // same result, no cached state
}

TEST(FilterView, EmptyWhenNothingMatches) {
    TArray<int> odds{1, 3, 5};
    EXPECT_TRUE(views::filter(odds, is_even).empty());
}

TEST(FilterView, ComposesWithTransformOverUnreal) {
    TArray<int> a{1, 2, 3, 4, 5, 6};
    int sum = 0;
    for (int x : a | views::filter(is_even) | views::transform(doubler)) {
        sum += x;
    }
    EXPECT_EQ(sum, 24); // (2+4+6)*2
}

TEST(FilterView, StdBase) {
    std::vector<int> v{10, 11, 12};
    int sum = 0;
    for (int x : v | views::filter(is_even)) {
        sum += x;
    }
    EXPECT_EQ(sum, 22);
}
