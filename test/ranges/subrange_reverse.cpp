// subrange and reverse_view, both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int doubler(int x) { return x * 2; }
} // namespace

TEST(Subrange, WrapsUnrealIteratorPair) {
    TArray<int> a{1, 2, 3, 4};
    auto sub = subrange(zr::ranges::begin(a), zr::ranges::end(a));
    static_assert(forward_range<decltype(sub)>);
    // Shipping: subrange<T*>; debug: ue_range_traits.h classifies the checked iterator alike.
#if ZR_FAKE_UE_VERSION >= 507 || !ZR_FAKE_UE_RANGED_FOR_CHECKS // the checked iterator's operator-> (UE 5.7)
    static_assert(contiguous_range<decltype(sub)> && sized_range<decltype(sub)>);
#else
    static_assert(bidirectional_range<decltype(sub)> && !random_access_range<decltype(sub)> && !sized_range<decltype(sub)>);
#endif
    int sum = 0;
    for (int v : sub) {
        sum += v;
    }
    EXPECT_EQ(sum, 10);
}

TEST(Subrange, WrapsStdIteratorPair) {
    std::vector<int> v{5, 6};
    int sum = 0;
    for (int x : subrange(v.begin(), v.end())) {
        sum += x;
    }
    EXPECT_EQ(sum, 11);
}

TEST(ReverseView, IndexedReverseOverUnreal) {
    TArray<int> a{1, 2, 3, 4};
    auto rev = views::reverse(a);
    static_assert(random_access_range<decltype(rev)>);
    static_assert(sized_range<decltype(rev)>);
    EXPECT_EQ(static_cast<int>(rev.size()), 4);
    EXPECT_EQ(rev[0], 4); // last element
    EXPECT_EQ(rev.front(), 4);

    std::vector<int> order;
    for (int x : rev) {
        order.push_back(x);
    }
    EXPECT_EQ(order, (std::vector<int>{4, 3, 2, 1}));
}

TEST(ReverseView, ComposesAndRoundTrips) {
    std::vector<int> w{1, 2, 3};
    TArray<int> out = w | views::reverse | views::transform(doubler) | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(out.Num(), 3);
    EXPECT_EQ(out[0] + out[1] + out[2], 12); // (3+2+1)*2

    TArray<int> a{1, 2, 3, 4};
    TArray<int> rr = a | views::reverse | views::reverse | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(rr[0] + rr[1] + rr[2] + rr[3], 10); // round-trips to 1,2,3,4
}
