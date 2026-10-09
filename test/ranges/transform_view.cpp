// transform_view over a contiguous (TArray), forward (TSet), random-access
// non-contiguous (TDeque), and std (vector) base, in both build configurations.
#include "CoreMinimal.h"
#include "Containers/Deque.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int doubler(int x) { return x * 2; }

using tarray_tv = decltype(views::transform(std::declval<TArray<int>&>(), doubler));
using tset_tv = decltype(views::transform(std::declval<TSet<int>&>(), doubler));
using tdeque_tv = decltype(views::transform(std::declval<TDeque<int>&>(), doubler));

// Category: base capped at random_access; never contiguous (elements are computed).
static_assert(random_access_range<tarray_tv> && !contiguous_range<tarray_tv>);
static_assert(forward_range<tset_tv> && !random_access_range<tset_tv>);
static_assert(random_access_range<tdeque_tv> && !contiguous_range<tdeque_tv>);
static_assert(std::is_same_v<range_value_t<tarray_tv>, int> && sized_range<tarray_tv>);
} // namespace

TEST(TransformView, ContiguousBaseIterateSizeIndex) {
    TArray<int> a{1, 2, 3};
    auto va = views::transform(a, doubler);
    int sum = 0;
    for (int x : va) {
        sum += x;
    }
    EXPECT_EQ(sum, 12);
    EXPECT_EQ(static_cast<int>(va.size()), 3);
    EXPECT_EQ(va[2], 6); // element_at -> doubler(element_at(a, 2))
}

TEST(TransformView, RandomAccessNonContiguousBaseIndexes) {
    TDeque<int> d{10, 20};
    auto vd = views::transform(d, doubler);
    EXPECT_EQ(vd[1], 40); // via element_at; TDeque's iterator is forward-only
}

TEST(TransformView, ForwardBaseIterates) {
    TSet<int> set{5, 6};
    int sum = 0;
    for (int x : views::transform(set, doubler)) {
        sum += x;
    }
    EXPECT_EQ(sum, 22);
}

TEST(TransformView, StdBasePipeAndIndex) {
    std::vector<int> sv{7, 8};
    EXPECT_EQ((sv | views::transform(doubler))[0], 14);
}
